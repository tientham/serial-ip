/*
 * network_handle.c
 *	This is to handle Sabre server.
 *	Note: My software is not going to implement a full RFC2217.
 *	I just need a remotely control from a PC to Sabre though telnet for setting up Sabre COM-Port parameters.
 *	Or sending RS232 commands and receiving data from serial machine.
 *	 *	The work of "state change" will leave for the future development.
 *  Created on: Aug 7, 2015
 *      Author: tientham
 */

#include "serial_ip.h"

/*
	Location: network_handle.c
	This function is the heart of the software, when server got at least a connection from remote client.
	this function passes data between sabre's serial port and the socket via buffers.
	There are 3 buffers for performing the function.
	1. Sabre buffer for immediate point.
	2. Serial port buffer for communicating with serial device/machine.
	3. Socket buffer for communicating with client.

	Function:
	- init the telnet options structure
	- send initial telnet options: Com Port Control, Binary, etc.
	- set up select() call and enable a timeout
	- enter select() loop
	- use select() timeout to poll modem for state changes
	- also check for pending signals; SIGINT indicates BREAK condition
	- buffer data to/from modem/socket
	- handle telnet option negotiations
	- watch i/o channels for the IAC char and escape it, when present
	- exit select() loop on socket EOF
	returns 0 on success, non-zero on failure (eg. failed to allocate memory for buffers).
*/
int serial_ip_communication_process(int sockfd, int serial_file_descriptor, struct termios newterm, SERIAL_INFO *sabre_serial_port)
{
	extern int errno;
	extern int signo_child;							/* what signal did we receive? */
	extern struct config_t conf;					/* built from config file */
	extern int client_logged_in;					/* is the client still "logged in"? */
	extern int raw_flag;							/* raw TCP mode? */
	BUFFER *socket_to_serial_buf;					/* buffer for socket -> modem */
	BUFFER *serial_to_socket_buf;					/* buffer for modem -> socket */
	BUFFER *sabre_to_socket_buf;					/* buffer for us -> socket */
	struct op_pdu op_pdu_recv;						/* buffer for raw TCP mode */
	struct op_pdu op_pdu_send;						/* buffer for raw TCP mode */
	struct timeval tv;								/* timer for select() */
	int tvsecs;										/* what to put into tv.tv_sec */
	time_t now, elapsed, idle_cnt;
	int idle;										/* connection went idle */
	int ret;										/* return value of select() */
	int maxfd;										/* highest fd + 1 */
	int error;										/* error flag */
	int check_serialfd;								/* check if serial fd is set in socket entry table or not.	*/
	fd_set orig_fds;								/* original fd struct */
	fd_set read_fds;								/* read fd struct */

	write_to_debuglog(DBG_INF, "network_handle.c: serial_ip_communication_process(): sockfd fd=%d, serial port fd=%d",
			sockfd, serial_file_descriptor);
	syslog(LOG_INFO, "network_handle.c: serial_ip_communication_process(): sockfd fd=%d, serial port fd=%d",
				sockfd, serial_file_descriptor);
	/* allocate buffers */
	socket_to_serial_buf = bfmalloc("network", SIZE_BUFFER);
	serial_to_socket_buf = bfmalloc("serial", SIZE_BUFFER);
	sabre_to_socket_buf = bfmalloc("sabre", SIZE_BUFFER);
	if ((socket_to_serial_buf == NULL) || (serial_to_socket_buf == NULL) || (sabre_to_socket_buf == NULL))
	{
		bffree(socket_to_serial_buf);
		bffree(serial_to_socket_buf);
		bffree(sabre_to_socket_buf);
		return(1);
	}
	bfdump(socket_to_serial_buf, 1);
	bfdump(serial_to_socket_buf, 1);
	bfdump(sabre_to_socket_buf, 1);

	syslog(LOG_INFO, "network_handle.c: serial_ip_communication_process(): initialization for buffers - status: ok!");
	if(raw_flag){
		/*Init buffer in raw TCP mode.	*/
		bzero(&op_pdu_send, sizeof(op_pdu_send));
		bzero(&op_pdu_recv, sizeof(op_pdu_recv));
	}

	/* init telnet options structure and send initial options when sever type is concurrent or iterative. */
	if (!raw_flag)
		telnet_init(sockfd, sabre_to_socket_buf);
	/* set up select loop */
	maxfd = (sockfd >= serial_file_descriptor ? sockfd : serial_file_descriptor) + 1;
	FD_ZERO(&orig_fds);															/* Clear all entries from orig_fd set.*/
	FD_SET(sockfd, &orig_fds);													/* Add network fd to orig_fd set.*/
	FD_SET(serial_file_descriptor, &orig_fds);									/* Add serial fd to orig_fd set.*/
	check_serialfd = FD_ISSET(serial_file_descriptor, &orig_fds);
	if(check_serialfd == 0)
		syslog(LOG_DEBUG, "network_handle.c: si_com_proc(): set serial fd %d to socket fd entry table failed",
				serial_file_descriptor);
	else
		syslog(LOG_INFO, "network_handle.c: si_com_proc(): set serial fd %d to socket fd entry table - status: ok!",
				serial_file_descriptor);
	error = 0;																	/* Default error flag.*/
	client_logged_in = 1;

	/* calc select() timeout value */
	idle = 0;
	idle_cnt = 0;
	tvsecs = MIN(conf.ms_pollinterval, conf.ls_pollinterval);
    if (conf.idletimer > 0)
    {
		tvsecs = MIN(conf.idletimer,tvsecs);
	}

	/* select loop */
	while ((client_logged_in) && (! idle) && (! error))
	{
		/* check for idle connection */
		if (conf.idletimer > 0)
		{
			if (idle_cnt >= conf.idletimer)
			{
				if (! idle)														/* we're just going into idle state */
				{
					idle++;														/* set the idle state flag */
					write_to_debuglog(DBG_INF, "network_handle.c: si_com_proc(): terminating idle connection on serial port %s",
							sabre_serial_port->device);
					syslog(LOG_INFO, "network_handle.c: si_com_proc(): terminating idle connection on serial port %s",
												sabre_serial_port->device);
					/* let the remote user know what's happening */
					bfstrcat(sabre_to_socket_buf, "\r\nserial_ip: terminating idle connection\r\n");
					write_from_buffer_to_fd(sockfd, sabre_to_socket_buf);
					/* Depend on server type, we perform a suitable action. */
					/* tell the client to log out if in server "concurrent" or "itarative".*/
					if (!raw_flag)
						send_telnet_option(sockfd, sabre_to_socket_buf, DO, TELOPT_LOGOUT);
					else{
						;}
					msleep(250000);		/* sleep for 0.25second */
				}
				/*	we want to go through the select loop one last time,
					in order to process the client's reply to DO LOGOUT.	*/
			}
		}
		tv.tv_sec = tvsecs;													/* set timeout */
		tv.tv_usec = 0;
		read_fds = orig_fds;												/* structure copy */

		check_serialfd = FD_ISSET(serial_file_descriptor, &read_fds);
		if(check_serialfd == 0)
			syslog(LOG_DEBUG, "network_handle.c: si_com_proc(): set serial fd %d to socket fd entry table failed",
					serial_file_descriptor);
		else
			syslog(LOG_INFO, "network_handle.c: si_com_proc(): set serial fd %d to socket fd entry table with check value %d - status: ok!",
					serial_file_descriptor, check_serialfd);

		now = time((time_t *) NULL);										/* current time, in seconds */
		ret = select(maxfd, &read_fds, NULL, NULL, &tv);
		switch (ret)
		{
		case -1:															/* select error */
			if (errno != EINTR)
				system_errorlog("network_handle.c: si_com_proc(): select() error");
				syslog(LOG_ERR, "network_handle.c: si_com_proc(): select() error: %s", strerror(errno));

			elapsed = time((time_t *) NULL) - now;							/* seconds spent in select() */
			idle_cnt += elapsed;											/* accumulate idle time */
			break;
		case 0:																/* select timeout */
			elapsed = time((time_t *) NULL) - now;							/* seconds spent in select() */
			idle_cnt += elapsed;											/* accumulate idle time */
			write_to_debuglog(DBG_INF, "network_handle.c: si_com_proc(): no any connection at the moment on serial port %s",
										sabre_serial_port->device);
			syslog(LOG_INFO, "network_handle.c: si_com_proc(): no any connection at the moment on serial port %s",
													sabre_serial_port->device);
/* For now, we do not need to use this, for clear seeing. ---- Changelog on 18.09.2015*/
			/* let the remote user know what's happening */
			//bfstrcat(sabre_to_socket_buf, "\r\nserial_ip: no any connection on sabre serial port\r\n");
			//write_from_buffer_to_fd(sockfd, sabre_to_socket_buf);
			/*Note: Refer to the author desciption at the beggining, */
#if 0
			advise_client_of_state_changes(sockfd, serial_file_descriptor, sabre_to_socket_buf);
#endif
			break;
		default:															/* select fds for read/write */
			idle_cnt = 0;													/* reset idle counter */
			if (FD_ISSET(sockfd, &read_fds))								/*Return true if sockfd is already in read_fd set. */
			{
				syslog(LOG_INFO, "network_handle.c: si_com_proc(): reading on network socket %d......", sockfd);
				/*Folow direction: Client => sockfd, sever read from sockfd and put into socket_to_serial buffer. */
				if(!raw_flag)
					error = read_socket(sockfd, serial_file_descriptor, socket_to_serial_buf, sabre_to_socket_buf);		/*Read socket fd */
				else
					error = raw_TCP_socket_to_serial(sockfd, serial_file_descriptor, (void *) &op_pdu_recv, sizeof(op_pdu_recv));
				if (! error)
				{
					if(!raw_flag)
						error = write_serial(serial_file_descriptor, socket_to_serial_buf);
				}
				if (error) continue;										/* this will break the while loop */
			}
			/*if (FD_ISSET(serial_file_descriptor, &read_fds)) 				Return true if serial fd is already in read_fd set. */
			if (check_serialfd)
			{
				if(raw_flag){
					syslog(LOG_INFO, "network_handle.c: si_com_proc(): now, read data on serial port and send to TCP socket");
					error = raw_data_to_TCP_socket(serial_file_descriptor, sockfd, (void*) &op_pdu_send, sizeof(op_pdu_send));
				}else
					error = read_serial(serial_file_descriptor, serial_to_socket_buf);
				if ((! error) && (!raw_flag)) {
					error = write_socket(sockfd, serial_to_socket_buf);
				}
				if (error) continue;										/* this will break the while loop */
			}
			/*Note: Refer to the author desciption at the beggining, */
#if 0
			advise_client_of_state_changes(sockfd, serial_file_descriptor, sabre_to_socket_buf);
#endif
			break;
		}
		child_pending_signal_handle();										/* did we receive any signals? */

		/* react to loss of carrier */
#if 0
		if (sabre_serial_port->track_carrier) {
			if (check_carrier_state(serial_file_descriptor) == LOST_CARRIER) {
				++error;													/* will break the while() loop */
				advise_client_of_state_changes(sockfd,serial_file_descriptor,sabre_to_socket_buf);
			}
		}
#endif
	}	/* while() loop ends */
	child_pending_signal_handle();											/* did we receive any signals? */
	if(!raw_flag){
		bffree(socket_to_serial_buf);											/* release the buffers */
		bffree(serial_to_socket_buf);
		bffree(sabre_to_socket_buf);
	}
	return(0);
}

