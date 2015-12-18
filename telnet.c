/*
 * telnet.c
 *	This is a short implementation of telnet. We are not going to implement all rfc2217.
 *	Note: My software is not going to implement a full RFC2217.
 *	I just need a remotely control from a PC to Sabre though telnet for setting up Sabre COM-Port parameters.
 *	Or sending RS232 commands and receiving data from serial machine.
 *	The work of "state change" will leave for the future development.
 *  Created on: Aug 7, 2015
 *      Author: tientham
 */

#include "serial_ip.h"


/* for the tnmode[] array */
#define CLIENT 0x00
#define SERVER 0x01

/* global variables */
TELNET_OPTIONS tnoptions[MAX_TELNET_OPTIONS];
int tnmode[2];
int session_state = RESUME;
int carrier_state = NO_CARRIER;
int break_signaled = 0;
int ask_client_signature = 1;
int client_logged_in = 0;
SERIAL_INFO *si = NULL;
/* these default values are dictated by RFC2217.  don't change them! */
unsigned char linestate_mask = 0x00;
unsigned char linestate = 0;
unsigned char modemstate_mask = 0xff;
unsigned char modemstate = 0;

/*
	Location: telnet.c
	This is to send a telnet Com Port Control (CPC) suboption
	returns 0 on success, 1 on failure
*/
int send_telnet_cpc_suboption(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned char *content, int cmdlen)
{
	extern int errno;
	extern int session_state;
	static unsigned char optstr[MAX_TELNET_CPC_COMMAND_LEN+8];
	unsigned char *p;
	unsigned char *cp;
	unsigned long value;
	int size;
	int i;
	int ret;
	/* if cmdlen is 1, 2 or 4 then interpret the content as a value */
	if (cmdlen == 1) {
		value = (unsigned long) *content;
	} else if (cmdlen == 2) {
		value = (unsigned long) ntohs(*((unsigned short *) content));
	} else if (cmdlen == 4) {
		value = ntohl(*((unsigned long *) content));
	} else {
		value = 0L;
	}

	/* build telnet CPC suboption string */
	optstr[0] = IAC;
	optstr[1] = SB;
	optstr[2] = TELOPT_COM_PORT_OPTION;
	optstr[3] = suboptcode;
	size = 4;
	if (cmdlen >= MAX_TELNET_CPC_COMMAND_LEN) {
		cmdlen = MAX_TELNET_CPC_COMMAND_LEN - 1;
	}
	p = &optstr[4];
	/* the command string may contain the IAC char, which must be escaped */
	cp = content;
	for (i = 0; i < cmdlen; i++) {
		if (*cp == IAC)
		{
			*p++ = IAC;						/* add a 2nd IAC char to optstr */
			size++;							/* adj size */
			if (size > MAX_TELNET_CPC_COMMAND_LEN) break;
		}
		*p++ = *cp++;
		size++;
		if (size > MAX_TELNET_CPC_COMMAND_LEN) break;
	}
	*p++ = IAC;
	*p++ = SE;
	*p = '\0';
	size += 2;		/* count the IAC and the SE */

	/* append it to the buffer */
	bfstrncat(sabre_to_socket_buf, (const char*) optstr, size);
	/* send it */
	if (session_state == RESUME) {
		bfdump(sabre_to_socket_buf, 0);		/* debugging dump of BUFFER */
		ret = write_from_buffer_to_fd(sockfd, sabre_to_socket_buf);
		if (ret >= size) {		/* buffer may have contained more than just our optstr */
			telnet_cpc_log_subopt("sent", suboptcode, value, content, cmdlen);
			return(0);
		} else {
			telnet_cpc_log_subopt("error sending", suboptcode, value, content, cmdlen);
			return(1);
		}
	}
	return(0);
}

