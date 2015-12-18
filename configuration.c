/*
 * configuration.c
 *	This is used to handle configuration initializations.
 *  Created on: Aug 4, 2015
 *      Author: tientham
 */


#include "serial_ip.h"

unsigned long def_speed = 9600L;
char *def_description = "Sabre Lite Serial Interface";


/*	Location: configuration.c
	This is to initialize serial port settings.	Return 0 on success.
*/

int serial_init_termios(SERIAL_INFO *sabre_serial_port, int *fd, struct termios old_setting, struct termios new_setting)
{
	int ret;
	/* get two copies of the current termios settings */
	ret = tcgetattr(*fd, &old_setting);
	if (ret == 0)
		ret = tcgetattr(*fd, &new_setting);
	if (ret != 0) {
		syslog(LOG_ERR,"configuration.c: serial_init_termios(): tcgetattr() failed on serial port %s",
				sabre_serial_port->device);
		return(ret);
	}
	/* configure the serial port speed */
	ret = cfsetospeed(&new_setting, ulong2speed_t(sabre_serial_port->speed));
	if (ret == 0){
		ret = cfsetispeed(&new_setting, ulong2speed_t(sabre_serial_port->speed));
	}
	if (ret != 0)
	{
		syslog(LOG_ERR, "configuration.c: serial_init_termios(): cfsetospeed() failed on modem %s, speed %lu",
				sabre_serial_port->device, sabre_serial_port->speed);
		return(ret);
	}
	/* configure the serial port control options */
	new_setting.c_cflag |= (CLOCAL|CREAD);
	switch (sabre_serial_port->parity)
	{
	case PARITY_ODD:
		new_setting.c_cflag |= PARENB;
		new_setting.c_cflag |= PARODD;
		new_setting.c_cflag &= ~CSTOPB;
		break;
	case PARITY_EVEN:
		new_setting.c_cflag |= PARENB;
		new_setting.c_cflag &= ~PARODD;
		new_setting.c_cflag &= ~CSTOPB;
		break;
	case PARITY_NONE:
	default:
		new_setting.c_cflag &= ~PARENB;
		new_setting.c_cflag &= ~CSTOPB;
		break;
	}
	new_setting.c_cflag &= ~CSIZE;
	new_setting.c_cflag |= uint2cs(sabre_serial_port->databits);
	/* enable/disable hardware flow control */
	if (sabre_serial_port->flowcontrol == HARDWARE_FLOW) {
		new_setting.c_cflag |= CRTSCTS;
	} else {
		new_setting.c_cflag &= ~CRTSCTS;
	}

	/* configure the serial port local options for raw mode */
	new_setting.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);

	/* configure the serial port input options for parity */
	if (new_setting.c_cflag & PARENB)
	{
		new_setting.c_iflag |= (INPCK|ISTRIP);
	} else {
		new_setting.c_iflag &= ~(INPCK|ISTRIP);
	}

	/* send us SIGINT when a break condition is present */
	new_setting.c_iflag |= BRKINT;

	/* enable/disable software flow control */
	if (sabre_serial_port->flowcontrol == SOFTWARE_FLOW)
	{
		new_setting.c_iflag |= (IXON|IXOFF|IXANY);
	} else {
		new_setting.c_iflag &= ~(IXON|IXOFF|IXANY);
	}
	/* disable these input options */
	new_setting.c_iflag &= ~(INLCR|ICRNL|IGNCR);
	/* configure the serial port output options for raw mode */
	new_setting.c_oflag &= ~OPOST;
	/*
		some systems (eg. Linux kernel 2.4.9) may do output processing,
		even when OPOST has been reset.  so reset these explicitly.
	*/
	new_setting.c_oflag &= ~(ONLCR|OCRNL);
	/*
		since we've specified O_NDELAY on the open(), we cannot
		specify character and packet timeouts via c_cc[VMIN]
		and c_cc[VTIME].
	*/
#if 0
	new_setting.c_cc[VMIN] = 0;
	new_setting.c_cc[VTIME] = 10;
