/*
 *	XSpamc - Spamassassin filter for Xmail
 *  Copyright (C) 2004 2005  Dario Jakopec
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Suggestions, patches and comments are welcome.
 *  Dario Jakopec <dario@henry.it>
 *
 *	Release 0.4.4 - 05/08/2005
 *
 */

#include "includes.h"
#include "xspamc.h"
#include "dep.h"
#include "libspamc.h"
#include "utility.h"

void free_ptrs(ptrs_t *ptr, options_t *option);
void free_first_ptrs(ptrs_t *ptr, options_t *option);
void PrintUsage();

void free_ptrs(ptrs_t *ptr, options_t *option) {

	free(ptr->Xspamc_DIR);
	free(ptr->Xspamc_LOG_DIR);
	free(ptr->Xspamc_TMP_DIR);
	free(ptr->Xspamc_MSG_DIR);
	free(ptr->Xmail_TEMP_DIR);
	free(ptr->Xmail_LOCAL_DIR);
	free(ptr->Domain);
	free(ptr->User);
	free(ptr->FileTmpPath);
	free(ptr->FileResPath);
	free(ptr->TempFileName);
	free(ptr->FilePath);

	free(option->SpamdUser);
	free(option->SpamdHost);
	free(option->RejectReceiverTemplate);
	free(option->RejectSenderTemplate);
	free(option->XmailRoot);
	free(option->UsedHostStr);
	free(option->QuarantineFolderPath);

}

void free_first_ptrs(ptrs_t *ptr, options_t *option) {

	free(ptr->Xspamc_DIR);
	free(ptr->Xspamc_LOG_DIR);
	free(ptr->FilePath);

	free(option->SpamdUser);
	free(option->SpamdHost);
	free(option->RejectReceiverTemplate);
	free(option->RejectSenderTemplate);
	free(option->XmailRoot);

}

void PrintUsage()
{

	printf("\r\n   %s version %s\r\n", "Xmail Server SpamAssassin Filter", VERSION_STRING);
	printf("   %s\r\n",C_STRING);
	printf("   Command line usage is not supported.\r\n\r\n");

}