/*	Location: network_handle.c
	this function provides a concurrent server (ie. one that accepts socket connections and forks to perform the
	specified function).  the function whose address is passed to us will be called with its socket fd as the only arg.
	This function does not return. Errors encountered are considered fatal.	*/
void concurrent_server(int sockfd, int (*funct)(int), int *signo, void (*sighandler)(int))
{
	extern int errno;
	int sockfd_for_client;
	socklen_t client_len;
	int childpid;
	struct sockaddr_in client_addr;
	int ret;

	for ( ; ; ) {
		/*	Wait for a connection from a client process.	*/
		client_len = sizeof(client_addr);
		errno = 0;
		syslog(LOG_DEBUG,"Server is listening.....");
		sockfd_for_client = accept(sockfd,(struct sockaddr *) &client_addr, &client_len);
		if (sockfd_for_client < 0) {
			if (errno != EINTR)
				syslog(LOG_ERR,"network_handle.c: concurrent_server(): accept error (%s)",strerror(errno));
			if ((signo != NULL) && (*signo != 0))
			{
				(*sighandler)(*signo);										/* call the signal handler */
			}
			continue;
		}else
			syslog(LOG_DEBUG, "got connection from remote client!");

#ifdef USE_TCP_WRAPPERS
		if (access_control(newsockfd) != 0) {
			close(newsockfd);
			continue;
		}
#endif
		if ((childpid = fork()) < 0)
		{
			syslog(LOG_ERR,"serial_handle.c: serial_port_init(): fork error (%s)",strerror(errno));
		} else if (childpid == 0) {											/* child process */
			close(sockfd);													/* close original socket */
			ret = (*funct)(sockfd_for_client);								/* process the request */
			_exit(ret);
		}
		close(sockfd_for_client);											/* parent process */
	}
}