#endif
	/* activate the new termios settings */
	ret = tcsetattr(*fd, TCSAFLUSH, &new_setting);
	if (ret != 0) {
		syslog(LOG_ERR,"configuration.c: serial_init_termios(): tcsetattr() failed on modem %s", sabre_serial_port->device);
		tcsetattr(*fd, TCSANOW, &old_setting);
		return(ret);
	}
	return(0);
}




/*
	Location: configuration.c

	This is to build a hash table from our configuration file keyword table defined in serial_ip.h.
	Returns the number of elements in the keyword table in which we will retrieve information.
	Otherwise, return -1 on error.
*/
int keyword_table_init(struct config_entry keyword_table[])
{
	int element;								/* elements in kw table */
	int i;										/* gp int */
	ENTRY hash_table;							/* hash table entry */

	element = 0;
	while ((keyword_table[element].name != NULL) && (keyword_table[element].name[0] != '\0')) {
		string_to_lower(keyword_table[element].name);	/* fold keyword to lower case */
		++element;							/* count elements in table */
	}
	if (hcreate(element) == 0) {
		syslog(LOG_ERR,"kw_init(): unable to create hash table");
		return(-1);
	}
	for (i = 0; i < element; i++) {
		hash_table.key = keyword_table[i].name;
		hash_table.data = (char *) &keyword_table[i];
		if (hsearch(hash_table,ENTER) == NULL) {
			syslog(LOG_ERR,"keyword_table_init(): failed to build hash table");
			hdestroy();
			return(-1);
		}
	}
	return(element);
}