int main(int argc,char **argv) {

	ptrs_t ptr;
	options_t option;

	unsigned long FileSize = 0;
	int hFile_in = 0;
	int hFile_out = 0;
	int exstatus = 0;
	int i = 0;
	int opt_args = 0;
	float score = 0;
	float threshold = 0;
	char *pch;
	char line[2048] = "";
	char linetmp[2048] = "";
	char Buf[512] = {0};
	char LogBuf[512] = {0};
	FILE *oStream = NULL;
	FILE *tStream = NULL;
	FILE *rStream = NULL;
	FILE *fLog = NULL;
#ifdef WIN32
	char new_line[] = "\n";
	struct timeb timeStart;
	ftime (&timeStart);
#else
	char new_line[] = "\r\n";
	struct timeval timeStart;
	gettimeofday(&timeStart, NULL);
#endif

	// Check if we have minimal parameters in tab file 
	if(argc<6 || argc>7) {
		PrintUsage();
		return (-1);
	}

	opt_args = argc-6;

#ifdef WIN32
	if (IsUncPath(argv[opt_args+1]) == 0)  {
		ptr.FilePath = RemoveUncPathPrefix(argv[opt_args+1]);
	}
	else {
		ptr.FilePath = StrDuplicate(argv[opt_args+1]);
	}
#else
	ptr.FilePath = StrDuplicate(argv[opt_args+1]);
#endif

	// Check if we have a file path (@@FILE) 
	if(strrchr(ptr.FilePath, SLASH_STR)==NULL)
		return (-2);

	// find the xspamc dir path 
	sprintf(Buf,"%s",argv[0]);
#ifdef WIN32
	Buf[strlen(argv[0])-10] = '\0';
#else
	Buf[strlen(argv[0])-6] = '\0';
#endif
	ptr.Xspamc_DIR = StrDuplicate(Buf);

	// check if we have the ini file
	if (argc == 7)
		option.IniPath = StrDuplicate(argv[1]);
	else if (argc == 6) {
		sprintf(Buf,"%s%c%s",ptr.Xspamc_DIR,SLASH_STR,"xspamc.ini");
		option.IniPath = StrDuplicate(Buf);
	}

	if ((oStream=fopen(option.IniPath,"r")) == NULL) {
		free(ptr.Xspamc_DIR);
		free(option.IniPath);
		free(ptr.FilePath);
		return (-3);
	}

	fclose(oStream);

	if (ULoadDefaults(&option) != 0){
		free(ptr.Xspamc_DIR);
		free(option.IniPath);
		free(ptr.FilePath);
		return (-4);
	}

	free(option.IniPath);
	
	// set the log dir path 
	sprintf(Buf,"%slog",ptr.Xspamc_DIR);
	ptr.Xspamc_LOG_DIR = StrDuplicate(Buf);

	// Check if we have the log dir, if not create it 
	if (ChangeDir(ptr.Xspamc_LOG_DIR) != 0)
		if (MakeDir(ptr.Xspamc_LOG_DIR) != 0) {
			free(ptr.Xspamc_DIR);
			free(ptr.Xspamc_LOG_DIR);
			free(option.IniPath);
			free(ptr.FilePath);
			return (-5);
		}

	//* check if the message is Xmail DSN */
	if (option.ExcludeXmailDsn) {
		if (strncmp(argv[opt_args+4],"X",1) == 0) {
			if (option.Log > 0) {
				sprintf(LogBuf,"%s	0	%s	%s	%s	%d	Message comes from Xmail DSN",argv[opt_args+4],argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
				UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
			}
			free_first_ptrs(&ptr, &option);
			return 0;
		}
	}

	//* check if the message comes from LMAIL */
	if (option.ExcludeXmailLmail) {
		if (strncmp(argv[opt_args+4],"L",1) == 0) {
			if (option.Log > 0) {
				sprintf(LogBuf,"%s	0	%s	%s	%s	%d	Message comes from LMAIL",argv[opt_args+4],argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
				UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
			}
			free_first_ptrs(&ptr, &option);
			return 0;

		}
	}

	// check if file exceeds MaxFileSize
	FileSize = Get_FileSize(ptr.FilePath);
	if (FileSize < 0){
		if (option.Log > 0) {
			sprintf(LogBuf,"%s	0	%s	%s	%s	%d	ERROR - Cannot get file size",argv[opt_args+4],argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		free_first_ptrs(&ptr, &option);
		return 0;
	}
	if ((long)option.MaxFileSize < FileSize) {
		if (option.Log > 0) {
			sprintf(LogBuf,"%s	0	%s	%s	%s	%d	Message exceeds MaxFileSize",argv[opt_args+4],argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		free_first_ptrs(&ptr, &option);
		return 0;
	}

	// find the domain of the rcpt 
	pch = strstr(argv[opt_args+3],"@");
	sprintf(Buf,"%s",pch+1);
	ptr.Domain = StrDuplicate(Buf);
	
	// find the username of the rcpt 
	pch = strchr(argv[opt_args+3],'@');
	strcpy(Buf,argv[opt_args+3]);
	Buf[pch - argv[opt_args+3]] = '\0';
	ptr.User = StrDuplicate(Buf);

	// set the msg dir path 
	sprintf(Buf,"%smsg",ptr.Xspamc_DIR);
	ptr.Xspamc_MSG_DIR = StrDuplicate(Buf);

	// set the tmp dir path 
	sprintf(Buf,"%stmp",ptr.Xspamc_DIR);
	ptr.Xspamc_TMP_DIR = StrDuplicate(Buf);

	// set the spool tmp dir path 
	sprintf(Buf,"%s%cspool%ctemp",option.XmailRoot,SLASH_STR,SLASH_STR);
	ptr.Xmail_TEMP_DIR = StrDuplicate(Buf);

	// set the spool local dir path 
	sprintf(Buf,"%s%cspool%clocal",option.XmailRoot,SLASH_STR,SLASH_STR);
	ptr.Xmail_LOCAL_DIR = StrDuplicate(Buf);

	ptr.FileTmpPath = StrDuplicate("");
	ptr.FileResPath = StrDuplicate("");
	ptr.TempFileName = StrDuplicate("");

	// Check if we have the tmp dir, if not create it 
	if (ChangeDir(ptr.Xspamc_TMP_DIR) != 0)
		if (MakeDir(ptr.Xspamc_TMP_DIR) != 0) {
			/* Log Format:
			 * Date[TAB]Time[TAB]MSGREF[TAB]IP:PORT[TAB]FROM[TAB]RCPT[TAB]HITS[TAB]INFO[CR][LF]
			 */
			if (option.Log > 0) {
				sprintf(LogBuf,"%s	0	%s	%s	%s	%d	ERROR - Cannot create tmp dir",argv[opt_args+4],argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
				UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
			}
			free_ptrs(&ptr, &option);
			return 0;
		}

	GenerateTmpFileName("T", ptr.Xspamc_TMP_DIR, Buf);
	ptr.FileTmpPath = StrDuplicate(Buf);
	GenerateTmpFileName("R", ptr.Xspamc_TMP_DIR, Buf);
	ptr.FileResPath = StrDuplicate(Buf);

	// Copy message to tmp dir 
	if (FileCopy(ptr.FilePath, ptr.FileTmpPath) != 0) {
		if (option.Log > 0) {
			sprintf(LogBuf,"%s	0	%s	%s	%s	%d	ERROR - Cannot copy msg to tmp dir",argv[opt_args+4],argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		free_ptrs(&ptr, &option);
		return 0;
	}
	
	// Open files or exit
	if( (oStream  = fopen( ptr.FilePath, "rb" )) == NULL ) {
		if (option.Log > 0) {
			sprintf(LogBuf,"%s	0	%s	%s	%s	%d	ERROR - Cannot open msg file",argv[opt_args+4],argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		free_ptrs(&ptr, &option);
		return 0;
	}
	if( (rStream  = fopen( ptr.FileResPath, "wb" )) == NULL ) {
		if (option.Log > 0) {
			sprintf(LogBuf,"%s	0	%s	%s	%s	%d	ERROR - Cannot open msg res file",argv[opt_args+4],argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		fclose(oStream);
		free_ptrs(&ptr, &option);
		return 0;
	}
	if( (tStream  = fopen( ptr.FileTmpPath, "wb" )) == NULL ) {
		if (option.Log > 0) {
			sprintf(LogBuf,"%s	0	%s	%s	%s	%d	ERROR - Cannot open msg tmp file",argv[opt_args+4],argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		fclose(oStream);
		fclose(tStream);
		RemoveFile(ptr.FileResPath);
		free_ptrs(&ptr, &option);
		return 0;
	}

	// copy the first 6 lines of the original mail to res file and the rest to tmp file
	// COPY DATA UP TO <<MAIL-DATA>> TAG TO RES FILE AND THE REST TO TMP FILE.
	while (fgets(line, sizeof(line) - 1, oStream) != NULL) {
		if (!i) {
			fputs(line,rStream);
			if (line[0] == '<')
				if (!strncmp(line,MAIL_DATA,sizeof(MAIL_DATA)-1) != NULL)
					i = 1;
		}
		else 
			fputs(line,tStream);
	}

	fclose(tStream);
	fclose(rStream);
	fclose(oStream);

	hFile_in = open( ptr.FileTmpPath, O_RDONLY );
	// cannot open file handle
	if (hFile_in == -1) {
		RemoveFile(ptr.FileTmpPath);
		RemoveFile(ptr.FileResPath);
		if (option.Log > 0) {
			sprintf(LogBuf,"%s	0	%s	%s	%s	%d	ERROR - Cannot open msg tmp file",argv[opt_args+4],argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		free_ptrs(&ptr, &option);
		return 0;
	}
	hFile_out = open( ptr.FileResPath,O_WRONLY | O_APPEND );
	// cannot open file handle
	if (hFile_out == -1) {
		close( hFile_in );
		RemoveFile(ptr.FileResPath);
		RemoveFile(ptr.FileTmpPath);
		if (option.Log > 0) {
			sprintf(LogBuf,"%s	0	%s	%s	%s	%d	ERROR - Cannot open msg res file",argv[opt_args+4],argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		free_ptrs(&ptr, &option);
		return 0;
	}

	exstatus = process_message(hFile_in,hFile_out,option.SpamdHost,option.SpamdPort,
								option.SpamdUser,option.MaxFileSize,Buf);

	option.UsedHostStr = StrDuplicate(Buf);

	close( hFile_in );
	close( hFile_out );

	// if we have errors log them and exit
	if (exstatus != EX_OK) {
		if (option.Log > 0) {
			Get_Error_Str(exstatus, Buf);
			sprintf(LogBuf,"%s	%s	%s	%s	%s	%i	%s (Error Code: %i)",argv[opt_args+4],option.UsedHostStr,argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0,Buf,exstatus);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		RemoveFile(ptr.FileResPath);
		RemoveFile(ptr.FileTmpPath);
		free_ptrs(&ptr, &option);
		return 0;
	}

	// get spam hits and required hits for spam 
	if( (rStream  = fopen( ptr.FileResPath, "rb" )) == NULL ) {
		RemoveFile(ptr.FileResPath);
		RemoveFile(ptr.FileTmpPath);
		if (option.Log > 0) {
			sprintf(LogBuf,"%s	%s	%s	%s	%s	%d	ERROR - Cannot open res file",argv[opt_args+4],option.UsedHostStr,argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		free_ptrs(&ptr, &option);
		return 0;
	}
	while (fgets(line,sizeof(line) - 1,rStream)){
		if (strstr(line,"X-Spam-Status: ") != NULL) {
			pch = strstr(line,"hits=");
			if (pch == NULL) { // added for spamassassin 3.x compatibility
				pch = strstr(line,"score=");
				sscanf(pch,"score=%f",&score);
			}
			else
				sscanf(pch,"hits=%f",&score);
			pch = strstr(line,"required=");
			sscanf(pch,"required=%f",&threshold);
			break;
		}
	}

	/* reformat report_safe */

	if (option.ReformatNewLines) {
		tStream = fopen( ptr.FileTmpPath, "wb" );
		rewind(rStream);
		while (fgets(line, sizeof(line) - 1, rStream) != NULL) {
			linetmp[0] = 0;
			// just in case... reformat eol to crlf avoiding crcrlf
			for (pch = strtok(line, "\r\n"); pch; pch = strtok(NULL, "\r\n")) {
				strcat(linetmp,pch);
			};
			sprintf(line,"%s%s",linetmp,new_line);
			fputs(line,tStream);
		}
		fclose(tStream);
		FileCopy( ptr.FileTmpPath, ptr.FileResPath);
	}
	fclose(rStream);

	// modify the original message
	
	if (FileCopy( ptr.FileResPath, ptr.FilePath) != 0) {
		RemoveFile(ptr.FileResPath);
		RemoveFile(ptr.FileTmpPath);
		if (option.Log > 0) {
			sprintf(LogBuf,"%s	%s	%s	%s	%s	%d	ERROR - Cannot copy msg res file",argv[opt_args+4],option.UsedHostStr,argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],0);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		free_ptrs(&ptr, &option);
		return 0;
	}

	/* write log if it's spam under threshold */
	if (score > threshold && score < option.Score2Delete) {
		/* save also under threshold if BackupLevel=2 */
		if (option.BackupLevel > 1) {
			sprintf(Buf,"%s%cspam",ptr.Xspamc_DIR,SLASH_STR);
			if (ChangeDir(Buf) != 0) {
				MakeDir(Buf);
			}
			sprintf(Buf,"%s%cspam%cprob",ptr.Xspamc_DIR,SLASH_STR,SLASH_STR);
			if (ChangeDir(Buf) != 0) {
				MakeDir(Buf);
			}
			sprintf(Buf,"%s%cspam%cprob%c%s.eml",ptr.Xspamc_DIR,SLASH_STR,SLASH_STR,SLASH_STR,argv[opt_args+4]);
			SpoolCopy(ptr.FileResPath,Buf);
		}
		if ((strlen(option.QuarantineFolderPath) > 0) && (option.QuarantineLevel > 0)) {

			sprintf(Buf,"%s%cdomains%c%s%c%s%c%s",option.XmailRoot,SLASH_STR,SLASH_STR,
												  ptr.Domain,SLASH_STR,ptr.User,SLASH_STR,
												  option.QuarantineFolderPath);
			if (ChangeDir(Buf) != 0) {
				MakeDir(Buf);
			}
			pch = strrchr(ptr.FilePath,SLASH_STR);
			strcat(Buf,pch);
			SpoolCopy(ptr.FileResPath,Buf);

		}
		RemoveFile(ptr.FileResPath);
		RemoveFile(ptr.FileTmpPath);

		if (option.Log > 0) {
			sprintf(LogBuf,"%s	%s	%s	%s	%s	%.1f	Spam Found Under Threshold",argv[opt_args+4],option.UsedHostStr,argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],score);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		free_ptrs(&ptr, &option);

		// If quarantine is enabled stop delivering since is already in the quarantine folder
		if ((strlen(option.QuarantineFolderPath) > 0) && (option.QuarantineLevel > 0))
			return option.XmailStopCode;
		else
			return option.XmailPassCode;
	}
	/* save if spam */
	if (score >= option.Score2Delete) {
		/* Move res to spam folder if specified */
		if (option.BackupLevel > 0) {
			sprintf(Buf,"%s%cspam",ptr.Xspamc_DIR,SLASH_STR);
			if (ChangeDir(Buf) != 0) {
				MakeDir(Buf);
			}
			sprintf(Buf,"%s%cspam%c%s.eml",ptr.Xspamc_DIR,SLASH_STR,SLASH_STR,argv[opt_args+4]);
			SpoolCopy(ptr.FileResPath,Buf);
		}
		if ((strlen(option.QuarantineFolderPath) > 0) && (option.QuarantineLevel > 1)) {

			sprintf(Buf,"%s%cdomains%c%s%c%s%c%s",option.XmailRoot,SLASH_STR,SLASH_STR,
												  ptr.Domain,SLASH_STR,ptr.User,SLASH_STR,
												  option.QuarantineFolderPath);
		
			if (ChangeDir(Buf) != 0) {
				MakeDir(Buf);	
			}

			pch = strrchr(ptr.FilePath,SLASH_STR);
			strcat(Buf,pch);
			SpoolCopy(ptr.FileResPath,Buf);

		}
		RemoveFile(ptr.FileResPath);
		RemoveFile(ptr.FileTmpPath);
		if (option.Log > 0) {
			sprintf(LogBuf,"%s	%s	%s	%s	%s	%.1f	Spam Deleted",argv[opt_args+4],option.UsedHostStr,argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],score);
			UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
		}
		
		if (strlen(option.RejectReceiverTemplate) > 0 || strlen(option.RejectSenderTemplate) > 0) {
			FILE * FMsg=NULL;
			FILE * FTxt=NULL;
			/* Prepare message to <from> in spool temp dir */
			if ((strlen(option.RejectSenderTemplate) > 0) && (strchr(argv[opt_args+2],'@') != NULL)) {
				sprintf(Buf,"%s%c%s",ptr.Xspamc_MSG_DIR,SLASH_STR,option.RejectSenderTemplate);
				if ((FTxt=fopen(Buf,"rt")) != NULL) {
					if ((FMsg=fopen(ptr.FileTmpPath,"wt"))!=NULL) {
						fprintf(FMsg,"mail from:<postmaster@%s>%s",ptr.Domain,new_line);
						fprintf(FMsg,"rcpt to:<%s>%s%s",argv[opt_args+2],new_line,new_line);
						fprintf(FMsg,"From:<postmaster@%s>%s",ptr.Domain,new_line);
						fprintf(FMsg,"To:<%s>%s",argv[opt_args+2],new_line);
						while (fgets(Buf,MAX_PATH,FTxt))
							fputs(Buf,FMsg);
						fclose(FTxt);
						fprintf(FMsg,"%s",new_line);
						fprintf(FMsg,"From: <%s> To: <%s>%s%s%s",argv[opt_args+2],argv[opt_args+3],new_line,new_line,new_line);
						fclose(FMsg);
						/* move message to spool local dir */
						pch = strstr(ptr.FileTmpPath,"tmp");
						strcpy(Buf,pch+4);
						ptr.TempFileName = StrDuplicate(Buf);
						sprintf(Buf,"%s%cS%s",ptr.Xmail_LOCAL_DIR,SLASH_STR,ptr.TempFileName);
						FileCopy(ptr.FileTmpPath,Buf);
						RemoveFile(ptr.FileTmpPath);
					}
				}
			}
			/* Prepare message to <rcpt> in spool temp dir */
			if ((strlen(option.RejectReceiverTemplate) > 0) && (strchr(argv[opt_args+2],'@') != NULL) && (strchr(argv[opt_args+3],'@') != NULL)) {
				sprintf(Buf,"%s%c%s",ptr.Xspamc_MSG_DIR,SLASH_STR,option.RejectReceiverTemplate);
				if ((FTxt=fopen(Buf,"rt")) != NULL) {
					if ((FMsg=fopen(ptr.FileTmpPath,"wt"))!=NULL) {
						fprintf(FMsg,"mail from:<postmaster@%s>%s",ptr.Domain,new_line);
						fprintf(FMsg,"rcpt to:<%s>%s%s",argv[opt_args+3],new_line,new_line);
						fprintf(FMsg,"From:<postmaster@%s>%s",ptr.Domain,new_line);
						fprintf(FMsg,"To:<%s>%s",argv[opt_args+3],new_line);
						fputs(Buf,FMsg);
						while (fgets(Buf,MAX_PATH,FTxt))
							fputs(Buf,FMsg);
						fclose(FTxt);
						fprintf(FMsg,"%s",new_line);
						fprintf(FMsg,"From: <%s> To: <%s>%s%s%s",argv[opt_args+2],argv[opt_args+3],new_line,new_line,new_line);
						fclose(FMsg);
						/* move message to spool local dir */
						free(ptr.TempFileName);
						pch = strstr(ptr.FileTmpPath,"tmp");
						strcpy(Buf,pch+4);
						ptr.TempFileName = StrDuplicate(Buf);
						sprintf(Buf,"%s%cR%s",ptr.Xmail_LOCAL_DIR,SLASH_STR,ptr.TempFileName);
						FileCopy(ptr.FileTmpPath,Buf);
						RemoveFile(ptr.FileTmpPath);
					}
				}
			}
		}
		free_ptrs(&ptr, &option);
		return option.XmailStopCode;
	}
	
	//RemoveFile(ptr.FileResPath);
	//RemoveFile(ptr.FileTmpPath);

	if (option.Log > 0) {
		sprintf(LogBuf,"%s	%s	%s	%s	%s	%.1f	Clean Message",argv[opt_args+4],option.UsedHostStr,argv[opt_args+5],argv[opt_args+2],argv[opt_args+3],score);
		UWriteLog(&timeStart,LogBuf,ptr.Xspamc_LOG_DIR);
	}

	/* Not spam but we still have modified the message */
	free_ptrs(&ptr, &option);
	return option.XmailPassCode;

}
