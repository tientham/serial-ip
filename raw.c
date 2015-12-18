/*
 * raw.c
 *	This is to handle actions regarding a raw TCP gateway.
 *  Created on: Aug 10, 2015
 *      Author: tientham
 */

#include "serial_ip.h"

#define MAX_LEN_MESSAGE 100

/* Location raw.c
 This function is to set up client network setting. */
void pack_uint16_t(int pack, uint16_t *num)
{
	uint16_t i;

	if (pack)
		i = htons(*num);     //htons()    host to network long
	else
		i = ntohs(*num);	// ntohs()    network to host short
	*num = i;
}

/* Location raw.c
 * This function is to set up client network setting. */
static ssize_t serialip_xmit(int sockfd, void *buff, size_t bufflen, int sending)
{
	ssize_t total = 0;

	if (!bufflen)
		return 0;
	do {
		ssize_t nbytes;
		if (sending){
			syslog(LOG_INFO,"raw.c: send() starting....");
			nbytes = send(sockfd, buff, bufflen, 0);
			syslog(LOG_INFO,"raw.c: send() %d byte(s) - status: done!", nbytes);
			if(nbytes > 0)
				return nbytes;
		}else{
			syslog(LOG_INFO,"raw.c: recv() starting....");
			nbytes = recv(sockfd, buff, bufflen, 0);
			//nbytes = recv(sockfd, buff, bufflen, MSG_WAITALL);
			syslog(LOG_INFO,"raw.c: recv() %d byte(s) - status: done!", nbytes);
			if(nbytes > 0)
				return nbytes;
		}
		if (nbytes < 0)
			return -1;
		if (nbytes == 0)
			return 0;
		buff	= (void *) ((intptr_t) buff + nbytes);
		bufflen	-= nbytes;
		total	+= nbytes;
	} while (bufflen > 0);
	return total;
}

/*
 * Location raw.c
 * This function is to read message from networking socket in raw TCP mode.
 */
ssize_t read_from_raw_tcp_buffer(int sockfd, void *buff, size_t bufflen)
{
	return serialip_xmit(sockfd, buff, bufflen, 0);
}

/* Location raw.c
 * This function is to read message from serial socket in raw TCP mode. */
ssize_t read_from_raw_serial_socket(int sockfd, void *buff, size_t bufflen)
{
	return serialip_xmit(sockfd, buff, bufflen, 0);
}

/* Location raw.c
 * This function is to send data to networking socket in raw TCP mode. */
ssize_t serialip_send(int sockfd, void *buff, size_t bufflen)
{
	return serialip_xmit(sockfd, buff, bufflen, 1);
}

/*	Location: raw.c
	This is to read from the networking socket and write content into socket_to_serial buffer.
	returns 0 on success, 1 on failure (including EOF)	*/