/*
	Location: configuration.c
	This is to open and parse the configuration file to get each parameters value.
	After this operation, the members of the configuration structure (config_t) are filled in
	from the contents of the serial_ip.conf file.
	Remember: struct "config_t" => configuration file, meanwhile struct "config_entry" => hash table
	returns 0 on success, 1 on error.
*/
int read_configuration_file(char *file, struct config_t *conf)
{
	struct config_entry entry;						/* entry is hash table which we are going to fill data from serial_ip.conf into */
	unsigned long token;
	int error = 0;
	int lines;
	FILE *fp;
	char buffer[BUFSIZ];
	SERIAL_INFO *serial_device;
	SERIAL_INFO sabre_defaults;

	fp = fopen(file, "r");
	if (fp == NULL) {
		syslog(LOG_ERR, "Cannot open config file %s", file);
		return(1);
	}

	/*
		Initialization Sabre serial port defaults
	*/
	memset(&sabre_defaults,(int) 0, sizeof(SERIAL_INFO));
	sabre_defaults.description = strdup(def_description);				/* MUST alloc memory */
	sabre_defaults.speed = def_speed;
	sabre_defaults.databits = 8;
	sabre_defaults.parity = PARITY_NONE;
	sabre_defaults.stopbits = 1;
	sabre_defaults.flowcontrol = HARDWARE_FLOW;
	sabre_defaults.conn_flush = 0;										/* don't flush serial port on connect */
	sabre_defaults.disc_flush = 1;										/* do flush serial port on discconnect */

	/*	Parse the configuration file and save the info within the config_t structure */
	lines = 0;
	serial_device = &sabre_defaults;									/* ptr to sabre serial port defaults */
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		++lines;														/* incr config file line count */
		token = parse_entry(buffer, &entry);
		switch (token) {
		case COMMENT:
		case BLANKLINE:
			break;														/* these are no-ops */
		case DIRECTORY:
			error = save_value(entry.value, entry.type, &(conf->directory));
			break;
		case TMPDIR:
			error = save_value(entry.value, entry.type, &(conf->tmpdir));
			break;
		case LOCKDIR:
			error = save_value(entry.value, entry.type, &(conf->lockdir));
			break;
		case LOCKTEMPLATE:
			error = save_value(entry.value, entry.type, &(conf->locktemplate));
			break;
		case DEBUGLOG:
			error = save_value(entry.value, entry.type, &(conf->debuglog));
			break;
		case DEBUGLEVEL:
			error = save_value(entry.value, entry.type, &(conf->debuglevel));
			break;
		case PIDFILE:
			error = save_value(entry.value, entry.type, &(conf->pidfile));
			break;
		case TIMEOUT:
			error = save_value(entry.value, entry.type, &(conf->timeout));
			break;
		case USERNAME:
			error = save_value(entry.value, entry.type, &(conf->user));
			break;
		case GROUP:
			error = save_value(entry.value, entry.type, &(conf->group));
			break;
		case SERVERTYPE:
			error = save_value(entry.value, entry.type, &(conf->server_type));
			break;
		case DEVICE:
			/*	add serial port to conf->serial_port[] array.  increments conf->number_ports
				and dynamically allocates memory for the device name. */
			error = add_serial_port_info(conf, entry.value);
			if (! error)
			{
				serial_device = conf->serial_port[conf->number_ports - 1];		/* ptr to current modem */
				serial_device->speed = sabre_defaults.speed;				/* copy defaults */
				serial_device->databits = sabre_defaults.databits;
				serial_device->parity = sabre_defaults.parity;
				serial_device->stopbits = sabre_defaults.stopbits;
				serial_device->flowcontrol = sabre_defaults.flowcontrol;
				serial_device->description = strdup(sabre_defaults.description);
				serial_device->conn_flush = sabre_defaults.conn_flush;
				serial_device->disc_flush = sabre_defaults.disc_flush;
			}
			break;
		case DESCRIPTION:
			if (serial_device->description != NULL)
			{
				free(serial_device->description);
				serial_device->description = NULL;
			}
			error = save_value(entry.value, entry.type, &(serial_device->description));
			if(error)
					syslog(LOG_ERR,"configuration.c(): invalid save_value for serial description at line %d: %s",lines,entry.value);
			break;
		case SPEED:
			error = save_value(entry.value, entry.type, &(serial_device->speed));
			if (! error)
			{
				if (! valid_serial_speed(serial_device->speed))
				{
					syslog(LOG_ERR,"configuration.c(): invalid serial port speed at line %d: %s",lines,entry.value);
					error = 1;
				}
			}else
					syslog(LOG_ERR,"configuration.c(): invalid speed/baudrate value at line %d: %s",lines,entry.value);
			break;
		case DATABITS:
			error = save_value(entry.value,entry.type,&(serial_device->databits));
			if (! error) {
				switch (serial_device->databits) {
				case 8:
				case 7:
				case 6:
				case 5:
					break;
				default:
					syslog(LOG_ERR,"configuration.c(): invalid databits value at line %d: %s",lines,entry.value);
					error = 1;
					break;
				}
			}else
					syslog(LOG_ERR,"configuration.c(): invalid databits value at line %d: %s",lines,entry.value);
			break;
		case PARITY:
			if (strcasecmp(entry.value,"none") == 0) {
				serial_device->parity = PARITY_NONE;
			} else if (strcasecmp(entry.value,"odd") == 0) {
				serial_device->parity = PARITY_ODD;
			} else if (strcasecmp(entry.value,"even") == 0) {
				serial_device->parity = PARITY_EVEN;
			} else {
				syslog(LOG_ERR,"configuration.c(): invalid parity value at line %d: %s",lines,entry.value);
				error = 1;
			}
			break;
		case STOPBITS:
			error = save_value(entry.value,entry.type,&(serial_device->stopbits));
			if (! error) {
				switch (serial_device->stopbits) {
				case 1:
				case 2:
					break;
				default:
					syslog(LOG_ERR,"configuration.c(): invalid stopbits value at line %d: %s",lines,entry.value);
					error = 1;
					break;
				}
			}else
					syslog(LOG_ERR,"configuration.c(): invalid stopbits value at line %d: %s",lines,entry.value);
			break;
		case FLOWCONTROL:
			if (strcasecmp(entry.value,"hardware") == 0) {
				serial_device->flowcontrol = HARDWARE_FLOW;
			} else if (strcasecmp(entry.value,"rts/cts") == 0) {
				serial_device->flowcontrol = HARDWARE_FLOW;
			} else if (strcasecmp(entry.value,"software") == 0) {
				serial_device->flowcontrol = SOFTWARE_FLOW;
			} else if (strcasecmp(entry.value,"xon/xoff") == 0) {
				serial_device->flowcontrol = SOFTWARE_FLOW;
			} else if (strcasecmp(entry.value,"none") == 0) {
				serial_device->flowcontrol = NO_FLOWCONTROL;
			} else {
				syslog(LOG_ERR,"configuration.c(): invalid flow control value at line %d: %s",lines,entry.value);
				error = 1;
			}
			break;
		case CONNFLUSH:
			error = save_value(entry.value,entry.type,&(serial_device->conn_flush));
			if(error)
				syslog(LOG_ERR,"configuration.c(): invalid conn_flush value at line %d: %s",lines,entry.value);
			break;
		case DISCFLUSH:
			error = save_value(entry.value,entry.type,&(serial_device->disc_flush));
			if(error)
				syslog(LOG_ERR,"configuration.c(): invalid dish_flush value at line %d: %s",lines,entry.value);
			break;
		case REPLYPURGEDATA:
			error = save_value(entry.value,entry.type,&(conf->reply_purge_data));
			if(error)
				syslog(LOG_ERR,"configuration.c(): invalid reply_purge_data value at line %d: %s",lines,entry.value);
			break;
		case MS_POLLINTERVAL:
			error = save_value(entry.value,entry.type,&(conf->ms_pollinterval));
			if(error)
				syslog(LOG_ERR,"configuration.c(): invalid ms_pollinterval value at line %d: %s",lines,entry.value);
			break;
		case LS_POLLINTERVAL:
			error = save_value(entry.value,entry.type,&(conf->ls_pollinterval));
			if(error)
				syslog(LOG_ERR,"configuration.c(): invalid ls_pollinterval value at line %d: %s",lines,entry.value);
			break;
		case IDLETIMER:
			error = save_value(entry.value,entry.type,&(conf->idletimer));
			if(error)
				syslog(LOG_ERR,"configuration.c(): invalid idletimer value at line %d: %s",lines,entry.value);
			break;
		case SENDLOGOUT:
			error = save_value(entry.value,entry.type,&(conf->send_logout));
			if(error)
				syslog(LOG_ERR,"configuration.c(): invalid send_logout value at line %d: %s",lines,entry.value);
			break;
		case INVALID:
			syslog(LOG_ERR,"invalid or missing value at line %d: %s",lines,buffer);
			/* we won't treat this as an error */
			break;
		case UNKNOWN:
		default:
			syslog(LOG_ERR,"unknown keyword at line %d: %s",lines,buffer);
			/* we won't treat this as an error */
			break;
		}
		if (error != 0)
			break;
	} /* while */

	fclose(fp);							/* close configuration file */
	if (error != 0) {
		syslog(LOG_ERR,"error in configuration file near line %d",lines);
	}
	return(error);
}