/*
	Location: telnet.c
	This is to response with a signature suboption. From RFC2217, suboption formation: IAC SB COM-PORT-OPT SIGNATURE <text> IAC SE
	If without text, the sender wish receive signature from the receiver.
*/
int respond_telnet_cpc_signature_subopt(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned char *command, int cmdlen)
{
	extern char *program_name;												/* our program name */
	extern char *version;													/* version string */
	extern SERIAL_INFO *si;													/* global modem ptr */
	extern int ask_client_signature;										/* ask client for its signature? */
	char format[32];														/* holds format string */
	char *content;
	int len;
	int ret;

	if (*command == '\0') {													/* client wants us to send our signature */
		strcpy(format,"%s %s");												/* initial format: program name and version number */
		len = strlen(program_name);
		len += strlen(version);
		len++;																/* +1 for a space between 'progname' and 'version' */
		if (si != NULL) {
			if (si->device != NULL) {
				strcat(format,", %s");
				len += 2;													/* +2 for a comma and a space */
				len += strlen(si->device);
			}
			if (si->description != NULL) {
				strcat(format,", %s");
				len += 2;													/* +2 for a comma and a space */
				len += strlen(si->description);
			}
		}
		len++;																/* +1 for a null terminator */
		content = malloc(len);
		if (content != NULL)
		{
			if ((si != NULL) && ((si->device != NULL) || (si->description != NULL))) {
				if ((si->device != NULL) && (si->description != NULL)) {
					snprintf(content, len, format, program_name, version, si->device, si->description);
				} else {
					if (si->device != NULL) {
						snprintf(content, len, format, program_name, version, si->device);
					} else {
						snprintf(content, len, format, program_name, version, si->description);
					}
				}
			} else {
				snprintf(content, len, format, program_name, version);
			}
			ret = send_telnet_cpc_suboption(sockfd, sabre_to_socket_buf, CPC_SIGNATURE_S2C, (unsigned char *) content, len-1);	/* -1 for the null */
			free(content);
		} else {
			syslog(LOG_ERR,"unable to allocate memory for signature");
			content = "serial_ip";
			ret = send_telnet_cpc_suboption(sockfd, sabre_to_socket_buf, CPC_SIGNATURE_S2C, (unsigned char *) content, strlen(content));
		}
		if (ret == 0) {				/* now server requests client signature by setting no content in the suboption code, but only once */
			if (ask_client_signature) {
				ret = send_telnet_cpc_suboption(sockfd, sabre_to_socket_buf, CPC_SIGNATURE_C2S, (unsigned char *) "", 0);
				ask_client_signature = 0;
			}
		}
	} else {						/* client has sent us their signature */
		syslog(LOG_INFO, "telnet.c: respond_telnet_cpc_signature_subopt(): telnet CPC client signature: %s", command);
		ret = 0;
	}
	return(ret);
}

/*
	Location: telnet.c
	This is to response with a set baudrate suboption. From RFC2217, suboption formation: IAC SB COM-PORT-OPT SET-BAUD <value(4)> IAC SE
	If without value, the sender wish receive current baudrate from the receiver.
*/
int respond_telnet_cpc_baudrate_subopt(int sockfd, int serial_file_descriptor, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned long value)
{
	unsigned int netvalue;
	int ret;

	if (value == 0L)							/* client wants us to send the baudrate */
	{
		value = get_baudrate(serial_file_descriptor);
		netvalue = (unsigned int) htonl(value);
		syslog(LOG_INFO,"telnet.c: respond_telnet_cpc_baudrate_subopt(): telnet CPC client requests the baudrate; sending %lu", value);
		ret = send_telnet_cpc_suboption(sockfd, sabre_to_socket_buf, CPC_SET_BAUDRATE_S2C, (unsigned char *) &netvalue, 4);
	} else {									/* client wants to set a new baudrate */
		syslog(LOG_INFO, "telnet.c: respond_telnet_cpc_baudrate_subopt(): telnet CPC client is setting the baudrate to %lu", value);
		ret = set_baudrate(serial_file_descriptor, value);
		/* send new baudrate in reply */
		value = get_baudrate(serial_file_descriptor);
		netvalue = (unsigned int) htonl(value);
		ret += send_telnet_cpc_suboption(sockfd, sabre_to_socket_buf, CPC_SET_BAUDRATE_S2C, (unsigned char *) &netvalue, 4);
	}
	return(ret);
}

/*
	Location: telnet.c
	This is to response with a set data size suboption. From RFC2217, suboption formation: IAC SB COM-PORT-OPT SET-DATASIZE <value(4)> IAC SE
	If without value, the sender wish receive current data size from the receiver.
*/
int respond_telnet_cpc_datasize_subopt(int sockfd, int serial_file_descriptor, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned long value)
{
	unsigned char datasize;
	int ret;

	if (value == CPC_DATASIZE_REQUEST) {	/* client wants us to send the data size */
		datasize = get_datasize(serial_file_descriptor);
		syslog(LOG_INFO, "telnet.c: respond_telnet_cpc_datasize_subopt(): telnet CPC client requests the data size; sending %s",
				telnet_cpc_datasize2str(datasize));
		ret = send_telnet_cpc_suboption(sockfd, sabre_to_socket_buf, CPC_SET_DATASIZE_S2C, (unsigned char *) &datasize, 1);
	} else {							/* client wants to set a new data size */
		syslog(LOG_INFO,"telnet.c: respond_telnet_cpc_datasize_subopt():  telnet CPC client is setting the data size to %s",
				telnet_cpc_datasize2str((unsigned char) value));
		ret = set_datasize(serial_file_descriptor, value);

		/* send new datasize in reply */
		datasize = get_datasize(serial_file_descriptor);
		ret += send_telnet_cpc_suboption(sockfd, sabre_to_socket_buf, CPC_SET_DATASIZE_S2C, (unsigned char *) &datasize, 1);
	}
	return(ret);
}

