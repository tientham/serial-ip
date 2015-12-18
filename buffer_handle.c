/*
 * buffer_handle.c
 *	This code is to handle buffer. Throughout the software, there are 3 buffer: sabre to socker buf; socket to serial buf; serial to socket buf.
 *  Created on: Aug 8, 2015
 *      Author: tientham
 */


#include "serial_ip.h"


/*
	Location: buffer_handle.c
	This is to insert char ch into buffer at ptr
	returns 1 on success, 0 on failure
*/
int bfinsch(BUFFER *buff, unsigned char *ptr, unsigned char ch)
{
	unsigned char *s;

	if (buff == NULL) return(0);
	if (ptr == NULL) return(0);

	/* make sure ptr is within the active portion of the buffer */
	if ((buff->writep <= ptr) && (ptr <= buff->readp)) {
		/* move the chars between ptr and readp, 1 place to the right */
		s = buff->readp - 1;
		while (s >= ptr) {
			*(s+1) = *s;
			s--;
		}
		*ptr = ch;
		buffer_readpointer_position(buff,1);
		return(1);
	} else {
		return(0);
	}
}

/*
	Location: buffer_handle.c
	This function is to search a specific character in the given buffer.
	this version can handle embedded nulls.
*/
unsigned char *bfstrchr(BUFFER *buff, unsigned char ch)
{
	unsigned char *s;
	int n;

	if (buff == NULL) return(NULL);

	s = buff->writep;
	n = buff->nbuffered;
	while ((n > 0) && (*s != ch)) {
		s++;
		n--;
	}
	if (*s == ch) {
		return(s);
	} else {
		return(NULL);
	}
}
/*
	Location: buffer_handle.c
	This is to remove nbytes characters beginning at ptr from the buffer
	Returns the number of characters removed.
*/
int bfrmstr(BUFFER *buff, unsigned char *ptr, int nbytes)
{
	unsigned char *s;

	if (buff == NULL) return(0);
	if (ptr == NULL) return(0);

	/* make sure ptr is within the active portion of the buffer */
	if ((buff->writep <= ptr) && (ptr <= buff->readp)) {
		if (nbytes >= buff->nbuffered) {	/* remove all chars */
			bfinit(buff);
			return(nbytes);
		}
		/* if caller asked to remove too much ... */
		if (nbytes > (buff->readp - ptr)) {
			nbytes = buff->readp - ptr;
		}
		/* remove nbytes characters between ptr and readp */
		s = ptr + nbytes;
		while (s < buff->readp) {
			*ptr++ = *s++;					/* move chars to the left */
		}
		buffer_readpointer_position(buff,(nbytes * -1));
		return(nbytes);
	} else {
		return(0);
	}
}

/*
	Location: buffer_handle.c
	This is to get number of bytes of active portion in the given buffer.
*/
int bf_get_nbytes_active(BUFFER *buff)
{
	if (buff == NULL) return(0);

	return(buff->nbuffered);
}

/*
	Location: buffer_handle.c
	This is to point to a active portion of buffer, and set its size to nbuffered field in the given buffer.
*/
unsigned char *bf_point_to_active_portion(BUFFER *buff, int *amount)
{
	if (buff == NULL) return(NULL);

	if (amount != NULL)
	{
		*amount = buff->nbuffered;
	}
	return(buff->writep);
}


/*
	Location: buffer_handle.c
	This is to search for a string in the given buffer.
*/
unsigned char *bfstrstr(BUFFER *buff, unsigned char *str)
{
	unsigned char *s;

	if (buff == NULL) return(NULL);
	if (str == NULL) return(NULL);

	s = (unsigned char *) strstr((char *) buff->writep,(char *) str);
	if ((s != NULL) && (buff->writep <= s) && (s <= buff->readp)) {
		return(s);
	} else {
		return(NULL);
	}
}

/*
	Location: buffer_handle.c
	This is to get eof flag in the given buffer.
*/
int bfeof(BUFFER *buff)
{
	if (buff == NULL) return(0);

	return(buff->eof);
}

/*
	Location: buffer_handle.c
	This is to calculate the number of bytes we could read into the buffer
*/
int cal_numbytes_to_read(BUFFER *buff)
{
	int bytes;

	if (buff == NULL) return(0);

	if (buff->readp > buff->writep) {
		bytes = buff->tailp - buff->readp;
	} else if (buff->readp < buff->writep) {
		bytes = buff->writep - buff->readp;
	} else {
		/* (buff->readp == buff->writep) */
		if (buff->nbuffered == 0) {			/* buffer is empty */
			bytes = buff->size;
		} else {							/* buffer is full */
			bytes = 0;
		}
	}
	if (bytes <= 0) {
		if (bytes == 0)
		{
			write_to_debuglog(DBG_INF,"buffer_handle.c: cal_ntr(): buffer \"%s\" is full",
					buff->label);
		} else {
			write_to_debuglog(DBG_ERR,"buffer_handle.c: cal_ntr(): bfread() buffer \"%s\" is corrupt: bytes=%d",
					buff->label,bytes);
		}
	}
	return(bytes);
}

