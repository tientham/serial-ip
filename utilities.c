/*
 * utilities.c
 *	This is to collect useful functions throughout my software.
 *  Created on: Aug 3, 2015
 *      Author: tientham
 */

#include "serial_ip.h"

/*
    Location: utilities.c
    This is to translate from a telnet parity code to its string form.
*/
char *telnet_cpc_parity2str(unsigned char optcode)
{
	static char unknown[16];
	char *str;
	char *p;

	switch (optcode) {
	case CPC_PARITY_NONE:
		str = "None";
		break;
	case CPC_PARITY_ODD:
		str = "Odd";
		break;
	case CPC_PARITY_EVEN:
		str = "Even";
		break;
	case CPC_PARITY_MARK:
		str = "Mark";
		break;
	case CPC_PARITY_SPACE:
		str = "Space";
		break;
	default:
		strcpy(unknown,"Code xxx");
		p = strstr(unknown,"xxx");
		snprintf(p,3,"%d",optcode);
		str = unknown;
		break;
	}
	return(str);
}

/*
    Location: utilities.c
    This is to translate from a telnet data size code to its string form.
*/
char *telnet_cpc_datasize2str(unsigned char datasize)
{
	char *str;

	switch (datasize) {
	case CPC_DATASIZE_CS8:
		str = "CS8";
		break;
	case CPC_DATASIZE_CS7:
		str = "CS7";
		break;
	case CPC_DATASIZE_CS6:
		str = "CS6";
		break;
	case CPC_DATASIZE_CS5:
		str = "CS5";
		break;
	default:
		str = "CS8";
		break;
	}
	return(str);
}

/*
    Location: utilities.c
    This is to translate from a telnet sub-option code to its string form.
*/
char *telnet_cpc_subopt2str(unsigned char suboptcode)
{
	static char unknown[16];
	char *str;
	char *p;

	switch (suboptcode) {
	case CPC_SIGNATURE_C2S:
		str = "Signature C2S";
		break;
	case CPC_SIGNATURE_S2C:
		str = "Signature S2C";
		break;
	case CPC_SET_BAUDRATE_C2S:
		str = "Set Baudrate C2S";
		break;
	case CPC_SET_BAUDRATE_S2C:
		str = "Set Baudrate S2C";
		break;
	case CPC_SET_DATASIZE_C2S:
		str = "Set Datasize C2S";
		break;
	case CPC_SET_DATASIZE_S2C:
		str = "Set Datasize S2C";
		break;
	case CPC_SET_PARITY_C2S:
		str = "Set Parity C2S";
		break;
	case CPC_SET_PARITY_S2C:
		str = "Set Parity S2C";
		break;
	case CPC_SET_STOPSIZE_C2S:
		str = "Set Stopsize C2S";
		break;
	case CPC_SET_STOPSIZE_S2C:
		str = "Set Stopsize S2C";
		break;
	case CPC_SET_CONTROL_C2S:
		str = "Set Control C2S";
		break;
	case CPC_SET_CONTROL_S2C:
		str = "Set Control S2C";
		break;
	case CPC_NOTIFY_LINESTATE_C2S:
		str = "Notify Linestate C2S";
		break;
	case CPC_NOTIFY_LINESTATE_S2C:
		str = "Notify Linestate S2C";
		break;
	case CPC_NOTIFY_MODEMSTATE_C2S:
		str = "Notify Modemstate C2S";
		break;
	case CPC_NOTIFY_MODEMSTATE_S2C:
		str = "Notify Modemstate S2C";
		break;
	case CPC_FLOWCONTROL_SUSPEND_C2S:
		str = "Suspend Flowcontrol C2S";
		break;
	case CPC_FLOWCONTROL_SUSPEND_S2C:
		str = "Suspend Flowcontrol S2C";
		break;
	case CPC_FLOWCONTROL_RESUME_C2S:
		str = "Resume Flowcontrol C2S";
		break;
	case CPC_FLOWCONTROL_RESUME_S2C:
		str = "Resume Flowcontrol S2C";
		break;
	case CPC_SET_LINESTATE_MASK_C2S:
		str = "Set Linestate Mask C2S";
		break;
	case CPC_SET_LINESTATE_MASK_S2C:
		str = "Set Linestate Mask S2C";
		break;
	case CPC_SET_MODEMSTATE_MASK_C2S:
		str = "Set Modemstate Mask C2S";
		break;
	case CPC_SET_MODEMSTATE_MASK_S2C:
		str = "Set Modemstate Mask S2C";
		break;
	case CPC_PURGE_DATA_C2S:
		str = "Purge Data C2S";
		break;
	case CPC_PURGE_DATA_S2C:
		str = "Purge Data S2C";
		break;
	default:
		strcpy(unknown,"Code xxx");
		p = strstr(unknown,"xxx");
		snprintf(p,3,"%d",suboptcode);
		str = unknown;
		break;
	}
	return(str);
}