/*
	Location: telnet.c
	This is to response with a set parity suboption. From RFC2217, suboption formation: IAC SB COM-PORT-OPT SET-PARITY <value(1)> IAC SE
	If without value, the sender wish receive current data size from the receiver.
*/
int respond_telnet_cpc_parity_subopt(int sockfd, int serial_file_descriptor, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned long value)
{
	unsigned char parity;
	int ret;

	if (value == CPC_PARITY_REQUEST) 						/* client wants us to send the parity */
	{
		parity = get_parity(serial_file_descriptor);
		syslog(LOG_INFO, "telnet.c: respond_telnet_cpc_parity_subopt(): telnet CPC client requests the parity setting; sending \"%s\"",
				telnet_cpc_parity2str(parity));
		ret = send_telnet_cpc_suboption(sockfd, sabre_to_socket_buf, CPC_SET_PARITY_S2C, (unsigned char *) &parity, 1);
	} else {												/* client wants to set a new parity setting */
		syslog(LOG_INFO, "telnet.c: respond_telnet_cpc_parity_subopt(): telnet CPC client is setting the parity to \"%s\"",
				telnet_cpc_parity2str((unsigned char) value));
		ret = set_parity(serial_file_descriptor, value);

		/* send new parity setting in reply */
		parity = get_parity(serial_file_descriptor);
		ret += send_telnet_cpc_suboption(sockfd, sabre_to_socket_buf, CPC_SET_PARITY_S2C, (unsigned char *) &parity, 1);
	}
	return(ret);
}

/*
	Location: telnet.c
	This is to response with a set stop size suboption. From RFC2217, suboption formation: IAC SB COM-PORT-OPT SET-STOPSIZE <value(1)> IAC SE
	If without value, the sender wish receive current data size from the receiver.
*/
int respond_telnet_cpc_stopsize_subopt(int sockfd, int serial_file_descriptor, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned long value)
{
	unsigned char stopbits;
	int ret;

	if (value == CPC_STOPSIZE_REQUEST) 								/* client wants us to send the number of stop bits */
	{
		stopbits = get_stopsize(serial_file_descriptor);
		syslog(LOG_INFO, "telnet CPC client requests the number of stop bits; sending %s", telnet_cpc_stopsize2str(stopbits));
		ret = send_telnet_cpc_suboption(sockfd, sabre_to_socket_buf, CPC_SET_STOPSIZE_S2C, (unsigned char *) &stopbits, 1);
	} else {							/* client wants to set a new number of stop bits */
		syslog(LOG_INFO, "telnet CPC client is setting the number of stop bits to %s", telnet_cpc_stopsize2str((unsigned char) value));
		ret = set_stopsize(serial_file_descriptor, value);

		/* send new stopsize in reply */
		stopbits = get_stopsize(serial_file_descriptor);
		ret += send_telnet_cpc_suboption(sockfd, sabre_to_socket_buf, CPC_SET_STOPSIZE_S2C, (unsigned char *) &stopbits, 1);
	}
	return(ret);
}

/*
	Location: telnet.c
	log some info about a telnet CPC suboption: its name, its value,
	and a hex dump of the command string.
*/
void telnet_cpc_log_subopt(char *prefix, unsigned char suboptcode, unsigned long value, unsigned char *command, int cmdlen)
{
	static char fmt[1024];

	fmt[0] = '\0';
	if ((prefix != NULL) && (*prefix != '\0'))
		snprintf(fmt, sizeof(fmt),"%s ", prefix);
	strcat(fmt, "telnet CPC suboption \"");
	strcat(fmt, telnet_cpc_subopt2str(suboptcode));
	strcat(fmt,"\", value ");
	if (value < 256) {
		strcat(fmt,"0x%02x");									/* one byte value, show as hex */
	} else {
		strcat(fmt,"%lu");										/* multi-byte value, show as long decimal */
	}
	if (cmdlen > 1)
		strcat(fmt,", hex dump:");

	syslog(LOG_INFO, fmt, value);								/* send to syslog */

	if (cmdlen > 1)
		memdump((char *) command, cmdlen, NULL);				/* will go to syslog() too */
}

/*
	Location: telnet.c
	optstr points to a Telnet CPC option string which contains at least
	the option code, plus the IAC and SE characters.
*/

