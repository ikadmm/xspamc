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

#ifndef _DEP_H
#define _DEP_H

char *StrDuplicate(char *pszString);
int RemoveFile(const char *pszFileName);
int FileCopy(char *pszFrom, char *pszTo);
int SpoolCopy(char *pszFrom, char *pszTo);
int MakeDir(const char *pszPath);
int ChangeDir(const char *pszPath);
unsigned long Get_FileSize(char *pszFileName);
void *SysAlloc(unsigned int uSize);
void GenerateTmpFileName(char *pszFilePrefix, char *pszDirPath, char *pszFileName);
void Init_Socket(void);
void Cleanup_Socket(void);
int Get_Socket_Error(void);
void Get_Error_Str(int exstatus, char *Buf);
#ifdef WIN32
void UWriteLog (struct timeb *timeStart, char *str_log, char *Xspamc_DIR);
char *RemoveUncPathPrefix(const char *pszFilePath);
int IsUncPath (const char *pszFileName);
#else
void UWriteLog (struct timeval *timeStart, char *str_log, char *Xspamc_DIR);
#endif

#define C_STRING "Copyright (C) 2004 2005 Dario Jakopec"
#define MAIL_DATA	"<<MAIL-DATA>>"

#endif

