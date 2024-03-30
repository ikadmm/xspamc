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

static int TimevalSubtract(struct timeval *result, struct timeval *x, struct timeval *y);
static double GetElapsedMillis(struct timeval *start);

int RemoveFile(const char *pszFileName)
{

	if (unlink(pszFileName) != 0) {
		return (-1);
	}

	return (0);

}

int FileCopy(char *pszFrom, char *pszTo)
{

	FILE  *fold = NULL, *fnew = NULL;
	int  c;

	if((fold = fopen(pszFrom, "rb")) == NULL)
		return  (-1);

	if((fnew = fopen(pszTo, "wb" )) == NULL) {
		fclose(fold);
		return  (-1);
	}

	while(1) {
		c  =  fgetc(fold);
		if(!feof(fold))
			fputc(c, fnew);
		else
			break;
	}

	fclose(fnew);
	fclose(fold);

	return  (0);

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

	if (mkdir(pszPath, 0700) != 0) {
		return (-1);
	}

	return (0);

}

int ChangeDir(const char *pszPath)
{

	if (chdir(pszPath) != 0) {
		return (-1);
	}

	return (0);
}

void GenerateTmpFileName(char *pszFilePrefix, char *pszDirPath, char *pszFileName)
{

	char buf[8] = "";
	int hFile = 0;
	sprintf(buf,"%sXXXXXX",pszFilePrefix);
	hFile = mkstemp (buf);
	sprintf(pszFileName,"%s%c%s",pszDirPath,SLASH_STR,buf);
	close(hFile);

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

	struct stat stat_p;

	if ( -1 ==  stat (pszFileName, &stat_p)) {
	  return (-1);
	}

	return ((unsigned long) stat_p.st_size);

}

void Init_Socket(void) {


}

void Cleanup_Socket(void) {


}

int Get_Socket_Error(void)
{
	return errno;
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
	case ECONNREFUSED:
		sprintf(Buf,"SOCKET ERROR - Connection Refused");
		break;
	case EACCES:
		sprintf(Buf,"SOCKET ERROR - Access denied");
		break;
	case ENOBUFS:
		sprintf(Buf,"SOCKET ERROR - No buffer space available");
		break;
	case ENETUNREACH:
		sprintf(Buf,"SOCKET ERROR - Network is unreachable");
		break;
	case EHOSTUNREACH:
		sprintf(Buf,"SOCKET ERROR - Host is unreachable");
		break;
	case ETIMEDOUT:
		sprintf(Buf,"SOCKET ERROR - Operation timed out");
		break;
	case EAFNOSUPPORT:
		sprintf(Buf,"SOCKET ERROR - Address family not supported");
		break;
	case EFAULT:
		sprintf(Buf,"SOCKET ERROR - Bad Address");
		break;
	}

}

static int TimevalSubtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
    /* Perform the carry for the later subtraction by updating Y. */
    if (x->tv_usec < y->tv_usec) {
	int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
	y->tv_usec -= 1000000 * nsec;
	y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
	int nsec = (x->tv_usec - y->tv_usec) / 1000000;
	y->tv_usec += 1000000 * nsec;
	y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       `tv_usec' is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

static double GetElapsedMillis(struct timeval *start)
{
    struct timeval difftime;
	struct timeval endtime;
    float et;
	gettimeofday(&endtime, NULL);
    TimevalSubtract(&difftime, &endtime, start);
    et = difftime.tv_sec * 1000.0 + difftime.tv_usec / 1000.0;
    return (et / 1000.0);
}

void UWriteLog (struct timeval *timeStart, char *str_log, char *Xspamc_DIR) 
{

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

	/* Log Format:
	 * Date[TAB]Time[TAB]MSGREF[TAB]IP:PORT[TAB]FROM[TAB]RCPT[TAB]HITS[TAB]INFO[CR][LF]
	 */

	sprintf(Buf,"%s%cxspamc-%s.txt",Xspamc_DIR, SLASH_STR,utc_date);
	if ((fLog = fopen(Buf,"at")) != NULL) {
		fprintf(fLog,"%s	%s	%.3f	%s\r\n",my_dateBuf,timeBuf,elapsed_time,str_log);
		fclose(fLog);
	}

}