/*
 	 Location: buffer_handle.c
 	 This is to read content from file descriptor to a specific buffer.
*/
int read_from_fd_to_buffer(int fd, BUFFER *buff)
{
	extern int errno;
	int bytes;
	int n;

	if (buff == NULL) return(0);

	bytes = cal_numbytes_to_read(buff);
	if (bytes <= 0)
		return(bytes);
	/* Read from file to buffer. */
	n = read(fd, buff->readp, bytes);
	if (n < 0) {
		debug_perror("buffer_handle.c: r_f_ftb()");
		if ((errno != EINTR) && (errno != EAGAIN)) {
			return(n);
		} else {
			return(0);
		}
	} else if (n == 0) {
		write_to_debuglog(DBG_INF,"buffer_handle.c: r_f_ftb(): eof on fd %d",fd);
		buff->eof = 1;
		return(0);
	}
	/* else (n > 0) */
	write_to_debuglog(DBG_VINF,"buffer_handle.c: r_f_ftb(): read(%d,readp,%d) = %d",fd,bytes,n);
	buffer_readpointer_position(buff, n);
	write_to_debuglog(DBG_VINF,"buffer_handle.c: r_f_ftb():  nbuffered=%d",buff->nbuffered);
	return(n);
}

/*
		Location: buffer_handle.c
		This is to set write pointer position a given in buffer struct.
 */

void buffer_writepointer_position(BUFFER *buff, int nbytes)
{
	if (buff == NULL) return;

	buff->nbuffered -= nbytes;
	if (buff->nbuffered > 0) {
		buff->writep += nbytes;
	} else {
		bfinit(buff);
	}
}

/*
	Location: buffer_handle.c
	This is to calculate the number of bytes we could write from the buffer
*/
int cal_numbytes_to_write(BUFFER *buff)
{
	int bytes;

	if (buff == NULL) return(0);

	if (buff->readp > buff->writep) {
		bytes = buff->readp - buff->writep;
	} else if (buff->readp < buff->writep) {
		bytes = buff->tailp - buff->writep;
	} else {
		/* (buff->readp == buff->writep) */
		if (buff->nbuffered == 0) {			/* buffer is empty */
			bytes = 0;
		} else {							/* buffer is full */
			bytes = buff->tailp - buff->writep;
		}
	}
	if (bytes <= 0) {
		if (bytes == 0) {
			;	/* too common, don't log it */
			/* debug(DBG_INF,"bfwrite() buffer \"%s\" is empty",buff->label); */
		} else {
			write_to_debuglog(DBG_ERR,"buffer_handle.c: cal_nw(): buffer \"%s\" is corrupt: bytes=%d",
					buff->label, bytes);
		}
	}
	return(bytes);
}

/*
	Location: buffer_handle.c
	This is to write the content from buffer to file descriptor.
	Remember: there are 2 file descripters
	- network fd.
	- serial fd.
*/

int write_from_buffer_to_fd(int fd, BUFFER *buff)
{
	extern int errno;
	int bytes;
	int n;

	if (buff == NULL) return(0);

	bytes = cal_numbytes_to_write(buff);
	if (bytes <= 0)
		return(bytes);

	n = write(fd, buff->writep, bytes);
	if (n < 0) {
		debug_perror("bfwrite()");
		if ((errno != EINTR) && (errno != EAGAIN)) {
			return(n);
		} else {
			return(0);
		}
	} else if (n == 0) {
		write_to_debuglog(DBG_INF,"buffer_handle.c: write_bfd(): write() returned 0");
		return(0);
	}
	/* else (n > 0) */
	write_to_debuglog(DBG_VINF,"buffer_handle.c: write_bfd(): write(%d,writep,%d) = %d", fd, bytes, n);
	buffer_writepointer_position(buff,n);
	write_to_debuglog(DBG_VINF,"buffer_handle.c: write_bfd(): nbuffered=%d",buff->nbuffered);
	return(n);
}



/*
	Location: buffer_handle.c
	This is to set the read pointer (readp) position in buffer.
*/

void buffer_readpointer_position(BUFFER *buff, int nbytes)
{
	if (buff == NULL) return;

	buff->nbuffered += nbytes;
	buff->readp += nbytes;
}


/*
	Location: buffer_handle.c
	This function is to write full string to buffer.
	returns the number of characters that were appended to the buffer
*/
int bfstrcat(BUFFER *buff, const char *command)
{
	int len;

	if (buff == NULL) return(0);
	if (command == NULL) return(0);

	len = strlen((char *) command);
	if (len > (buff->size - buff->nbuffered)) {
		len = buff->size - buff->nbuffered;		/* truncate */
	}
	memcpy(buff->readp, command, len);
	buffer_readpointer_position(buff, len);
	return(len);
}

