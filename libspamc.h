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

/* Exit Codes */

#ifndef _LIBSPAMC_H
#define _LIBSPAMC_H

#define EX_OK				  0      /** successful termination */
#define EX_SOFTWARE			 70      /** internal software error */
#define EX_OSERR_MEM		 71      /** system error (cannot allocate memory) */
#define EX_IOERR			 72      /** input/output error in dump */
#define EX_PROTOCOL			 73      /** remote error in protocol */
#define EX_MSG_BIG			 74		 /** message is too big */
#define EX_MSG_MIS			 75		 /** message is diffent from content-lenght */
#define EX_SERV				 76		 /** Spamd Version Not Supported */
#define EX_TIMEOUT			 77		 /** Socket Timeout with select()*/

#define MAX_RETRIES 5
#define MAX_HOSTS 5

struct s_hostname {

	char *host;

} hostname_t[MAX_HOSTS];

int process_message(int hFile_in, int hFile_out, char *hostname, unsigned short port, 
					char *username, unsigned int max_size, char *usedhost);

#endif