/*
	Location: configuration.c

	This function is to look up the uid value for the specified user name.
	returns 0 and the uid on success, 1 on failure.
*/
int lookup_user(char *user, uid_t *uid)
{
	struct passwd *pw;

	pw = getpwnam(user);
	if (pw == NULL) {
		syslog(LOG_ERR,"no passwd entry for %s",user);
		return(1);
	}
	*uid = pw->pw_uid;
	return(0);
}

/*
	Location: configuration.c

	This function is to look up the gid value for the specified group name.
	returns 0 and the gid on success, 1 on failure.
*/
int lookup_group(char *group, gid_t *gid)
{
	struct group *gr;

	gr = getgrnam(group);
	if (gr == NULL) {
		syslog(LOG_ERR,"no group entry for %s",group);
		return(1);
	}
	*gid = gr->gr_gid;
	return(0);
}


/*
	Location: configuration.c

	This function is to check if a path name is directory or not.
	Returns 0 if name is a directory, otherwise it returns 1
*/
int is_directory(char *name)
{
	struct stat st;

	if (stat(name,&st) == 0) {
		if (S_ISDIR(st.st_mode)) {      /*S_ISDIR : is directory? */
			return(0);
		}
	}
	return(1);
}




/*
	Location: configuration.c

	In case of incompatible parameters given by users.
	This function is to set default parameters to our program configuration.
	Returns 0 on success, 1 on failure
*/
int sane_config(struct config_t *conf)
{
	extern char *def_servertype;
	//extern char *def_configfile;
	extern char *def_pidfile;
	extern int def_timeout;
	extern char *def_directory;
	extern char *def_tmpdir;
	extern int def_ms_pollinterval;
	extern int def_ls_pollinterval;
	extern int def_reply_purge_data;
	extern int def_idletimer;
	extern int def_send_logout;
	//int i;

	if (conf->user == NULL)
	{
		syslog(LOG_ERR,"configuration.c: sane_config() 'user' parameter error.");
		return(1);
	}
	if (lookup_user(conf->user,&conf->uid) != 0)
	{
		syslog(LOG_ERR,"configuration.c: sane_config() failed to look up 'user'.");
		return(1);
	}
	if (conf->group == NULL)
	{
		syslog(LOG_ERR,"configuration.c: sane_config() 'group' parameter error.");
		return(1);
	}
	if (lookup_group(conf->group,&conf->gid) != 0)
	{
		syslog(LOG_ERR,"configuration.c: sane_config() failed to look up 'group'.");
		return(1);
	}
	if (conf->server_type == NULL)
	{
		conf->server_type = strdup(def_servertype);
	} else 							/* check specified server type */
	{
		if ((strcmp(conf->server_type,"raw TCP gateway") != 0) &&
			(strcmp(conf->server_type,"concurrent") != 0) &&
		    (strcmp(conf->server_type,"iterative") != 0)) {
			syslog(LOG_ERR,"config: unknown server type: %s",conf->server_type);
			return(1);
		}
	}
	if (conf->pidfile == NULL)
		conf->pidfile = strdup(def_pidfile);
	if (conf->timeout <= 0)
		conf->timeout = def_timeout;
	if (conf->ms_pollinterval <= 0)
		conf->ms_pollinterval = def_ms_pollinterval;
	if (conf->ls_pollinterval <= 0)
		conf->ls_pollinterval = def_ls_pollinterval;
	if (conf->reply_purge_data < 0)
		conf->reply_purge_data = def_reply_purge_data;
	if (conf->directory == NULL)
		conf->directory = strdup(def_directory);
	if (conf->tmpdir == NULL)
		conf->tmpdir = strdup(def_tmpdir);
	if (conf->lockdir == NULL)
		conf->lockdir = strdup(UUCPLOCK_DIR);
	if (conf->locktemplate == NULL)
		conf->locktemplate = strdup(UUCPLOCK_TMPL);
	if (conf->idletimer < 0)
		conf->idletimer = def_idletimer;
	if (conf->send_logout < 0)
		conf->send_logout = def_send_logout;
	/*
		make sure configured directories exist
	*/
	if (is_directory(conf->directory) != 0) {
		syslog(LOG_ERR,"%s is not a directory",conf->directory);
		return(1);
	}
	if (is_directory(conf->tmpdir) != 0) {
		syslog(LOG_ERR,"%s is not a directory",conf->tmpdir);
		return(1);
	}
	if (is_directory(conf->lockdir) != 0) {
		syslog(LOG_ERR,"%s is not a directory",conf->lockdir);
		return(1);
	}
	/*
		make sure we have one or more serial port. Remember that Sabre has 1 serial port.
	*/
	if (conf->number_ports < 1) {
		syslog(LOG_ERR,"no modems were configured");
		return(1);
	}
	return(0);
}