/*	Location: network_handle.c
	this function provides an iterative server (ie. one that accepts socket connections and performs the specified
	function without fork()ing). Function whose address is passed to us will be called with its socket fd as only arg.
	This function does not return. Errors encountered are considered fatal.		*/
void iterative_server(int sockfd, int (*funct)(int), int *signo, void (*sighandler)(int))
{
	extern int errno;
	int sockfd_for_client;
	socklen_t client_len;
	struct sockaddr_in client_addr;
	int ret;

	for ( ; ; )
	{
		/*	Wait for a connection from a client process.*/
		client_len = sizeof(client_addr);
		errno = 0;
		syslog(LOG_DEBUG,"Server is listening.....");
		sockfd_for_client = accept(sockfd, (struct sockaddr *) &client_addr, &client_len);
		if (sockfd_for_client < 0) {
			if (errno != EINTR)
				syslog(LOG_ERR,"accept error (%s)",strerror(errno));
			if ((signo != NULL) && (*signo != 0))
			{
				(*sighandler)(*signo);	/* call the signal handler */
			}
			continue;
		}else
			syslog(LOG_DEBUG, "got connection from remote client!");
#ifdef USE_TCP_WRAPPERS
		if (access_control(newsockfd) != 0) {
			close(newsockfd);
			continue;
		}
#endif
		ret = (*funct)(sockfd_for_client);		/* process the request */
		close(sockfd_for_client);
	}
}