/*
	Location: utilities.c
	This is to point input string to a specific character.
	Return the first occurence of char in string when succesfull.
	Otherwise, return NULL.
*/

unsigned char *mystrchr(unsigned char *s, int ch, int size)
{
	while (size > 0) {
		if (*s == (unsigned char) ch)
		{
			return(s);
		}
		size--;
		s++;
	}
	return(NULL);
}

/*
	Location: utilities.c
	This is to convert the telnet stop size to string.
	Return equivalent string.
*/
char *telnet_cpc_stopsize2str(unsigned char optcode)
{
	static char unknown[16];
	char *str;
	char *p;

	switch (optcode) {
	case CPC_STOPSIZE_1BIT:
		str = "1";
		break;
	case CPC_STOPSIZE_2BITS:
		str = "2";
		break;
	case CPC_STOPSIZE_15BITS:
		str = "1.5";
		break;
	default:
		strcpy(unknown,"Code xxx");
		p = strstr(unknown,"xxx");
		snprintf(p,3,"%d",optcode);
		str = unknown;
		break;
	}
	return(str);
}



/*
	Location: utilities.c
	This is to convert the telnet option code (optcode) to string.
	Return equivalent string.
*/
char *telnet_optcode2str(unsigned char optcode)
{
	char *string;

	switch (optcode) {
	case IAC:
		string = "IAC";
		break;
	case DONT:
		string = "DONT";
		break;
	case DO:
		string = "DO";
		break;
	case WONT:
		string = "WONT";
		break;
	case WILL:
		string = "WILL";
		break;
	case BREAK:
		string = "BREAK";
		break;
	case SB:
		string = "SB";
		break;
	default:
		string = "UNKNOWN";
		break;
	}
	return(string);
}
/*
	Location: utilities.c
	This is to convert the telnet option to string.
	Return equivalent string.
*/
char *telnet_option2str(unsigned char option)
{
	static char unknown[16];
	char *string;
	char *p;

	switch (option) {
	case TELOPT_BINARY:
		string = "Binary Transmission";
		break;
	case TELOPT_ECHO:
		string = "Echo";
		break;
	case TELOPT_SGA:
		string = "Suppress Go Ahead";
		break;
	case TELOPT_COM_PORT_OPTION:
		string = "Com Port Control";
		break;
	case TELOPT_KERMIT:
		string = "Kermit";
		break;
	case TELOPT_LOGOUT:
		string = "Logout";
		break;
	default:
		strcpy(unknown,"Option xxx");
		p = strstr(unknown,"xxx");
		snprintf(p,3,"%d",option);
		strcat(unknown,p);
		string = unknown;
		break;
	}
	return(string);
}

/*
	Location: utilities.c
	This is to convert the number of data bits, specified as an integer, to the corresponding CSn symbol.
*/
unsigned long uint2cs(unsigned int data_bits)
{
	unsigned long cs;

	switch (data_bits)
	{
	case 8:
		cs = CS8;
		break;
	case 7:
		cs = CS7;
		break;
	case 6:
		cs = CS6;
		break;
	case 5:
		cs = CS5;
		break;
	default:
		cs = CS8;
		break;
	}
	return(cs);
}

