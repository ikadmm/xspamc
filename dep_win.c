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
#include "dep.h"
#include "libspamc.h"

static double GetElapsedMillis(struct timeb *start);

int RemoveFile(const char *pszFileName)
{

	if (!DeleteFile(pszFileName)) {
		return (-1);
	}

	return (0);

}

int FileCopy(char *pszFrom, char *pszTo)
{

	if (!CopyFile(pszFrom,pszTo,FALSE)) {
		return (-1);
	}

	return (0);

}

int SpoolCopy(char *pszFrom, char *pszTo)
{

	FILE  *fold = NULL, *fnew = NULL;
	char line[2048] = "";
	char buf[2048] = "";
	unsigned int i = 0;

	if((fold = fopen(pszFrom, "rb")) == NULL)
		return  (-1);

	if((fnew = fopen(pszTo, "wb" )) == NULL) {
		fclose(fold);
		return  (-1);
	}

	while (fgets(line, sizeof(line) - 1, fold)){
		if (i)
			fputs(line,fnew);
		else
			if (line[0] == '<')
				if (!strncmp(line,MAIL_DATA,sizeof(MAIL_DATA)-1))
					i = 1;
	}

	fclose(fnew);
	fclose(fold);

	return (0);

}

int MakeDir(const char *pszPath)
{

	if (!CreateDirectory(pszPath, NULL)) {
		return (-1);
	}

	return (0);

}

int ChangeDir(const char *pszPath)
{

	if (!SetCurrentDirectory(pszPath)) {
		return (-1);
	}

	return (0);
}

void GenerateTmpFileName(char *pszFilePrefix, char *pszDirPath, char *pszFileName)
{

	GetTempFileName(pszDirPath, pszFilePrefix, 0, pszFileName);

}


char *StrDuplicate(char *pszString)
{

	int iStrLength = strlen(pszString);
	char *pszBuffer = (char *) SysAlloc(iStrLength + 1);

	if (pszBuffer != NULL)
		strcpy(pszBuffer, pszString);

	return (pszBuffer);

}

void *SysAlloc(unsigned int uSize)
{

	void *pData = malloc(uSize);

	if (pData != NULL)
		memset(pData, 0, uSize);

	return (pData);

}

unsigned long Get_FileSize(char *pszFileName)
{

	long size;
	int hFile_in = open( pszFileName, O_RDONLY );

	if (hFile_in == -1)
		return (-1);

	size = _filelength(hFile_in);
	close(hFile_in);

	if ( size < 0 )
		return(-2);
	
	return ((unsigned long) size);

}

void Init_Socket(void) 
{

	WSADATA	wsaData;
	WSAStartup(SOCK_VERSION, &wsaData);	

}

void Cleanup_Socket(void) 
{

	WSACleanup();	

}

int Get_Socket_Error(void)
{
	return WSAGetLastError();
}

void Get_Error_Str(int exstatus, char *Buf)
{
	switch (exstatus) {
	case EX_TIMEOUT:
		sprintf(Buf,"SOCKET ERROR - Connection to host timed out");
		break;
	case EX_SOFTWARE:
		sprintf(Buf,"ERROR - Internal software error");
		break;
	case EX_OSERR_MEM:
		sprintf(Buf,"ERROR - Cannot allocate memory");
		break;
	case EX_PROTOCOL:
		sprintf(Buf,"ERROR - Remote protocol error");
		break;
	case EX_IOERR:
		sprintf(Buf,"ERROR - Dump Input Output error");
		break;
	case EX_SERV:
		sprintf(Buf,"ERROR - Spamd Version Not Supported");
		break;
	case EX_MSG_BIG:
		sprintf(Buf,"ERROR - Message is too big");
		break;
	case EX_MSG_MIS:
		sprintf(Buf,"ERROR - Message is corupted");
		break;
	case WSAECONNREFUSED:
		sprintf(Buf,"SOCKET ERROR - Connection Refused");
		break;
	case WSAEACCES:
		sprintf(Buf,"SOCKET ERROR - Access denied");
		break;
	case WSAENOBUFS:
		sprintf(Buf,"SOCKET ERROR - No buffer space available");
		break;
	case WSAENETUNREACH:
		sprintf(Buf,"SOCKET ERROR - Network is unreachable");
		break;
	case WSAEHOSTUNREACH:
		sprintf(Buf,"SOCKET ERROR - Host is unreachable");
		break;
	case WSAETIMEDOUT:
		sprintf(Buf,"SOCKET ERROR - Operation timed out");
		break;
	case WSAEAFNOSUPPORT:
		sprintf(Buf,"SOCKET ERROR - Address family not supported");
		break;
	case WSAEFAULT:
		sprintf(Buf,"SOCKET ERROR - Segmentation fault");
		break;
	}

}

static double GetElapsedMillis(struct timeb *start)
{

	struct timeb timeEnd;
	int second, milliSecond;
	ftime (&timeEnd);
	second = (int) difftime (timeEnd.time, start->time);
	milliSecond = timeEnd.millitm - start->millitm;

	return ((double) (second + milliSecond / 1000.0 + 0.001));

}

void UWriteLog (struct timeb *timeStart, char *str_log, char *Xspamc_DIR) {

	FILE *fLog = NULL;
	char Buf[MAX_PATH] = {0};
	char timeBuf [10] = {0};
	char utc_date[10] = {0};
	char dateBuf[12] = {0};
	char my_dateBuf[12] = {0};
	struct tm *tm_ptr;
	double elapsed_time;
	size_t nchar;
	time_t now;

	/* Obtain dates */
	time(&now);                
	tm_ptr = localtime(&now);
	/* Obtain today's date in yyyymmdd format */
	nchar = strftime(utc_date, sizeof(utc_date),"%Y%m%d",tm_ptr);
	/* Obtain today's date in yyyy-mm-dd format */
	nchar = strftime(my_dateBuf, sizeof(dateBuf),"%Y-%m-%d",tm_ptr);
	/* Obtain today's date in dd/mm/yyyy format */
	nchar = strftime(dateBuf, sizeof(dateBuf),"%d/%m/%Y",tm_ptr);
	nchar = strftime(timeBuf, sizeof(timeBuf),"%H:%M:%S",tm_ptr);
	elapsed_time = GetElapsedMillis(timeStart);
	//_strtime(timeBuf);
	/* Log Format:
	 * Date[TAB]Time[TAB]MSGREF[TAB]IP:PORT[TAB]FROM[TAB]RCPT[TAB]HITS[TAB]INFO[CR][LF]
	 */

	sprintf(Buf,"%s%cxspamc-%s.txt",Xspamc_DIR, SLASH_STR,utc_date);
	if ((fLog = fopen(Buf,"at")) != NULL) {
		fprintf(fLog,"%s	%s	%.3f	%s\r\n",my_dateBuf,timeBuf,elapsed_time,str_log);
		fclose(fLog);
	}

}

int IsUncPath (const char *pszFileName)
{

	return (strncmp(pszFileName,"\\\\",2));

}

char *RemoveUncPathPrefix(const char *pszFilePath)
{
	int iFilePathLength = strlen(pszFilePath);
	char *pszDest = strchr(pszFilePath,':');
	int iDestLength = strlen(pszDest)+1;
	int res = iFilePathLength - iDestLength;
	char *pszBuffer = (char *) SysAlloc(iDestLength + 2);

	if (pszBuffer != NULL)
		strcpy(pszBuffer, pszFilePath + res);

	return (pszBuffer);

}