int process_telnet_cpc_suboption(int sockfd, int serial_file_descriptor, BUFFER *socket_to_serial_buf, BUFFER *sabre_to_socket_buf, unsigned char *optstr, int optlen)
{
	extern int session_state;
	static unsigned char command[MAX_TELNET_CPC_COMMAND_LEN];
	unsigned char suboptcode;
	unsigned char *iac;
	unsigned char *se;
	unsigned char *ptr;
	unsigned long value;
	int len;
	int n;
	int ret;

	/* sanity checking */
	if (optlen < 3) return(1);

	suboptcode = *optstr++;
	optlen--;

	/* get ptrs to IAC SE sequence which terminates the command */
	se = mystrchr(optstr, SE, optlen);	/* find SE char */
	if (se == NULL) return(1);
	iac = se - 1;						/* ptr to IAC char */
	if (*iac != IAC) return(1);

	/* calc length of command.  may be zero. */
	len = iac - optstr;
	if (len >= MAX_TELNET_CPC_COMMAND_LEN) {
		len = MAX_TELNET_CPC_COMMAND_LEN - 1;
	}

	/*
		copy command to our buffer and null-terminate it.  note that
		the command may be empty.
	*/
	if (len > 0)
		memcpy(command, optstr, len);
	command[len] = '\0';

	/*
		process IAC escapes within the command
	*/
	iac = mystrchr(command, IAC, len);
	while (iac != NULL) {
		if (*(iac+1) == IAC) {				/* double IAC */
			iac++;							/* point to 2nd IAC */
			n = len - (iac - command);		/* # chars after 2nd IAC */
			for (ptr = iac; n > 0; n--) {	/* shift chars 1 position left */
				*ptr = *(ptr+1);
				ptr++;
			}
			ptr = iac;						/* *iac is no longer IAC */
			len--;							/* adj len for removed IAC */
		} else {							/* else, not double IAC */
			ptr = iac + 1;
		}
		n = len - (ptr - command);			/* # chars left in command */
		iac = mystrchr(ptr, IAC, n);			/* any more IAC chars? */
	}

	/*
		if len is 1, 2 or 4 then interpret the command as a value
	*/
	if (len == 1) {
		value = (unsigned long) command[0];
	} else if (len == 2) {
		value = (unsigned long) ntohs(*((unsigned short *) command));
	} else if (len == 4) {
		value = ntohl(*((unsigned long *) command));
	} else {
		value = 0L;
	}

	/* log which telnet CPC suboption we've received, its args, and the arg value */
	telnet_cpc_log_subopt("received", suboptcode, value, command, len);

	/* process the telnet CPC suboption */
	ret = 0;
	switch (suboptcode)
	{
	case CPC_SIGNATURE_C2S:
	case CPC_SIGNATURE_S2C:
		ret = respond_telnet_cpc_signature_subopt(sockfd, sabre_to_socket_buf, suboptcode, command, len);
		break;
	case CPC_SET_BAUDRATE_C2S:
	case CPC_SET_BAUDRATE_S2C:
		ret = respond_telnet_cpc_baudrate_subopt(sockfd, serial_file_descriptor, sabre_to_socket_buf, suboptcode, value);
		break;
	case CPC_SET_DATASIZE_C2S:
	case CPC_SET_DATASIZE_S2C:
		ret = respond_telnet_cpc_datasize_subopt(sockfd, serial_file_descriptor, sabre_to_socket_buf, suboptcode, value);
		break;
	case CPC_SET_PARITY_C2S:
	case CPC_SET_PARITY_S2C:
		ret = respond_telnet_cpc_parity_subopt(sockfd, serial_file_descriptor, sabre_to_socket_buf, suboptcode, value);
		break;
	case CPC_SET_STOPSIZE_C2S:
	case CPC_SET_STOPSIZE_S2C:
		ret = respond_telnet_cpc_stopsize_subopt(sockfd, serial_file_descriptor, sabre_to_socket_buf, suboptcode, value);
		break;
		/*Note: Refer to this code desciption at the beggining, */
#if 0
	case CPC_SET_CONTROL_C2S:
	case CPC_SET_CONTROL_S2C:
		ret = respond_telnet_cpc_control_subopt(sockfd, serial_file_descriptor, sabre_to_socket_buf, suboptcode, value);
		break;
	/*
		the flow control suspend and resume commands are the only ones for
		which no reply is expected by the client
	*/
	case CPC_FLOWCONTROL_SUSPEND_C2S:
	case CPC_FLOWCONTROL_SUSPEND_S2C:
		syslog(LOG_INFO,"telnet CPC client suspends the session");
		session_state = SUSPEND;
		break;
	case CPC_FLOWCONTROL_RESUME_C2S:
	case CPC_FLOWCONTROL_RESUME_S2C:
		syslog(LOG_INFO,"telnet CPC client resumes the session");
		session_state = RESUME;
		break;

	case CPC_SET_LINESTATE_MASK_C2S:
	case CPC_SET_LINESTATE_MASK_S2C:
	case CPC_NOTIFY_LINESTATE_C2S:
	case CPC_NOTIFY_LINESTATE_S2C:
		ret = respond_telnet_cpc_linestate_subopt(sockfd, serial_file_descriptor, sabre_to_socket_buf, suboptcode, value);
		break;
	case CPC_SET_MODEMSTATE_MASK_C2S:
	case CPC_SET_MODEMSTATE_MASK_S2C:
	case CPC_NOTIFY_MODEMSTATE_C2S:
	case CPC_NOTIFY_MODEMSTATE_S2C:
		ret = respond_telnet_cpc_modemstate_subopt(sockfd, serial_file_descriptor, sabre_to_socket_buf, suboptcode, value);
		break;
	case CPC_PURGE_DATA_C2S:
	case CPC_PURGE_DATA_S2C:
		ret = respond_telnet_cpc_purge_data_subopt(sockfd, serial_file_descriptor, socket_to_serial_buf, sabre_to_socket_buf, suboptcode, value);
		break;
#endif
	default:
		break;
	}
	return(ret);
}