/*
	Location: utilities.c
	This function is to convert a baud rate symbol to its correspsonding unsigned long speed.
	By default: a speed of 9600 will be returned.
*/
unsigned long speed_t2ulong(speed_t s)
{
	unsigned long speed;

	switch (s) {
	case B0:
		speed = 0;
		break;
	case B50:
		speed = 50;
		break;
	case B75:
		speed = 75;
		break;
	case B110:
		speed = 110;
		break;
	case B134:
		speed = 134;
		break;
	case B150:
		speed = 150;
		break;
	case B200:
		speed = 200;
		break;
	case B300:
		speed = 300;
		break;
	case B600:
		speed = 600;
		break;
	case B1200:
		speed = 1200;
		break;
	case B1800:
		speed = 1800;
		break;
	case B2400:
		speed = 2400;
		break;
	case B4800:
		speed = 4800;
		break;
	case B9600:
		speed = 9600;
		break;
	case B19200:
		speed = 19200;
		break;
	case B38400:
		speed = 38400;
		break;
#ifdef B57600
	case B57600:
		speed = 57600;
		break;
#endif
#ifdef B115200
	case B115200:
		speed = 115200;
		break;
#endif
#ifdef B230400
	case B230400:
		speed = 230400;
		break;
#endif
#ifdef B460800
	case B460800:
		speed = 460800;
		break;
#endif
	default:
		speed = 9600;
		break;
	}
	return(speed);
}

/*
	Location: utilities.c
	This function is to convert an unsigned long speed to its correspsonding
	baud rate symbol.
*/
speed_t ulong2speed_t(unsigned long s)
{
	speed_t speed;

	switch (s) {
	case 0:
		speed = B0;
		break;
	case 50:
		speed = B50;
		break;
	case 75:
		speed = B75;
		break;
	case 110:
		speed = B110;
		break;
	case 134:
		speed = B134;
		break;
	case 150:
		speed = B150;
		break;
	case 200:
		speed = B200;
		break;
	case 300:
		speed = B300;
		break;
	case 600:
		speed = B600;
		break;
	case 1200:
		speed = B1200;
		break;
	case 1800:
		speed = B1800;
		break;
	case 2400:
		speed = B2400;
		break;
	case 4800:
		speed = B4800;
		break;
	case 9600:
		speed = B9600;
		break;
	case 19200:
		speed = B19200;
		break;
	case 38400:
		speed = B38400;
		break;
#ifdef B57600
	case 57600:
		speed = B57600;
		break;
#endif
#ifdef B115200
	case 115200:
		speed = B115200;
		break;
#endif
#ifdef B230400
	case 230400:
		speed = B230400;
		break;
#endif
#ifdef B460800
	case 460800:
		speed = B460800;
		break;
#endif
	default:
		speed = B9600;
		break;
	}
	return(speed);
}

/*
  	  Location: utilities.c
  	  This function is to get current time in string type
*/
char* get_timestamp()
{
	time_t now = time(NULL);
	return asctime(localtime(&now));
}

/*
  	  Location: utilities.c
  	  This function works like sleep(). It returns 0 if success, otherwise -1 if failed.
  	  The last argument 'tv' is to set timeout.
*/
int msleep(unsigned long micro_second)
{
	struct timeval tv;

	tv.tv_sec = micro_second / 1000000L;
	tv.tv_usec = micro_second % 1000000L;

	return(select(0,NULL,NULL,NULL,&tv));
}

/*
 * 	Location: utilities.c
 *	Remove a feed new line '\n' on input string
*/
char* remove_feedline_from_string(char* str)
{
	char* s;
	if((s=strchr(str, (int)'\n')) != NULL)
	{
		*s = '\0';
		return s;
	}
	return str;
}

/*
 * 	Location: utilities.c
 *	Remove a cariage return '\r' on input string
*/
char* remove_carriage_return_from_string(char* str)
{
	char* s;
	if((s=strchr(str, (int)'\r')) != NULL)
	{
		*s = '\0';
		return s;
	}
	return str;
}

/*
	Location: utilities.c
	skip "white space" characters at the beginning of the buffer.
	Returns a pointer to the first non-space character.
*/
char *skip_white_space(char *str)
{
	while ((*str != '\0') && (isspace(*str)))
		++str;
	return(str);
}

/*
 * 	Location: utilities.c
 *	This is to retrive from serial_ip.conf a configuration name to put into hash entry table.
 *	The reason is that there are some spaces in a line which has no meaning.
*/
char *retrieve_configuration_entry_name(char *s)
{
	char *p;
	int len;

	len = strlen(s);
	if (len < 1)
		return(s);						/* empty string */

	p = s + (len - 1);					/* point to end of string */
	while ((isspace(*p)) && (len > 0)) {
		--p;
		--len;
	}
	++p;
	*p = '\0';
	return(s);
}