int raw_TCP_socket_to_serial(int sockfd, int serial_file_descriptor, void* buff, size_t bufflen)
{
	extern int useconds;			/* i/o wait time */
	int n, i;
	int size = 0;
	int numbytes;
	int error = 0;
	char raw_buffer[256];
#if 0
	struct op_pdu op_pdu_recv;
#endif
	unsigned char command[MAX_LEN_MESSAGE];
	char *p;

#if 0
	bzero(&op_pdu_recv, sizeof(op_pdu_recv));
#endif

	/* wait a bit before reading the socket */
	if (useconds > 0) {
		msleep(useconds);
	}
	syslog(LOG_INFO, "raw.c: raw_TCP_socket_to_serial(): reading.....!");
#if 0
	n = read_from_raw_tcp_buffer(sockfd, (void *) &op_pdu_recv.message, sizeof(op_pdu_recv));
#endif
	n = read_from_raw_tcp_buffer(sockfd, raw_buffer, sizeof(raw_buffer));
	syslog(LOG_INFO, "raw.c: raw_TCP_socket_to_serial(): read %d byte(s) on socket", n);

	/*Piece of code for checking version.*/
#if 0
	syslog(LOG_INFO, "raw.c: raw_TCP_socket_to_serial(): reading.....!");
	n = read_from_raw_tcp_buffer(sockfd, (void *) &op_pdu_recv, sizeof(op_pdu_recv));
	syslog(LOG_INFO, "raw.c: raw_TCP_socket_to_serial(): read %d byte(s) on socket", n);
	PACK_OP_PDU(0, &op_pdu_recv);

	if(op_pdu_recv.version != SERIAL_IP_VERSION){
		syslog(LOG_DEBUG, "raw.c: raw_TCP_socket_to_serial(): version mismatch!");
		error = 1;
		return error;
	}else
		syslog(LOG_INFO, "raw.c: raw_TCP_socket_to_serial(): version match - status: ok!");
#endif

	syslog(LOG_INFO, "raw.c: raw_TCP_socket_to_serial(): reading message....");
#if 0
	for(i= 0; i< sizeof(op_pdu_recv.message) ; i++){
			command[i] = op_pdu_recv.message[i];
			size++;
	}
#endif
	/*----Change Log on 18.09.2015 */
	//for(i= 0; i< sizeof(raw_buffer) ; i++){
	for(i= 0; i< n ; i++){
		command[i] = raw_buffer[i];
		size++;
		}
	p = (char*) command;
	syslog(LOG_INFO,"raw.c: raw_TCP_socket_to_serial(): message is %s", p);
	if(size > 0){
#if 0
	command[size] = '\r';
	command[size+1]	= '\n';
	command[size+2]	= '\0';
#endif
	command[size] = '\0';
	/*----Change Log on 18.09.2015 (add next line)*/
	raw_buffer[n] = '\0';
	}else
		error = 1;
	syslog(LOG_INFO,"raw.c: raw_TCP_socket_to_serial(): message after appending is %s", p);
	/*----Change Log on 18.09.2015 (next line)*/
	//numbytes = write(serial_file_descriptor, command, sizeof(command));
	numbytes = write(serial_file_descriptor, raw_buffer, n+1);
	msleep(250000);
	if(numbytes < 0)
	{
		error = 1;
		syslog(LOG_ERR, "raw.c: process_serial_command(): write to serial socket fd error %s", strerror(errno));
	}else
		syslog(LOG_INFO,"raw.c: raw_TCP_socket_to_serial(): write to serial socket %d with %d error(s)- status: done!", serial_file_descriptor, error);

	return (error);
}

/*	Location: raw.c
	In raw TCP mode, this is to read data from serial_file_descriptor, and
	put its content into serial_to_socket buffer.
	returns 0 on success, 1 on failure (including EOF).	*/
int raw_data_to_TCP_socket(int serial_file_descriptor, int sockfd, void* buff, size_t bufflen)
{
	extern int useconds;														/* i/o wait time */
	int numbytes;
	int error = 0;
	struct op_pdu op_pdu_send;

	/* wait a bit before reading the serial port */
	/*if (useconds > 0) {
		msleep(useconds);
	}*/

	bzero(&op_pdu_send, sizeof(op_pdu_send));
	syslog(LOG_INFO, "raw.c: raw_data_to_TCP_socket(): reading on serial fd %d.....!", serial_file_descriptor);
	numbytes = read(serial_file_descriptor, op_pdu_send.data, sizeof(op_pdu_send.data));
	if (numbytes < 0) {															/* error on read */
		error = 1;
		syslog(LOG_DEBUG, "raw.c: raw_data_to_TCP_socket(): error when read on serial fd %d (hints:may check cable communication)",
				serial_file_descriptor);
	}else{
		syslog(LOG_INFO, "raw.c: raw_data_to_TCP_socket(): read %d byte(s) on serial fd %d and wrote it to packet",
						numbytes, serial_file_descriptor);
		op_pdu_send.data[numbytes]='\n';
	}
		/* piece of code for adding version checking.*/
#if 0
	op_pdu_send.version = SERIAL_IP_VERSION;
	syslog(LOG_INFO, "raw.c: raw_data_to_TCP_socket(): pack serial-ip version before sending to remote client!");
	*/
	PACK_OP_PDU(1, &op_pdu_send);

	if(serialip_send(sockfd, (void *) &op_pdu_send, sizeof(op_pdu_send)) <0){
		error = 1;
		syslog(LOG_ERR,"raw.c: raw_data_to_TCP_socket(): unable to send command");
	}
	else
		syslog(LOG_INFO, "raw.c: raw_data_to_TCP_socket(): data is sent to remote client!");
#endif
	/*----Change Log on 18.09.2015 (change next line)*/
	//if(serialip_send(sockfd, (void *) &op_pdu_send.data, sizeof(op_pdu_send.data)) <0){
	if(serialip_send(sockfd, (void *) &op_pdu_send.data, numbytes+1) <0){
			error = 1;
			syslog(LOG_ERR,"raw.c: raw_data_to_TCP_socket(): unable to send command");
		}
		else
			syslog(LOG_INFO, "raw.c: raw_data_to_TCP_socket(): data is sent to remote client!");

	return(error);
}

