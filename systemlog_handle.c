/*
 * systemlog_handle.c
 *	This is to handle system log.
 *  Created on: Aug 7, 2015
 *      Author: tientham
 */


#include "serial_ip.h"

/*
	Location: systemlog_handle.c
	This function is to dump a line for the buffer then write it to debug file.
*/
char *slinedump(char *str,char *buf,int len)
{
	int i;								/* gp int */
	char *p;							/* buf ptr */
	char *s;							/* str ptr */

	if (str == NULL) return(NULL);
	if (buf == NULL) return(NULL);
	if (len < 1) return(NULL);
	if (len > CHUNK) len = CHUNK;		/* trim */

	s = str;							/* init str ptr */
	p = buf;							/* point to buffer */
	for (i = 0; i < len; i++,p++) {		/* output hex part of dump */
		sprintf(s,"%02x ",(unsigned char) *p);
		s += 3;
	}

	for (i = len+1; i <= CHUNK; i++) {	/* pad w/spaces if (len < CHUNK) */
		sprintf(s,"   ");
		s += 3;
	}

	sprintf(s,"   ");					/* add spaces for separation */
	s += 3;

	p = buf;							/* point to buffer */
	for (i = 0; i < len; i++,p++) {		/* output character part of dump */
		sprintf(s,"%c",isprint(*p) ? *p : '.');
		s++;
	}

	/* null terminate str */
	*s = '\0';

	return(str);
}

/*
	Location: systemlog_handle.c
	Depend on the serial_ip.conf, we can choose 1 of 2 style for systemlog with syslog() or the debug log
	This function doesnot return.
*/
void syslog_linedump(char *buf, int len)
{
	int i;								/* gp int */
	char *p;							/* buf ptr */
	char *str;							/* the string we'll create */

	if (buf == NULL) return;
	if (len < 1) return;
	if (len > CHUNK) len = CHUNK;		/* trim */

	/* allocate memory for string */
	str = malloc((CHUNK * 4) + 16);
	if (str == NULL) return;

	/* get formatted dump */
	slinedump(str, buf, len);

	/* send it to syslog */
	syslog(LOG_DEBUG, "%s", str);

	/* release allocated memory */
	free(str);
}

/*
	Location: systemlog_handle.c
	Depend on the serial_ip.conf, we can choose 1 of 2 style for systemlog with syslog() or the debug log
	This function doesnot return.
*/
void write_to_systemlog(int level, char *fmt, ...)
{
	static char str[1024];
	va_list args;

	/* sanity checks */
	if (fmt == NULL) return;

	va_start(args,fmt);
	vsnprintf(str,sizeof(str),fmt,args);

	if (level == LOG_ERR)
	{
		write_to_debuglog(DBG_ERR,"%s",str);
	} else if (level == LOG_INFO)
	{
		write_to_debuglog(DBG_INF,"%s",str);
	} else {
		write_to_debuglog(DBG_VINF,"%s",str);
	}
	va_end(args);
}

/*
  	  Location: systemlog_handle.c
  	  This function is to write error message from Standard Error Stream to systemlog (syslog).
  	  There is no return value.
*/
void system_errorlog(char *str)
{
	extern int errno;

	if (errno > 0) {
		if ((str == NULL) || (*str == '\0')) {
			syslog(LOG_ERR,"%s",strerror(errno));
		} else {
			syslog(LOG_ERR,"%s: %s",str,strerror(errno));
		}
	}
}