/*
     Location: utilities.c
 	 This function is to get the program name from the argument input.
 	 Formation of input: ./<program_name>.
 	 Check and sift backwards to find full program_name.
 */
char *get_program_name(char* str)
{
	char* p;

	p = str + (strlen(str)) -1;			/*	Point to the end of string.*/

	while((*p != '/') && (p!=str))
		p--;

	if(*p == '/')
		++p;
	return p;
}

/*
	Location: utilities.c
	This function is to set an input string to lower case.
	Does not return.
*/
void string_to_lower(char *str)
{
	int c;
	char *p;

	if (str == NULL) return;

	p = str;
	while (*p != '\0') {
		c = tolower((int) *p);
		*p++ = (char) c;
	}
}

/*
	Location: utilities.c
	This function is change token type to string.
	Do not return.
	when adding a token type, we also need to adding on serial_ip.h.
*/
char *token_type_to_str(int tokentype)
{
	char *str;

	switch (tokentype) {
	case LONGVALUE:
		str = "number";
		break;
	case BOOLEAN:
		str = "boolean";
		break;
	case PRINTFSTRING:
		str = "printf string";
		break;
	case STRING:
		str = "string";
		break;
	case VALUE:
		str = "number";
		break;
	default:
		str = "unknown";
		break;
	}
	return(str);
}

/*
	Location: utilities.c
	We need this function to handle various kind of input value arguments from users.
	For instance, a boolean "no" can interpret as "0", "off", "false".
	and a boolean "yes" can interpret as "1", "on", "true", "yes".
	We make a convention that:
	boolean "no" returns 0.
	boolean "yes" returns 1.
	Otherwise, returns -1 as fall.
*/
int boolean_string_handle(char *value)
{
	int ret = -1;							/* return value. Default, -1. */
	int i,j;
	char *str;							/* ptr to copy of value */
	char *word_options[2][5] = {
		{ "0", "off", "false", "no",  NULL },
		{ "1", "on",  "true",  "yes", NULL }
	};

	str = strdup(value);				/* make a copy of value. We don't need to malloc before using strdup. */
	if (str != NULL) {
		char *p = str;
		while (*p != '\0') {
			*p = tolower(*p);			/* lower case the value */
			p++;
		}
		for (i = 0; i < 2; i++) {
			for (j = 0; word_options[i][j] != NULL; j++) {
				if (strcmp(str,word_options[i][j]) == 0) {
					ret = i;		/* use 0/1 array index as return value */
					break;
				}
			}
			if (ret != -1) break;
		}
		free(str);						/* release memory */
	}
	return(ret);
}

/*
	Location: utilities.c
	This function is to save the value associated with serial_ip.conf file.
	Return error status (0 on success, 1 on failed).
	Source code: telnetcpcd project.
*/
int save_value(char *value, int type, void *target)
{
	int error = 0;					/* Error flag.*/
	char **str;
	int *i;
	unsigned long *u;

	switch (type) {
	case STRING:
	case PRINTFSTRING:
		str = (char **) target;			/* ptr cast */
		if (*str == NULL)
			*str = malloc(strlen(value) + 1);
		if (*str != NULL) {
			if (type == STRING) {
				strcpy(*str,value);		/* regular string, copy it. Also update value of target after this action.*/
			} else {					/* else, printf string */
				str= (char **) escape_sequence_copy(*str, value);	/* allow character escapes,
															also update value of target after this action. */
			}
		} else {
			syslog(LOG_ERR,"cannot malloc space for string %s",value);
			error = 1;
		}
		break;
	case VALUE:
		i = (int *) target;				/* ptr cast */
		*i = atoi(value);				/* copy value */
		break;
	case BOOLEAN:
		i = (int *) target;				/* ptr cast */
		*i = boolean_string_handle(value);			/* boolean string to integer */
		if (*i == -1) error = 1;
		break;
	case LONGVALUE:
		u = (unsigned long *) target;	/* ptr cast */
		*u = atol(value);				/* copy value */
		break;
	default:							/* unknown */
		syslog(LOG_ERR,"bad token type passed to save_value(): %d",type);
		error = 1;
		break;
	}
	return(error);
}

