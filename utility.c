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
#include "utility.h"

#define MAX_FILE_SIZE 256000

int ULoadDefaults(options_t *option) 
{

	FILE *cfil = NULL;
	char *tmp, *var, *val;
	char buf[512] = {0};
	char Hints2Delete[6] = {0};

	option->SpamdPort = 783;
	option->SpamdUser = StrDuplicate("spamd");
	option->SpamdHost = StrDuplicate("127.0.0.1");
	option->BackupLevel = 2;
	option->MaxFileSize = MAX_FILE_SIZE;
	option->Score2Delete = 10.0;
	option->RejectSenderTemplate = StrDuplicate("");
	option->RejectReceiverTemplate = StrDuplicate("");
	option->ExcludeXmailDsn = 1;
	option->ExcludeXmailLmail = 1;
	option->XmailStopCode = 4;
	option->XmailPassCode = 7;
	option->XmailRoot = StrDuplicate("");
	option->UsedHostStr = StrDuplicate("127.0.0.1");
	option->QuarantineFolderPath = StrDuplicate("");
	option->QuarantineLevel = 0;
	option->Log = 0;
	option->Outbound = 0;
	option->ReformatNewLines = 0;

	if ((cfil = fopen(option->IniPath, "r")) == NULL) {

		return -1;

	}

	while (fgets(buf, sizeof(buf) - 1, cfil) != NULL) {

		buf[strlen(buf) - 1] = 0;
		for (var = buf; *var && strchr(" \t", *var); var++);
		if (!*var)
			continue;
		for (tmp = var; *tmp && (isalpha(*tmp) || isdigit(*tmp)); tmp++);
		if (!*tmp || tmp == var || strchr(" \t=", *tmp) == NULL)
			continue;
		for (val = tmp; *val && strchr(" \t", *val) != NULL; val++);
		if (*val++ != '=')
			continue;
		*tmp++ = 0;
		for (; *val && strchr(" \t", *val) != NULL; val++);
		for (tmp = val + strlen(val); tmp > val && strchr(" \t\r\n", tmp[-1]) != NULL; tmp--);
		*tmp = 0;

		if (!strcmp(var, "SpamdPort")) {
			option->SpamdPort = atoi(val);
		} else if (!strcmp(var, "SpamdUser")) {
			free(option->SpamdUser);
			option->SpamdUser = StrDuplicate(val);
		} else if (!strcmp(var, "SpamdHost")) {
			free(option->SpamdHost);
			option->SpamdHost = StrDuplicate(val);
		} else if (!strcmp(var, "BackupLevel")) {
			option->BackupLevel = atoi(val);
		} else if (!strcmp(var, "MaxFileSize")) {
			option->MaxFileSize = atoi(val) * 1024;
			option->MaxFileSize = option->MaxFileSize * 1024;
			if ((option->MaxFileSize) > (MAX_FILE_SIZE)) 
				option->MaxFileSize = MAX_FILE_SIZE;
		} else if (!strcmp(var, "Hints2Delete")) {
			strcpy(Hints2Delete, val);
			option->Score2Delete = (float) atof(Hints2Delete);
		} else if (!strcmp(var, "RejectSenderTemplate")) {
			free(option->RejectSenderTemplate);
			option->RejectSenderTemplate = StrDuplicate(val);
		} else if (!strcmp(var, "RejectReceiverTemplate")) {
			free(option->RejectReceiverTemplate);
			option->RejectReceiverTemplate = StrDuplicate(val);
		} else if (!strcmp(var, "ExcludeXmailDsn")) {
			option->ExcludeXmailDsn = atoi(val);
		} else if (!strcmp(var, "ExcludeXmailLmail")) {
			option->ExcludeXmailLmail = atoi(val);
		} else if (!strcmp(var, "XmailStopCode")) {
			option->XmailStopCode = atoi(val);
		} else if (!strcmp(var, "XmailPassCode")) {
			option->XmailPassCode = atoi(val);
		} else if (!strcmp(var, "XmailRoot")) {
			free(option->XmailRoot);
			option->XmailRoot = StrDuplicate(val);
		} else if (!strcmp(var, "QuarantineFolderExt")) {
			free(option->QuarantineFolderPath);
			option->QuarantineFolderPath = StrDuplicate(val);
		} else if (!strcmp(var, "QuarantineLevel")) {
			option->QuarantineLevel = atoi(val);
		} else if (!strcmp(var, "Outbound")) {
			option->Outbound = atoi(val);
		} else if (!strcmp(var, "ReformatEndOfLine")) {
			option->ReformatNewLines = atoi(val);
		} else if (!strcmp(var, "Log")) {
			option->Log = atoi(val);
		}
	}
	fclose(cfil);

	if (strlen(option->XmailRoot) == 0)
		return (-1);

	return 0;

}