/*
	Location: buffer_handle.c
	This function is to write up to nbytes of command to buffer.
	returns the number of characters that were appended to the buffer
*/
int bfstrncat(BUFFER *buff, const char *command, int nbytes)
{
	int len;

	if (buff == NULL) return(0);
	if (command == NULL) return(0);

	len = nbytes;
	if (len > (buff->size - buff->nbuffered)) {
		len = buff->size - buff->nbuffered;		/* truncate */
	}
	memcpy(buff->readp, command, len);
	buffer_readpointer_position(buff, len);
	return(len);
}


/*
		Location: buffer_handle.c
		In case of data give into buffer is big, this is to dump buffer
		into lines with defined bytes (16 bytes - a chunk.).

		There are 2 styles being to use syslog or debug log.
		If we use syslog, debug fp will be NULL => wrtie log message to systemlog.
		Otherwise, write to debug log.
 */

void memdump(char *buf, int len, FILE *stream)
{
	int bytes;							/* bytes on this line */

	if (buf == NULL) return;

	for ( ; len > 0; len -= CHUNK, buf += CHUNK)
	{
		bytes = (len >= CHUNK) ? CHUNK : len;
		if (stream != NULL)
		{
			debug_linedump(buf, bytes, stream);
			if (ferror(stream)) break;
		} else {
			syslog_linedump(buf, bytes);
		}
	}
}

/*
 	 Location: buffer_handle.c
 	 This is to write log messages from buffer to debug file.
 	 If opt = 1, we print detail buffer information.
 */

void bfdump(BUFFER *buff,int opt)
{
	extern FILE *debugfp;
	extern int debug_level;
	FILE *fp;
	int level;

	if (buff == NULL) return;

	if (opt) {
		write_to_debuglog(DBG_VINF, "dumping BUFFER object \"%s\"", buff->label);
		write_to_debuglog(DBG_VINF, "  buffer size is %d", buff->size);
		write_to_debuglog(DBG_VINF, "  buffer begins at %p", buff->buffp);
		write_to_debuglog(DBG_VINF, "  buffer ends at %p", buff->tailp);
		write_to_debuglog(DBG_VINF, "  buffer read ptr is at %p", buff->readp);
		write_to_debuglog(DBG_VINF, "  buffer write ptr is at %p",buff->writep);
		write_to_debuglog(DBG_VINF, "  buffer contains %d bytes", buff->nbuffered);
	} else {
		write_to_debuglog(DBG_INF,"buffer \"%s\" contains %d bytes",buff->label,buff->nbuffered);
	}
	if (buff->nbuffered > 0) {
		fp = debugfp;
		level = debug_level;
		if (level >= DBG_LV5) {
			memdump((char *) buff->writep, buff->nbuffered, fp);
		}
	}
}


/*
 	 Location: buffer_handle.c
 	 This is to free buffer.
 	 Free: buff-> label and buff->buffp.
 */

void bffree(BUFFER *buff)
{
	if (buff == NULL) return;

	if (buff->label != NULL)
		free(buff->label);
	if (buff->buffp != NULL)
		free(buff->buffp);
	free(buff);
}


/*
 	 Location: buffer_handle.c
 	 This is to initialize buffer.
 	 Set: readp, writep to the begin of buffer.
 	 tailp points to the end of buffer.
 	 number bytes in buffer = 0
 */

void bfinit(BUFFER *buff)
{
	if (buff == NULL) return;

	buff->readp = buff->buffp;
	buff->writep = buff->buffp;
	buff->tailp = buff->buffp + buff->size;
	buff->nbuffered = 0;
	memset(buff->buffp,(int) '\0',buff->size);
}



/*
 	 Location: buffer_handle.c

 	 Malloccing buffer.
 */

BUFFER *bfmalloc(char *label, int size)
{
	struct buffer_t *buff;

	buff = calloc(1,sizeof(struct buffer_t));
	if (buff == NULL) {
		debug_perror("buffer_handle.c: bfmalloc()");
		return(NULL);
	}
	if (size > 0) {
		buff->size = size;
	} else {
		buff->size = BUFSIZ;
	}
	buff->buffp = calloc(1,(buff->size + 1));
	if (buff->buffp == NULL) {
		debug_perror("buffer_handle.c: bfmalloc()");
		free(buff);
		return(NULL);
	}
	if (label != NULL) {
		buff->label = (unsigned char *) strdup(label);
	} else {
		buff->label = (unsigned char *) strdup("");
	}
	if (buff->label == NULL) {
		debug_perror("buffer_handle.c: bfmalloc()");
		free(buff->buffp);
		free(buff);
		return(NULL);
	}
	buff->eof = 0;
	bfinit(buff);
	return(buff);
}