/*
	Location: utilities.c
	This function is to validate a specified speed/baud rate given from user.
	Success return 1; otherwise it returns 0.
*/
int valid_serial_speed(unsigned long speed)
{
	int valid_speed;

	switch (speed) {
	case 0:
	case 50:
	case 75:
	case 110:
	case 134:
	case 150:
	case 200:
	case 300:
	case 600:
	case 1200:
	case 2400:
	case 4800:
	case 9600:
	case 19200:
	case 38400:
	case 57600:
	case 115200:
	case 230400:
	case 460800:
		valid_speed = 1;
		break;
	default:
		valid_speed = 0;
		break;
	}
	return(valid_speed);
}

/*
	Location: utilities.c
	Parse the buffer, which contains an line of serial_ip.conf file. If this line contains a valid keyword, we copy its
	information including name and value to the entry_table. After this function, we already update input entry_table.
	returns the keyword token, UNKNOWN, or INVALID
*/
unsigned long parse_entry(char *buffer,struct config_entry *entry_table)
{
	char *p;
	char *s;
	int c;
	ENTRY *node;						/* ptr to hash table entry */
	ENTRY item;							/* hash table entry */
	struct config_entry *entry;				/* keyword table entry */


	s = remove_feedline_from_string(buffer);
	s = remove_carriage_return_from_string(buffer);
	p = skip_white_space(buffer);
	if (strlen(p) <= 0)					/* blank line */
		return(BLANKLINE);
	if (strchr("#",(int) *p) != NULL)	/* comment */
		return(COMMENT);
	if (strchr(";",(int) *p) != NULL)	/* comment */
		return(COMMENT);
	s = p;								/* save ptr to keyword */

	while ((*p != '=') && (*p != '\0'))
		++p;							/* search for '=' */
	if (*p == '\0')
		return(UNKNOWN);
	/* p now points to '=' */
	if (s == p)							/* missing parameters */
		return(UNKNOWN);

	/* We are now going to set value for each parameter of our hash entry_table. */
	memset(entry_table->name,(int) ' ',SIZE_KEYWORD);
	c = p - s;							/* number of chars */
	if (c >= SIZE_KEYWORD)				/* bounds check */
		c = SIZE_KEYWORD - 1;
	strncpy(entry_table->name,s,c);
	entry_table->name[c] = '\0';					/* null terminate */
	retrieve_configuration_entry_name(entry_table->name);	/* retrieve hash entry table name. */
	string_to_lower(entry_table->name);				/* fold to lower case */
	syslog(LOG_DEBUG,"keyword \"%s\"",entry_table->name);

	/*
		look up this entry's keyword in the hash table to see if
		it's valid.  if it is valid, this gives us the keyword type.
		if it is not valid, we use the STRING keyword type.
	*/
	item.key = entry_table->name;
	node = hsearch(item,FIND);      /* Make a comparision between node - the hash table already built, and item - which has
	 	 	 	 	 	 	 	 	 	 its "key" being one of our parameter from serial_ip,conf.
	 	 	 	 	 	 	 	 	 	 This is to check if the keyword for parameter is correct/match
	 	 	 	 	 	 	 	 	 	 or incorrect with our built hash keyword table..*/
	if (node != NULL) {
		/* node.data can be used as a ptr to struct config_entry */
		entry = (struct config_entry *) node->data;				/*For an ENTRY structure, key is the name of hash table.
																	But, data is one row of the hash table.
																	(in our case, it includes (name | token | type | value)).*/
		entry_table->type = entry->type;			/* keyword token type */
		entry_table->token = entry->token;			/* keyword token */
	} else {
		entry_table->type = STRING;				/* keyword token type */
		entry_table->token = UNKNOWN;			/* keyword token */
	}
	syslog(LOG_DEBUG,"  token type: %s",token_type_to_str(entry_table->type));
	syslog(LOG_DEBUG,"  token 0x%08x", (unsigned int) entry_table->token);

	/*
		parse errors from here on are due to a missing value
		for the keyword.
	*/
	++p;								/* skip past equals sign */
	if (*p == '\0')
		return(INVALID);
	s = p;
	p = skip_white_space(s);			/* skip spaces after equals sign */
	if (*p == '\0')
		return(INVALID);
	retrieve_configuration_entry_name(p);		/* retrieve configuration parameter. */
	if (*p == '\0')						/* shouldn't happen */
		return(INVALID);

	entry_table->value = p;						/* ptr to parameter value */
	syslog(LOG_DEBUG,"  value \"%s\"",entry_table->value);
	return(entry_table->token);
}

