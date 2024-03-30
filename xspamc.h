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

#ifndef _XSPAMC_H
#define _XSPAMC_H

typedef struct s_options {
	char *IniPath;
	unsigned short SpamdPort;
	char *SpamdUser;
	char *SpamdHost;
	char *UsedHostStr;
	unsigned int BackupLevel;
	unsigned long MaxFileSize;
	float Score2Delete;
	char *RejectSenderTemplate;
	char *RejectReceiverTemplate;
	unsigned int ExcludeXmailLmail;
	unsigned int ExcludeXmailDsn;
	unsigned int XmailStopCode;
	unsigned int XmailPassCode;
	char *XmailRoot;
	char *QuarantineFolderPath;
	unsigned int QuarantineLevel;
	unsigned int Log;
	unsigned int Outbound;
	unsigned int ReformatNewLines;
} options_t;


typedef struct s_ptrs {
	char *Xspamc_DIR;
	char *Xspamc_LOG_DIR;
	char *Xspamc_TMP_DIR;
	char *Xspamc_MSG_DIR;
	char *Xmail_TEMP_DIR;
	char *Xmail_LOCAL_DIR;
	char *Domain;
	char *User;
	char *FileTmpPath;
	char *FileResPath;
	char *TempFileName;
	char *FilePath;
} ptrs_t;

#endif
