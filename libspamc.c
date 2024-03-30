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
 *
 *	Part of this code is copyright 2001 by Craig Hughes									 
 *	It is licensed for use with SpamAssassin according									 
 *  to the terms of the Perl Artistic License											 
 *																						 
 *	I've modified this functions (taken from Spamassassin 2.0 spamc code)				 
 *  to make them work with Windows Sockets and handle better errors for filter logging.	 
 *																						 
 */

#include "includes.h"
#include "dep.h"
#include "libspamc.h"

#define SD_RECEIVE		   0x00
#define SD_SEND            0x01

/* set EXPANSION_ALLOWANCE to something more than might be
   added to a message in X-headers and the report template */
const int EXPANSION_ALLOWANCE = 16384;

/* Set the protocol version that this spamc speaks */
const char *PROTOCOL_VERSION="SPAMC/1.2";

int SAFE_FALLBACK = -1; /* default to on now - CRH */

/* Keep track of how much is stored in this buffer in case of failure later */
int amount_read = 0;

/* This guy has to be global so that if comms fails, we can passthru the message raw */
unsigned char *msg_buf = NULL;

static int file_read (int fd, unsigned char *buf, int min, int len);
static int sock_read (int fd, unsigned char *buf, int min, int len);
static int file_write (int fd, const unsigned char *buf, int len);
static int sock_write (int fd, const unsigned char *buf, int len);
static int dump_message(int in,int out);
static int send_message(int in,int out,char *username, int max_size);
static int read_message(int in, int out, int max_size);
//static int chk_host (const struct sockaddr *addr);
static int try_to_connect (const struct sockaddr *addr, int *sockptr);
static int randomize_hosts(int nhosts);

static int file_read (int fd, unsigned char *buf, int min, int len)
{

	int total;
	int thistime;

	for (total = 0; total < min; ) {
		thistime = read (fd,(char *) buf+total, len-total);

		if (thistime < 0) {
			return -1;
		} else if (thistime == 0) {
			/* EOF, but we didn't read the minimum.  return what we've read
			* so far and next read (if there is one) will return 0. */
			return total;
		}

		total += thistime;
	}

	return total;

}

static int sock_read (int fd, unsigned char *buf, int min, int len)
{

	int total;
	int thistime;

	for (total = 0; total < min; ) {
		thistime = recv (fd,(char *) buf+total, len-total,0);

		if (thistime < 0) {
			return -1;
		} 
		else if (thistime == 0) {
		/* EOF, but we didn't read the minimum.  return what we've read
		* so far and next read (if there is one) will return 0. */
			return total;
		}

		total += thistime;
	}

	return total;

}

static int file_write (int fd, const unsigned char *buf, int len)
{

	int total;
	int thistime;

	for (total = 0; total < len; ) {
		thistime = write (fd,(char *) buf+total, len-total);

		if (thistime < 0) {
		  return thistime;        /* always an error for writes */
		}
		total += thistime;
	}

	return total;

}

static int sock_write (int fd, const unsigned char *buf, int len)
{

	int total;
	int thistime;

	for (total = 0; total < len; ) {
		thistime = send (fd,(char *) buf+total, len-total,0);

		if (thistime < 0) {
			return thistime;        /* always an error for writes */
		}
		total += thistime;
	}

	return total;

}

static int dump_message(int in,int out)
{

	int bytes;
	unsigned char buf[8192];

	while((bytes=file_read(in, buf, 8192, 8192)) > 0) {
		if(bytes != file_write (out,buf,bytes)) {
			/* err writing dump */
			return EX_IOERR;
		}
	}

	return (0==bytes)?EX_OK:EX_IOERR;

}

static int send_message(int in,int out,char *username, int max_size)
{

	char *header_buf = NULL;
	int bytes,bytes2;
	int ret = EX_OK;
	
	if(NULL == (header_buf = malloc(1024))) {
		/* cannot allocate memory */
		shutdown(out,SD_SEND);
		return EX_OSERR_MEM;
	}

	/* Ok, now we'll read the message into the buffer up to the limit */
	/* Hmm, wonder if this'll just work ;) */
	if((bytes = file_read (in, msg_buf, max_size+1024, max_size+1024)) > max_size) {
		/* Message is too big, so return so we can dump the message back out */
		bytes2 = snprintf(header_buf,1024,"SKIP %s\r\nUser: %s\r\n\r\n",
						   PROTOCOL_VERSION, username);
		sock_write (out,header_buf,bytes2);
		ret = EX_MSG_BIG;
	} 
	else {
	/* First send header */
		bytes2 = snprintf(header_buf,1024,"PROCESS %s\r\nContent-length: %d\r\nUser: %s\r\n\r\n",
						   PROTOCOL_VERSION,bytes,username);
		sock_write (out,header_buf,bytes2);
		sock_write (out,msg_buf,bytes);
	}
	free(header_buf);

	amount_read = bytes;
	shutdown(out,SD_SEND);
	return ret;

}

