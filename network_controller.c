/*
 * network_controller.c
 *	This function is to handle data flow inside sabre lite.
 *  Created on: Aug 7, 2015
 *      Author: tientham
 */

#include "serial_ip.h"


/*	Location: network_controller.c
	This is to write the content from serial_to_socket_buf to the socket file descriptor.
	returns 0 on success, 1 on failure	*/
int write_socket(int sockfd, BUFFER *serial_to_socket_buf)
{
	extern int session_state;
	int n;
	int ret;

	if (session_state == SUSPEND)
		return(0);
	n = write_from_buffer_to_fd(sockfd, serial_to_socket_buf);
	if (n < 0) {											/* error on write */
		ret = 1;
	} else if (n == 0) {									/* no data written */
		ret = 0;
	} else {												/* some data was written */
		ret = 0;
		bfdump(serial_to_socket_buf, 0);					/* debugging dump of BUFFER */
	}
	return(ret);
}

/*	Location: network_controller.c
	This is to read data from serial_file_descriptor, and put its content into serial_to_socket buffer.
	returns 0 on success, 1 on failure (including EOF)	*/
int read_serial(int serial_file_descriptor, BUFFER *serial_to_socket_buf)
{
	extern unsigned char linestate;
	extern int noquote;														/* don't quote IAC char */
	extern int useconds;													/* i/o wait time */
	int n;
	int ret;

	/* wait a bit before reading the serial port */
	if (useconds > 0) {
		msleep(useconds);
	}
	n = read_from_fd_to_buffer(serial_file_descriptor, serial_to_socket_buf);
	if (n < 0) {															/* error on read */
		ret = 1;
	} else if (n == 0) {													/* no data read or EOF */
		if (bfeof(serial_to_socket_buf)) {
			ret = 1;														/* serial EOF */
			write_to_debuglog(DBG_INF,"network_controller.c: read_serial(): read EOF on modem fd %d", serial_file_descriptor);
		} else {
			ret = 0;
		}
	} else {																/* some data was read */
		ret = 0;
		linestate |= CPC_LINESTATE_DATA_READY;								/* Enable flag of data ready => can read data.*/
		bfdump(serial_to_socket_buf,0);										/* debugging dump of BUFFER */

		/* check for IAC char unless 'noquote' option was enabled */
		if (! noquote) {
			if (bfstrchr(serial_to_socket_buf, IAC) != NULL) {			/* we detect IAC chars in the serial_to_sock buf for escaping them.*/
				escape_iac_chars(serial_to_socket_buf);
				bfdump(serial_to_socket_buf,0);								/* debugging dump of BUFFER */
			}
		}
	}
	return(ret);
}


/*	Location: network_controller.c
	This is to write content from socket_to_serial_buf to the serial port file descriptor.
	Returns 0 on success, 1 on failure		*/
int write_serial(int serial_file_descriptor, BUFFER *socket_to_serial_buf)
{
	int n;
	int ret;

	n = write_from_buffer_to_fd(serial_file_descriptor, socket_to_serial_buf);
	if (n < 0)														/* error on write */
	{
		ret = 1;
	} else if (n == 0) {											/* no data written */
		ret = 0;
	} else {														/* some data was written */
		ret = 0;
		if (bf_get_nbytes_active(socket_to_serial_buf) == 0)		/* no data left in buffer */
			//linestate &= ~CPC_LINESTATE_DATA_READY;					/* Disable flag of data ready => can not receive anymore.*/
		bfdump(socket_to_serial_buf, 0);							/* debugging dump of BUFFER */
	}
	return(ret);
}

/*	Location: network_controller.c
	This is to read from the networking socket and write content into socket_to_serial buffer.
	returns 0 on success, 1 on failure (including EOF)	*/
int read_socket(int sockfd, int serial_file_descriptor, BUFFER *socket_to_serial_buf, BUFFER *sabre_to_socket_buf)
{
	extern int useconds;			/* i/o wait time */
	extern int raw_flag;			/* is raw TCP gateway?*/
	int n;
	int ret;

	/* wait a bit before reading the socket */
	if (useconds > 0) {
		msleep(useconds);
	}

	if(!raw_flag)
		n = read_from_fd_to_buffer(sockfd, socket_to_serial_buf);

	if (n < 0) {													/* error on read */
		ret = 1;
	} else if (n == 0) {											/* no data read or EOF */
		if (bfeof(socket_to_serial_buf))
		{
			ret = 1;												/* socket EOF */
			write_to_debuglog(DBG_INF, "network_controller.c: read_socket(): read EOF on socket fd %d", sockfd);
		} else {
			ret = 0;												/* probably interrupted by a signal */
		}
	} else {														/* some data was read */
		ret = 0;
		bfdump(socket_to_serial_buf, 0);							/* debugging dump of BUFFER */
		if(!raw_flag)
		{
			/* check for IAC char */
			if (bfstrchr(socket_to_serial_buf, IAC) != NULL)
			{
				process_telnet_options(sockfd, serial_file_descriptor, socket_to_serial_buf, sabre_to_socket_buf);
				bfdump(socket_to_serial_buf, 0);							/* debugging dump of BUFFER */
			}
		}
	}
	return(ret);
}