/*	Location: network_handle.c
	this function provides an raw TCP server (ie. one that accepts socket connections and performs the specified
	function without fork()ing).
	raw TCP server will accept RS232 commands from remote client and forward to the serial port. The data if available will
	be given back to client.
	This function does not return. Errors encountered are considered fatal.		*/
void raw_TCP_gateway(int sockfd, int (*funct)(int), int *signo, void (*sighandler)(int))
{
	extern int errno;
	int sockfd_for_client;
	socklen_t client_len;
	struct sockaddr_in client_addr;
	int ret;

	for ( ; ; )
	{
		/*	Wait for a connection from a client process. */
		client_len = sizeof(client_addr);
		errno = 0;
		syslog(LOG_DEBUG, "server is listening.....");
		sockfd_for_client = accept(sockfd, (struct sockaddr *) &client_addr, &client_len);
		if (sockfd_for_client < 0) {
			if (errno != EINTR)
				syslog(LOG_ERR, "accept error (%s)", strerror(errno));
			if ((signo != NULL) && (*signo != 0))
			{
				(*sighandler)(*signo);	/* call the signal handler */
			}
			continue;
		}else
			syslog(LOG_INFO, "got connection from remote client!");
#ifdef USE_TCP_WRAPPERS
		if (access_control(newsockfd) != 0) {
			close(newsockfd);
			continue;
		}
#endif
		ret = (*funct)(sockfd_for_client);		/* process the request */
		close(sockfd_for_client);
	}
}