static int read_message(int in, int out, int max_size)
{

	int bytes;
	int header_read=0;
	char buf[8192];
	float version = 0;
	int response=EX_OK;
	char* out_buf;
	int out_index=0;
	int expected_length=0;

	//out_buf = malloc(max_size+EXPANSION_ALLOWANCE);

	if(NULL == (out_buf = malloc(max_size+EXPANSION_ALLOWANCE))) {
		/* cannot allocate memory */
		shutdown(in,SD_RECEIVE);
		return EX_OSERR_MEM;
	}

	for(bytes=0;bytes<8192;bytes++) {
		if(recv(in,&buf[bytes],1,0) == 0) { /* read header one byte at a time */
			/* Read to end of message!  Must be because this is version <1.0 server */
			if(bytes < 100) {
				/* No, this wasn't a <1.0 server, it's a comms break! */
				response = EX_SERV;
			}
			/* No need to copy buf to out_buf here, because since header_read is unset that'll happen below */
			break;
		}

		if('\n' == buf[bytes]) {
			buf[bytes] = '\0';	/* terminate the string */

			if(2 != sscanf(buf,"SPAMD/%f %d %*s",&version,&response)) {
				//syslog (LOG_ERR, "spamd responded with bad string '%s'", buf);
				response = EX_PROTOCOL; 
				break;
			}
			header_read = -1; /* Set flag to show we found a header */
			break;
		}
	}

	if(!header_read && EX_OK == response) {
		/* We never received a header, so it's a message with version <1.0 server */
		memcpy(&out_buf[out_index], buf, bytes);
		out_index += bytes;
		/* Now we'll fall into the while loop if there's more message left. */
	}
	else if(header_read && EX_OK == response) {
		/* Now if the header was 1.1, we need to pick up the content-length field */
		if(version - 1.0 > 0.01) { /* Do this for any version higher than 1.0 [and beware of float rounding errors]*/
			for(bytes=0;bytes<8192;bytes++) {
				if(recv(in,&buf[bytes],1,0) == 0) { /* keep reading one byte at a time */
					/* Read to end of message, but shouldn't have! */
					response = EX_PROTOCOL; break;
				}
				if('\n' == buf[bytes]) {
					/* Ok, found a header line, it better be content-length */
					if(1 != sscanf(buf,"Content-length: %d",&expected_length)) {
						/* Something's wrong, so bail */
						response = EX_PROTOCOL; break;
					}

					/* Ok, got here means we just read the content-length.  Now suck up the header/body separator.. */
					if(sock_read (in,buf,2,2) != 2 || !('\r' == buf[0] && '\n' == buf[1])) {
						/* Oops, bail */
						response = EX_PROTOCOL; break;
					}

					/* header done being sucked, let's get out of this inner-for */
					break;
				} /* if EOL */
			} /* for loop to read subsequent header lines */
		}
	}

	if(EX_OK == response) {
		while((bytes=sock_read (in,buf,8192, 8192)) > 0) {
			if (out_index+bytes >= max_size+EXPANSION_ALLOWANCE) {
				//	syslog (LOG_ERR, "spamd expanded message to more than %d bytes",
				//		max_size+EXPANSION_ALLOWANCE);
				response = EX_MSG_BIG;
				break;
			}
			memcpy(&out_buf[out_index], buf, bytes);
			out_index += bytes;
		}
	}

	shutdown(in,SD_RECEIVE);

	if (EX_OK == response) {
		/* Check the content length for sanity */
		if(expected_length && (expected_length != out_index)) {
		//      syslog (LOG_ERR, "failed sanity check, %d bytes claimed, %d bytes seen",
		//	      expected_length, out_index);
			response = EX_MSG_MIS;
		}
		else {
			file_write (out, out_buf, out_index);
		}
	}

	free(out_buf);

	return response;

}
/*
static int chk_host (const struct sockaddr *addr) 
{
	
	int mysock;
	int origerr;
	int iConn;
	int iMode;
	fd_set wfds;
	struct timeval tv;

	if(-1 == (mysock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))) {
		origerr = Get_Socket_Error();	
		return origerr;
	}

	iMode = 1;
	ioctlsocket(mysock, FIONBIO, (u_long FAR*) &iMode);

	if(connect(mysock,(const struct sockaddr *) addr, sizeof(*addr)) == SOCKET_ERROR) {
		
		origerr = Get_Socket_Error();

		// Use 1 sec timeout to avoid default connect timeout (usally 20 sec)
		if (origerr == WSAEWOULDBLOCK) {

			tv.tv_sec = 1;
			tv.tv_usec = 0;
			FD_ZERO(&wfds);
			FD_SET(mysock,&wfds);

			iConn = select(0,NULL,&wfds,NULL,&tv);
			origerr = Get_Socket_Error();
			iMode = 0;
			ioctlsocket(mysock, FIONBIO, (u_long FAR*) &iMode);
			closesocket(mysock);

			if (iConn == SOCKET_ERROR)
				return origerr;
			else if (iConn == 0)
				return EX_TIMEOUT;

			return EX_OK;

		}
		else
			return origerr;
	}

	return EX_OK;

}
*/
static int try_to_connect (const struct sockaddr *addr, int *sockptr)
{

	int mysock;
	int origerr;
	//int i;
	int iMode = 0;
	int bOptVal = 1;

	//i = chk_host(addr);
	//if (i != 0)
	//	return i;

	if(-1 == (mysock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))) {
		origerr = Get_Socket_Error();	
		return origerr;
	}

	if(setsockopt(mysock, IPPROTO_TCP, TCP_NODELAY, &bOptVal, sizeof(bOptVal)) < 0) {
		origerr = Get_Socket_Error();
		return origerr;
	}

	if(connect(mysock,(const struct sockaddr *) addr, sizeof(*addr)) < 0) {
		origerr = Get_Socket_Error();
		return origerr;
	}

	*sockptr = mysock;

	return EX_OK;

}