/*
	Location: telnet.c
	This function is called when the IAC char is detected in the data that was read from the modem/serial file descriptor.
	For every IAC char in the buffer, we "escape" it by inserting an additional IAC char.
*/
void escape_iac_chars(BUFFER *serial_to_socket_buf)
{
	unsigned char *ptr;
	unsigned char *iac;
	unsigned char *se;
	unsigned char optcode;
	unsigned char option;
	int done;
	int size;

	/* sanity checks */
	if (serial_to_socket_buf == NULL) return;

	/* Ptr points to its "active" portion and size holds the number of chars buffered.
 	   size is number of bytes of active portion that we are dealing with.	*/
	ptr = bf_point_to_active_portion(serial_to_socket_buf, &size);

	/* get pointer to IAC char */
	iac = mystrchr(ptr, IAC, size);
	if (iac == NULL) return;

	write_to_debuglog(DBG_VINF,"telnet.c: escaping IAC chars received from serial");

	/* adjust size */
	size -= (iac - ptr);

	done = 0;
	while (! done) {
		/* insert another IAC char */
		bfinsch(serial_to_socket_buf, iac, IAC);
		size++;						/* account for added char */

		/* point past the sequence of 2 IAC chars */
		iac += 2;
		size -= 2;

		/* find next IAC char */
		if (size > 0) {
			ptr = iac;
			iac = mystrchr(ptr, IAC, size);
			if (iac == NULL) {
				++done;
			} else {
				/* adjust size */
				size -= (iac - ptr);
			}
		} else {
			++done;
		}
	}
}


/*
	Location: telnet.c
	mark our telnet options structure to indicate that the specified
	option has been negotiated, client to server.
*/
void enable_telnet_client_option(unsigned char option)
{
	extern TELNET_OPTIONS tnoptions[];

	tnoptions[option].client = 1;
}

/*
	Location: telnet.c
	mark our telnet options structure to indicate that the specified
	option has not been negotiated, client to server.
*/
void disable_telnet_client_option(unsigned char option)
{
	extern TELNET_OPTIONS tnoptions[];
	tnoptions[option].client = 0;
}


/*
	Location: telnet.c
	is the specified telnet option disabled, client to server?
*/
int telnet_client_option_is_disabled(unsigned char option)
{
	return(! telnet_client_option_is_enabled(option));
}


/*
	Location: telnet.c
	This is to mark our telnet options structure to indicate that the specified
	option has been negotiated, server to client.
*/
void enable_telnet_server_option(unsigned char option)
{
	extern TELNET_OPTIONS tnoptions[];

	tnoptions[option].server = 1;
}

/*
	Location: telnet.c
	This is to mark our telnet options structure to indicate that the specified
	option has not been negotiated, server to client.
*/
void disable_telnet_server_option(unsigned char option)
{
	extern TELNET_OPTIONS tnoptions[];
	tnoptions[option].server = 0;
}

/*
	Location: telnet.c
	is the specified telnet option enabled, server to client?
*/
int telnet_server_option_is_enabled(unsigned char option)
{
	extern TELNET_OPTIONS tnoptions[];
	int enabled;

	enabled = tnoptions[option].server;
	return(enabled);
}


/*
	Location: telnet.c
	This is to respond to the optcode+option that we've received from the client
*/
int respond_telnet_binary_option(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char optcode, unsigned char option)
{
	extern int tnmode[];
	int ret;
	/* just in case */
	if (option != TELOPT_BINARY)
	{
		return(respond_known_telnet_option(sockfd, sabre_to_socket_buf, optcode, option));
	}
	ret = 0;
	switch (optcode)
	{
		case WILL:											/* server DO, client WILL */
		if (telnet_client_option_is_disabled(option))
		{	/* prevent option loop */
			ret = send_telnet_option(sockfd, sabre_to_socket_buf, DO, option);
			enable_telnet_client_option(option);		/* server <<== client */
			if (tnmode[CLIENT] != BINARY)
			{
				syslog(LOG_INFO,"telnet.c: telnet connection is now in BINARY mode (server <<== client)");
				tnmode[CLIENT] = BINARY;
			}
		}
		break;
	case WONT:											/* server DO, client WONT */
		if (telnet_client_option_is_enabled(option))
		{	/* prevent option loop */
			ret = send_telnet_option(sockfd, sabre_to_socket_buf, DONT, option);
			disable_telnet_client_option(option);
			if (tnmode[CLIENT] != ASCII)
			{
				syslog(LOG_INFO,"telnet.c: telnet connection is now in ASCII mode (server <<== client)");
				tnmode[CLIENT] = ASCII;
			}
		}
		break;
	case DO:											/* server WILL, client DO */
		if (telnet_server_option_is_disabled(option))
		{	/* prevent option loop */
			ret = send_telnet_option(sockfd, sabre_to_socket_buf, WILL, option);
			enable_telnet_server_option(option);		/* server ==>> client */
			if (tnmode[SERVER] != BINARY)
			{
				syslog(LOG_INFO,"telnet.c: telnet connection is now in BINARY mode (server ==>> client)");
				tnmode[SERVER] = BINARY;
			}
		}
		break;
	case DONT:											/* server WILL, client DONT */
		if (telnet_server_option_is_enabled(option)) {	/* prevent option loop */
			ret = send_telnet_option(sockfd, sabre_to_socket_buf, WONT, option);
			disable_telnet_server_option(option);
			if (tnmode[SERVER] != ASCII) {
				syslog(LOG_INFO,"telnet.c: telnet connection is now in ASCII mode (server ==>> client)");
				tnmode[SERVER] = ASCII;
			}
		}
		break;
	default:
		break;
	}
	return(ret);
}