int gsockfd = -1;						/* global copy of child socket fd for signal handle.*/

/*	Location: network_handle.c
	This function is to handle connection from client. called by concurrent_server() and by	iterative_server(),
	after accepting a client connection.	*/
int handle_network_connection(int sockfd)
{
	extern struct config_t conf;		/* built from config file */
	extern int gsockfd;					/* global copy of sockfd */
	int ret;							/* return value from rfc2217() */

	gsockfd = sockfd;					/* save our socket fd */
	syslog(LOG_DEBUG,"network_handle.c: network_handle_connection(): client socket is fd %d", sockfd);
	/*
		if the server type is concurrent, then the process that is
		executing this code is a child of the "master" daemon (ie.
		the one that's listening on the socket).  if that's the case,
		then we want to reset some of the signal handlers that we
		inherited from our parent.
	*/
	if (strcmp(conf.server_type, "concurrent") == 0)
		reset_signals();				/* reset child's signal handlers */

	if (chdir(conf.directory) != 0)	{	/* set working directory */
		syslog(LOG_ERR,"network_handle.c: network_handle_connection(): cannot chdir(%s): %s",
				conf.directory,strerror(errno));
		_exit(1);
	}else
		syslog(LOG_INFO, "network_handle.c: now working at %s", conf.directory);
	if (setgid(conf.gid) != 0) {
		syslog(LOG_ERR,"network_handle.c: network_handle_connection(): setgid(%d) error: %s",
				conf.gid,strerror(errno));
		_exit(1);
	}else
		syslog(LOG_INFO, "network_handle.c: set group id for %s: ok!", conf.directory);
	if (setuid(conf.uid) != 0) {
		syslog(LOG_ERR,"network_handle.c: network_handle_connection(): setuid(%d) error: %s",
				conf.uid,strerror(errno));
		_exit(1);
	}else
		syslog(LOG_INFO, "network_handle.c: set user id for %s: ok!", conf.directory);
	ret = parent_accept_socket_connection(sockfd);
	disconnect(sockfd);
	return(ret);
}
/*	Location: network_handle.c
	This is to set the socket file descriptor to non-blocking mode.
	Return file status flag on success.	Return -1 on error.		*/
int set_nonblocking_mode(int sockfd)
{
	int flags;

	/* fcntl with flag F_GETFL mean we need to get the file acess mode and file status flag. */
	flags = fcntl(sockfd, F_GETFL,0);
	if (flags != -1) {
		flags |= O_NONBLOCK;       /* Set flag to nonblock. */
		flags = fcntl(sockfd, F_SETFL, flags);
	}
	if (flags == -1) {
		return(flags);
	}
	return(0);
}
/*	Location: network_handle.c
	create a tcp server on this host at the specified port number.
	returns a socket fd or -1 on error.	*/