/*	Location: network_controller.c
	This function deals with networking socket file descriptor setting.
	It is to:
	- sets socket options: keepalive, blockopt mode.
	If blockopt = BLOCK => we set non-blocking flag (O_NONBLOCK) is off, then if a process tries to
	perform an incompatible access on sockfd, this action will be blocked until the lock is removed or
	mode is converted to non-block.
	it returns 0 on success, non-zero on failure.	*/
int network_init(int sockfd, int blockopt)
{
	extern int errno;
	//int i;
	int opt;
	int flags;
	int error;

	error = 0;
	opt = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) < 0) {
		syslog(LOG_ERR,"network_controller.c: network_init(): cannot set socket SO_KEEPALIVE (%s)", strerror(errno));
		error++;
	}else
		syslog(LOG_INFO, "network_controller.c: network_init(): set socket %d SO_KEEPALIVE - status: ok!", sockfd);
	flags = fcntl(sockfd, F_GETFL, 0);
	if (flags != -1) {
		if (blockopt == NONBLOCKING) {
			flags |= O_NONBLOCK;
			syslog(LOG_INFO, "network_controller.c: network_init(): flag for socket %d is NONBLOCK - status: ok!", sockfd);
		} else {
			flags &= ~O_NONBLOCK;
			syslog(LOG_INFO, "network_controller.c: network_init(): flag for socket %d is BLOCK - status: ok!", sockfd);
		}
		flags = fcntl(sockfd, F_SETFL, flags);
	}
	if (flags == -1) {
		if (blockopt == NONBLOCKING)
			syslog(LOG_ERR,"network_controller.c: network_init(): cannot set socket O_NONBLOCK (%s)", strerror(errno));
		else
			syslog(LOG_ERR,"network_controller.c: network_init(): cannot remove O_NONBLOCK from socket (%s)", strerror(errno));
		error++;
	}else
		syslog(LOG_INFO, "network_controller.c: network_init(): set block-mode for socket %d - status: ok!", sockfd);
	return(error);
}

/*	Location: network_controller.c
	this function is called from handle_connection() in server.c, as a result of our parent process accept()'ing a new socket connection.
	returns 0 on success, non-zero on failure.	*/
int parent_accept_socket_connection(int sockfd)
{
	extern SERIAL_INFO *si;						/* global ptr */
	extern struct config_t conf;
	SERIAL_INFO *sabre_serial_port;				/* a serial port from the pool */
	int serial_file_descriptor;					/* serial port file descriptor */
	struct termios old_setting;					/* original termios */
	struct termios new_setting;					/* our custom termios */
	//char *p;									/* gp ptr */

	/* allocate a serial port for SabreLite and prepare it for use */
	sabre_serial_port = serial_port_init(&serial_file_descriptor, old_setting, new_setting);
	if (sabre_serial_port == NULL) {
		/* The next 2 lines will be disabled for easy checking!  ---- Changelog on 18.09.2015*/
		//p = "network_controller.c: Unable to allocate a serial port on Sabre for you.\r\n";
		//write(sockfd, p, strlen(p));
		return(1);
	}else
		syslog(LOG_INFO, "network_controller.c: parent_accept_socket_connection(): Sabre's serial port status: ok!");

	/* store a global ptr to the sabre serial port */
	si = sabre_serial_port;
	/* set socket options: keep-alive and non-blocking mode. */
	network_init(sockfd, BLOCKING);
	syslog(LOG_INFO, "network_controller.c: parent_accept_socket_connection(): network_init() - status: ok!");
	/* pass data between the modem and the socket */
	serial_ip_communication_process(sockfd, serial_file_descriptor, new_setting, sabre_serial_port);
	/* restore the modem line to its original state */
	serial_cleanup(sabre_serial_port, &serial_file_descriptor, old_setting, new_setting);
	si = NULL;
	return(0);
}