/*
	Location: telnet.c
	This is to respond to the optcode+option that we've received from the client
*/
int respond_known_telnet_option(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char optcode, unsigned char option)
{
	int ret;
	/* just in case */
	if (option == TELOPT_BINARY)
	{
		return(respond_telnet_binary_option(sockfd, sabre_to_socket_buf, optcode, option));
	}
	ret = 0;
	switch (optcode)
	{
	case WILL:														/* server DO, client WILL */
		if (telnet_client_option_is_disabled(option))				/* prevent option loop */
		{
			ret = send_telnet_option(sockfd, sabre_to_socket_buf, DO, option);
			enable_telnet_client_option(option);						/* server <<== client */
		}
		break;
	case WONT:														/* server DO, client WONT */
		if (telnet_client_option_is_enabled(option)) 					/* prevent option loop */
		{
			ret = send_telnet_option(sockfd, sabre_to_socket_buf, DONT, option);
			disable_telnet_client_option(option);
		}
		break;
	case DO:															/* server WILL, client DO */
		if (telnet_server_option_is_disabled(option)) 					/* prevent option loop */
		{
			ret = send_telnet_option(sockfd, sabre_to_socket_buf, WILL, option);
			enable_telnet_server_option(option);						/* server ==>> client */
		}
		break;
	case DONT:														/* server WILL, client DONT */
		if (telnet_server_option_is_enabled(option))					/* prevent option loop */
		{
			ret = send_telnet_option(sockfd, sabre_to_socket_buf, WONT, option);
			disable_telnet_server_option(option);
		}
		break;
	default:
		break;
	}
	return(ret);
}

/*
	Location: telnet.c
	This is to send a response to a telnet option request.
	returns 0 on success, 1 on failure
*/
int respond_telnet_option(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char optcode, unsigned char option)
{
	extern int errno;
	extern int client_logged_in;		/* is the client still "logged in"? */
	int ret = 0;
	unsigned char answer;

	syslog(LOG_INFO, "telnet.c: response_t_o(): received telnet option %s %s",
			telnet_optcode2str(optcode), telnet_option2str(option));

	switch (option)
	{
		case TELOPT_COM_PORT_OPTION:
		case TELOPT_ECHO:
		case TELOPT_SGA:
			ret = respond_known_telnet_option(sockfd, sabre_to_socket_buf, optcode, option);
			break;
		case TELOPT_LOGOUT:
			ret = respond_known_telnet_option(sockfd, sabre_to_socket_buf, optcode, option);
			if ((optcode == WILL) || (optcode == DO))
			{
				client_logged_in = 0;									/* client and server agree to the logout */
			}
			break;
		case TELOPT_BINARY:
			ret = respond_telnet_binary_option(sockfd, sabre_to_socket_buf, optcode, option);
			break;
		default:
			switch (optcode)
			{
				case WILL:
				case WONT:
					answer = DONT;
					break;
				case DO:
				case DONT:
					answer = WONT;
					break;
			}
			/* send a negative acknowledgement */
			ret = send_telnet_option(sockfd, sabre_to_socket_buf, answer, option);
			break;
	}
	return(ret);
}