int process_message(int hFile_in, int hFile_out, char *hostname, unsigned short port, char *username, 
					unsigned int max_size, char *usedhost)
{

	int exstatus = 1;
	int mysock;
	struct sockaddr_in addr;
	struct hostent *hent;
	int stop = 0;
	int nhosts = 0, nhost;
	char *pch;

	Init_Socket();
	
	// Check if we have more than one hostname, if so fill the hosts struct
	if(strchr(hostname,';') != NULL) {
		
		pch = strtok(hostname,";");
		while (pch != NULL) {

			if (nhosts > MAX_HOSTS)
				break;

			hostname_t[nhosts].host = StrDuplicate(pch);

			nhosts++;

			pch = strtok (NULL,";");

		}

		do {

			nhost = randomize_hosts(nhosts);

			hent = gethostbyname(hostname_t[nhost].host);
			memcpy ((char *)&addr.sin_addr,(char *)hent->h_addr, hent->h_length);
			addr.sin_family = hent->h_addrtype;
			addr.sin_port = htons(port);
				
			exstatus = try_to_connect ((const struct sockaddr *) &addr, &mysock);
			if (exstatus == EX_OK) {
				break;
			}

			stop++;

		} while (stop <= MAX_RETRIES);

		strcpy(usedhost,hostname_t[nhost].host);

		for (stop = 0; stop < MAX_HOSTS; stop++)
			free(hostname_t[stop].host);

	}
	else {

		hent = gethostbyname(hostname);
		memcpy ((char *)&addr.sin_addr,(char *)hent->h_addr, hent->h_length);
		addr.sin_family = hent->h_addrtype;
		addr.sin_port = htons(port);

		// retry up to MAX_RETRIES if connect fails
		do {
			
			exstatus = try_to_connect ((const struct sockaddr *) &addr, &mysock);

			if (exstatus == EX_OK)
				break;

			stop++;

		} while (stop <= MAX_RETRIES);

		strcpy(usedhost,hostname);

	}

	if (EX_OK == exstatus) {
		/* Cannot allocate memory */
		if(NULL == (msg_buf = malloc(max_size+1024))) { 
			Cleanup_Socket();
			return EX_OSERR_MEM;
		}

		exstatus = send_message(hFile_in,mysock,username,max_size);

		/* send_message returened error */
		if (EX_OK != exstatus) {
			Cleanup_Socket();
			return exstatus;
		}
		/* send_message returened ok */
		if (EX_OK == exstatus) {
			exstatus = read_message(mysock,hFile_out,max_size);
			/* read_message returened error */
			if (EX_OK != exstatus) {
				Cleanup_Socket();
				return exstatus;
			}
		}
		/* read_message returened error Message was too big or corrupted, so dump the buffer then bail */
		if(EX_MSG_BIG == exstatus || (SAFE_FALLBACK && EX_OK != exstatus)) {
			file_write (hFile_out,msg_buf,amount_read);
			dump_message(hFile_in,hFile_out);
			/* Message was too big (shuld not be) */
			if (EX_MSG_BIG == exstatus) {
				exstatus = EX_MSG_BIG;
			}
			/* Message was corrupted */
			if (SAFE_FALLBACK && EX_OK != exstatus) {
				exstatus = EX_PROTOCOL;
			}
		}
		free(msg_buf);
	}
	/* try_to_connect failed, but SAFE_FALLBACK set then dump original message */
	else if(SAFE_FALLBACK) { 
		if(amount_read > 0) {
			file_write(hFile_out,msg_buf,amount_read);
		}
		
		dump_message(hFile_in,hFile_out);
	}

	Cleanup_Socket();

	return exstatus;	/* return the last failure code */

}

static int randomize_hosts(int nhosts)
{

    srand ( time(NULL) );
	return (rand() % nhosts);

}
