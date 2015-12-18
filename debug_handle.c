/*
 * debug_handle.c
 *	This is to set up debug level and handle debug functions.
 *  Created on: Aug 6, 2015
 *      Author: tientham
 */


#include "serial_ip.h"


char hostname[MAXHOSTNAME_LEN+1];
char *myprogname = NULL;
FILE *debugfp = NULL;
int debug_level = 0;			/* disabled */

/*
	Location: debug_handle.c
	This function is to dump a line for the buffer then write it to debug file.
*/
void debug_linedump(char *buf, int len, FILE *stream)
{
	int i;								/* gp int */
	char *p;							/* buf ptr */

	if ((stream == NULL) || (buf == NULL))
		return;
	if (len < 1) return;

	p = buf;							/* point to buffer */
	for (i = 0; i < len; i++,p++)		/* output hex part of dump */
		fprintf(stream,"%02x ", (unsigned char) *p);	/*%02x prints at least 2 digits, if there is 1 digit input, append 0 before.*/

	for (i = len+1; i <= CHUNK; i++)	/* pad w/spaces if (len < CHUNK) */
		fprintf(stream,"   ");

	fprintf(stream,"   ");				/* add spaces for separation */

	p = buf;							/* point to buffer */
	for (i = 0; i < len; i++,p++)		/* output character part of dump */
		fprintf(stream,"%c",isprint(*p) ? *p : '.');

	fprintf(stream,"\n");				/* add newline */

	if (! ferror(stream))
		fflush(stream);
}

/*
	Location: debug_handle.c
	This is to write the error message for the current system error to the debug log.
*/
void debug_perror(char *str)
{
	extern int errno;

	if (errno > 0) {
		if ((str == NULL) || (*str == '\0')) {
			write_to_debuglog(DBG_ERR,"%s",strerror(errno));
		} else {
			write_to_debuglog(DBG_ERR,"%s: %s",str,strerror(errno));
		}
	}
}

/*
	Location: debug_handle.c
	This is to open the debug log in append mode.
	returns the stream pointer or NULL on error.
*/
FILE *open_debug(char *file, int level, char *program)
{
	extern char hostname[];
	extern FILE *debugfp;
	extern char *myprogname;
	extern int debug_level;
	FILE *fp;
	//time_t seconds;
	char *p;

	if ((file != NULL) && (*file != '\0')) {
		fp = fopen(file,"a");
		if (fp != NULL) {
			debugfp = fp;				/* save in global variable */
		}
	} else {
		debugfp = fp = NULL;			/* direct debug output to syslog */
	}

	debug_level = level;					/* save these in global variables */
	myprogname = program;
	/*
		get our host name and truncate it at the first '.'
	*/
	if (gethostname(hostname,MAXHOSTNAME_LEN) == 0) {
		if ((p = strchr(hostname,'.')) != NULL) {
			*p = '\0';
		}
	} else {
		strcpy(hostname,"localhost");
	}
	return(fp);
}

/*
	Location: debug_handle.c
	Depend on the serial_ip.conf, we can choose 1 of 2 style for systemlog with syslog() or the debug log
	This function doesnot return.
*/
void write_to_debuglog(int level, char *fmt, ...)
{
	extern char hostname[];
	extern FILE *debugfp;
	extern char *myprogname;
	extern int debug_level;
	static char str[1024];
	va_list args;

	/* sanity checks */
	if (level > debug_level) return;
	if (fmt == NULL) return;

	va_start(args,fmt);
	/* debug pointer # NULL when the server has already run.*/
	if (debugfp != NULL) {
		fprintf(debugfp,"%s %s %s[%d]: ", get_timestamp(),hostname, myprogname, getpid());
		vfprintf(debugfp, fmt, args);
		fputc('\n', debugfp);
		if (! ferror(debugfp)) {
			fflush(debugfp);
		} else {
			close_debug();
		}
	} else {
		/*Particularly using for the inizialization period.*/
		vsnprintf(str, sizeof(str), fmt, args);
		if (level == DBG_ERR) {
			syslog(LOG_ERR,"%s",str);
		} else if (level <= DBG_INF) {
			syslog(LOG_INFO,"%s",str);
		} else {
			syslog(LOG_DEBUG,"%s",str);
		}
	}
	va_end(args);
}

/*
  Location: debug_handle.c
  This function is close debug log.
  Non return.
*/
void close_debug(void)
{
	extern FILE *debugfp;
	extern char *myprogname;
	extern int debug_level;

	if (debugfp != NULL) {
		fclose(debugfp);
		debugfp = NULL;
	}
	debug_level = 0;
	myprogname = NULL;
}

/*
  Location: debug_handle.c
  This function is set level for debug mode.
  Non return.
*/
void set_debug_level(int new_level)
{
	extern int debug_level;

	if ((new_level >= DBG_LV0) && (new_level <= DBG_LV9)) {
		debug_level = new_level;
	}
}

/*
  Location: debug_handle.c
  This function is get level for debug mode.
  Non return.
*/
int get_debug_level()
{
	extern int debug_level;

	return(debug_level);
}