/*
	Location: telnet.c
	This function regards to the progress of processing the networking socket file descriptor which interacts with client.
	This function is called when the IAC char is detected in the data that was read from the socket.
	If the buffer contains one or more telnet option strings, then we act on them and remove them from
	the buffer (so they won't be passed on to the serial port).
*/
void process_telnet_options(int sockfd, int serial_file_descriptor, BUFFER *socket_to_serial_buf, BUFFER *sabre_to_socket_buf)
{
	unsigned char *ptr;
	unsigned char *iac;
	unsigned char *se;
	unsigned char optcode;
	unsigned char option;
	int done;
	int size;

	/* sanity checks */
	if (socket_to_serial_buf == NULL) return;
	if (sabre_to_socket_buf == NULL) return;
	/* Ptr points to its "active" portion and size holds the number of chars buffered.
	 * size is number of bytes of active portion that we are dealing with.	*/
	ptr = bf_point_to_active_portion(socket_to_serial_buf, &size);

	/* get pointer to IAC char */
	iac = mystrchr(ptr, IAC, size);									/* iac points to the begginning of fisrt IAC character*/
	if (iac == NULL) return;

	write_to_debuglog(DBG_VINF, "telnet.c: p_t_o(): processing telnet options received from client");

	/* compute size of a IAC character: iac-ptr. Then remove it by using size- (iac-ptr) */
	size -= (iac - ptr);
	if (size < 2) return;												/* 2 is the minimum telnet command length */

	done = 0;															/* Processing flag.*/
	while (! done) {

		/* decide what to do next, based upon the char following the IAC */
		optcode = (unsigned char) *(iac + 1);							/* optcode points to next option code following IAC.*/
		switch (optcode) {
		case IAC:													/* a double IAC */
			bfrmstr(socket_to_serial_buf, iac, 1);						/* remove one IAC char */
			iac++;														/* point past remaining IAC */
			size -= 2;													/* remove 2 IAC, and set new size.*/
			break;
		case WILL:													/* WILL, DO, DONT, WONT */
		case DO:
		case DONT:
		case WONT:
			option = (unsigned char) *(iac + 2);
			respond_telnet_option(sockfd, sabre_to_socket_buf, optcode, option);
			bfrmstr(socket_to_serial_buf,iac,3);		/* remove those 3 bytes */
			size -= 3;
			break;
		case SB:						/* sub-option */
			option = (unsigned char) *(iac + 2);
			if ((option == TELOPT_COM_PORT_OPTION) && (telnet_client_option_is_enabled(option))) {
				process_telnet_cpc_suboption(sockfd, serial_file_descriptor, socket_to_serial_buf, sabre_to_socket_buf, iac+3, size-3);
			} else {
				syslog(LOG_ERR,"telnet.c: ignoring telnet suboption negotiations for %s", telnet_option2str(option));
			}
			se = mystrchr(iac+3, SE, size-3);	/* find SE char */
			if (se != NULL) {
				bfrmstr(socket_to_serial_buf, iac, (se - iac + 1));	/* remove telnet options str from buffer */
				size -= (se - iac + 1);
			} else {
				iac += 3;
				size -= 3;
			}
			break;
		case BREAK:
		default:						/* invalid char after IAC.  RFC854 says treat it as NOP. */
			syslog(LOG_ERR, "telnet.c: p_t_opt():ignoring telnet %s command", telnet_optcode2str(optcode));
			bfrmstr(socket_to_serial_buf, iac, 2);		/* remove those 2 bytes */
			size -= 2;
			break;
		}
		if (size < 2)
		{					/* 2 is the minimum telnet command length */
			++done;
		}
		/* find next IAC char */
		if (! done) {
			ptr = iac;
			iac = mystrchr(ptr,IAC,size);
			if (iac == NULL) {
				++done;
			} else {
				/* adjust size */
				size -= (iac - ptr);
				if (size < 2) {			/* 2 is the minimum telnet command length */
					++done;
				}
			}
		}
	}
}
/*
	Location: telnet.c
	This is to check if is the specified telnet option enabled, client to server?
*/
int telnet_client_option_is_enabled(unsigned char option)
{
	extern TELNET_OPTIONS tnoptions[];
	int enabled;
	enabled = tnoptions[option].client;
	return(enabled);
}

/*
	Location: telnet.c
	is the specified telnet option disabled, server to client?
*/
int telnet_server_option_is_disabled(unsigned char option)
{
	return(! telnet_server_option_is_enabled(option));
}

/*
	Location: telnet.c
	This is to send a telnet option. Formation: IAC optcode option.
	returns 0 on success, 1 on failure
*/
int send_telnet_option(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char optcode, unsigned char option)
{
	extern int errno;
	extern int session_state;
	unsigned char optstr[4];
	int size;
	int ret;

	/* make sure we only send it once */
	if (telnet_option_was_sent(optcode, option)) {
		syslog(LOG_INFO,"telnet option %s %s already sent", telnet_optcode2str(optcode), telnet_option2str(option));
		return(0);
	}

	/* build telnet option string */
	optstr[0] = IAC;
	optstr[1] = optcode;
	optstr[2] = option;
	optstr[3] = '\0';
	size = 3;

	/* append it to the buffer */
	bfstrncat(sabre_to_socket_buf, (const char*) optstr, size);

	/* send it */
	if (session_state == RESUME) {
		bfdump(sabre_to_socket_buf, 0);		/* debugging dump of BUFFER */
		ret = write_from_buffer_to_fd(sockfd, sabre_to_socket_buf);
		if (ret >= size) {		/* buffer may have contained more than just our optstr */
			mark_telnet_option_as_sent(optcode, option);
			syslog(LOG_INFO,"telnet.c: s_t_o(): sent telnet option %s %s",
					telnet_optcode2str(optcode), telnet_option2str(option));
			return(0);
		} else {
			syslog(LOG_ERR,"telnet.c: s_t_o(): error sending telnet option %s %s",
					telnet_optcode2str(optcode), telnet_option2str(option));
			return(1);
		}
	}
	return(0);
}