int create_server_socket(int tcp_network_port, int block_mode)
{
	extern int errno;
	int sockfd;
	int yes = 1;    /* Use for setsockopt*/
	struct sockaddr_in server_addr;
	/*
		Open a TCP socket file descriptor (an Internet stream socket).
	*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		syslog(LOG_ERR, "network_handle.c: create_server_socket(): cannot open stream socket (%s)", strerror(errno));
		return(-1);
	}else
		syslog(LOG_INFO, "network_handle.c: create_server_socket(): open stream socket (%d) - status: ok!", sockfd);
	/*	set the socket to non-blocking mode if this option was passed in to us	*/
	if (block_mode == NONBLOCKING) {
		if (set_nonblocking_mode(sockfd) < 0) {
			syslog(LOG_ERR, "network_handle.c: create_server_socket(): cannot set socket to non-blocking mode (%s)", strerror(errno));
			close(sockfd);
			return(-1);
		}
	}
	/*
		SOL_SOCKET: is to manipulate SO_KEEPALIVE, SO_REUSEADDR (socket options)
		set socket options SO_KEEPALIVE and SO_REUSEADDR
		SO_KEEPALIVE: enable sending of keepalive message on connection-oriented socket.
		SO_REUSEADDR: refers man page for more information.
	*/
	if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)) < 0) {
		syslog(LOG_ERR,"network_handle.c: create_server_socket(): cannot set SO_KEEPALIVE (%s)",strerror(errno));
		close(sockfd);
		return(-1);
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		syslog(LOG_ERR,"network_handle.c: create_server_socket(): cannot set SO_REUSEADDR (%s)",strerror(errno));
		close(sockfd);
		return(-1);
	}
	bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;							/*Ipv4*/
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);			/*INADDR_ANY: bind wildcard address.*/
	server_addr.sin_port = htons(tcp_network_port);

	if (bind(sockfd,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
		syslog(LOG_ERR,"network_handle.c: create_server_socket(): cannot bind local address (%s)",strerror(errno));
		return(-1);
	}
	if(listen(sockfd, 5) == -1)
	{
		syslog(LOG_ERR,"network_handle.c: create_server_socket(): listen failed.");;
	}
	return(sockfd);
}
/*	Location: network_handle.c
	This function is to create the server TCP socket. There are 3 types of server:
	1. Raw TCP gateway.
	2. Concurrent.
	3. Iterative.
	Function doesnot return.	*/
void server_init(int tcp_network_port)
{
	extern struct config_t conf;		/* built from config file */
	extern int signo;					/* parent signal number */
	extern int raw_flag;
	int sockfd;

	/* create a blocking TCP server socket	*/
	sockfd = create_server_socket(tcp_network_port, BLOCKING);
	if (sockfd == -1) {
		syslog(LOG_ERR,"network_handle.c: server_init(): cannot create server tcp socket");
		exit(1);
	}
	syslog(LOG_DEBUG,"network_handle.c: server_init(): server socket is fd %d",sockfd);
	/*	accept socket connections.  for a concurrent server, we fork a child process for each new connection.
		for an iterative server, we handle the connection within this process.
		for a raw TCP gateway, we handle with raw data.
		this function does not return.	*/
	if (strcmp(conf.server_type,"concurrent") == 0){
		syslog(LOG_DEBUG,"serial-ip: Now we are in the %s mode!", conf.server_type);
		concurrent_server(sockfd,handle_network_connection, &signo, parent_signal_received);
	}
	if (strcmp(conf.server_type,"iterative") == 0){
		iterative_server(sockfd, handle_network_connection, &signo, parent_signal_received);
		syslog(LOG_DEBUG,"serial-ip: Now we are in the %s mode!", conf.server_type);
	}
	if (strcmp(conf.server_type,"raw TCP gateway") == 0)
	{
		raw_flag = 1;
		syslog(LOG_DEBUG,"serial-ip: Now we are in the %s mode!", conf.server_type);
		raw_TCP_gateway(sockfd, handle_network_connection, &signo, parent_signal_received);
	}
	exit(1);							/* not reached */
}

/*	Location: network_handle.c
	This is to disconnect socket file descriptor. */
void disconnect(int sockfd)
{
	extern int gsockfd;					/* global copy of sockfd */

	shutdown(sockfd,SHUT_RDWR);
	syslog(LOG_INFO,"network_handle.c: disconnect(): closed socket fd %d", sockfd);
	gsockfd = -1;
}