/*
	Location: telnet.c
	This is to mark our telnet options structure to indicate that the specified
	opt code for the specified option has been sent to client.
*/
void mark_telnet_option_as_sent(unsigned char optcode, unsigned char option)
{
	extern TELNET_OPTIONS tnoptions[];

	switch (optcode) {
	case WILL:
		tnoptions[option].sent_will = 1;
		break;
	case DO:
		tnoptions[option].sent_do = 1;
		break;
	case WONT:
		tnoptions[option].sent_wont = 1;
		break;
	case DONT:
		tnoptions[option].sent_dont = 1;
		break;
	default:
		syslog(LOG_ERR,"telnet.c: mark_telnet_option: unknown telnet option code: %d", (int) optcode);
		break;
	}
}

/*
	Location: telnet.c
	returns 1 if the specified optcode was already sent for	the specified option. Otherwise it returns 0.
*/
int telnet_option_was_sent(unsigned char optcode, unsigned char option)
{
	extern TELNET_OPTIONS tnoptions[];
	int status;

	switch (optcode) {
	case WILL:
		status = tnoptions[option].sent_will;
		break;
	case DO:
		status = tnoptions[option].sent_do;
		break;
	case WONT:
		status = tnoptions[option].sent_wont;
		break;
	case DONT:
		status = tnoptions[option].sent_dont;
		break;
	default:
		status = 0;
		break;
	}
	return(status);
}

/*
	Location: telnet.c
	This is to send a initial telnet option for negotiation.
	returns 0 on success, 1 on failure
*/
int send_init_telnet_option(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char optcode, unsigned char option)
{
	extern int errno;
	extern int session_state;
	unsigned char optstr[4];
	int size;
	int ret;

	/* make sure we only send it once */
	if (telnet_option_was_sent(optcode, option)) {
		syslog(LOG_INFO,"telnet.c: send_init_telnet_option(): telnet option %s %s already sent",
				telnet_optcode2str(optcode), telnet_option2str(option));
		return(0);
	}
	/* build telnet option string */
	optstr[0] = IAC;
	optstr[1] = optcode;
	optstr[2] = option;
	optstr[3] = '\0';
	size = 3;

	/* append it to the buffer */
	bfstrncat(sabre_to_socket_buf, (const char*) optstr, size);

	/* send it */
	if (session_state == RESUME)
	{
		bfdump(sabre_to_socket_buf, 0);		/* debugging dump of BUFFER */
		ret = write_from_buffer_to_fd(sockfd, sabre_to_socket_buf);
		if (ret >= size) {		/* buffer may have contained more than just our optstr */
			mark_telnet_option_as_sent(optcode,option);
			syslog(LOG_INFO,"telnet.c: send_init_telnet_option(): sent telnet option %s %s",
					telnet_optcode2str(optcode), telnet_option2str(option));
			return(0);
		} else {
			syslog(LOG_ERR, "telnet.c: send_init_telnet_option(): error sending telnet option %s %s",
					telnet_optcode2str(optcode), telnet_option2str(option));
			return(1);
		}
	}
	return(0);
}

/*
	Location: telnet.c
	This function is to:
	- init's the telnet options structure
	- init's some global variables related to telnet options
	- sends initial telnet options: Com Port Control, Binary, etc.
	it always returns 0.
*/
int telnet_init(int sockfd, BUFFER *sabre_to_socket_buf)
{
	extern TELNET_OPTIONS tnoptions[];
	extern int tnmode[];
	extern int session_state;
	extern int break_signaled;
	extern int ask_client_signature;
	int i;
	/*
		init all telnet options to "off"
	*/
	for (i = 0; i < MAX_TELNET_OPTIONS; i++) {
		tnoptions[i].sent_will = 0;
		tnoptions[i].sent_do = 0;
		tnoptions[i].sent_wont = 0;
		tnoptions[i].sent_dont = 0;
		tnoptions[i].server = 0;
		tnoptions[i].client = 0;
	}
	/* initially we're in ASCII mode */
	tnmode[CLIENT] = ASCII;
	tnmode[SERVER] = ASCII;
	/* init other global variables */
	session_state = RESUME;
	break_signaled = 0;
	ask_client_signature = 1;
	/*
		send initial telnet option negotiations
	*/
	send_init_telnet_option(sockfd, sabre_to_socket_buf, DO,   TELOPT_COM_PORT_OPTION);
	send_init_telnet_option(sockfd, sabre_to_socket_buf, WILL, TELOPT_BINARY);
	send_init_telnet_option(sockfd, sabre_to_socket_buf, DO,   TELOPT_BINARY);
	send_init_telnet_option(sockfd, sabre_to_socket_buf, WILL, TELOPT_ECHO);
	send_init_telnet_option(sockfd, sabre_to_socket_buf, WILL, TELOPT_SGA);
	send_init_telnet_option(sockfd, sabre_to_socket_buf, DO,   TELOPT_SGA);
	return(0);
}







