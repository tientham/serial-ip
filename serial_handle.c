/*
 * serial_handle.c
 *	This is to handle serial port configuration.
 *	For Sabre case, we have just only 1 RS232/RS485 port.
 *	However, for the future use, one can add more line of code into this file to handle with more than 1 serial port.
 *  Created on: Aug 5, 2015
 *      Author: tientham
 */


#include "serial_ip.h"

/*
	Location: serial_handle.c
	this function:
	- puts the modem device back to non-blocking mode
	- hangs up the modem by setting the speed to B0
	- (optionally) flushes the modem device
	- restores the previous modem line termios settings
	- closes the modem device file
	- releases the modem back to the pool
	- closes the debug log
	it returns nothing.
*/
void serial_cleanup(SERIAL_INFO *sabre_serial_port, int *serial_file_descriptor, struct termios old_setting, struct termios new_setting)
{
	extern int errno;
	speed_t current_speed;
	int flags;
	int ret;

	syslog(LOG_INFO,"serial_handle.c: serial_cleanup(): releasing serial port %s", sabre_serial_port->device);

	/* put the serial device back to non-blocking mode */
	flags = fcntl(*serial_file_descriptor, F_GETFL, 0);
	if (flags != -1)
	{
		flags |= O_NONBLOCK;
		flags = fcntl(*serial_file_descriptor, F_SETFL, flags);
	}
	if (flags == -1)
	{
		syslog(LOG_ERR,"serial_handle.c: serial_cleanup(): cannot set O_NONBLOCK on serial device %s (%s)",
				sabre_serial_port->device, strerror(errno));
	}

	/* briefly set the speed to zero to force a hangup */
	current_speed = cfgetospeed(&new_setting);
	cfsetospeed(&new_setting, B0);
	tcsetattr(*serial_file_descriptor, TCSADRAIN, &new_setting);
	sleep(1);
	cfsetospeed(&new_setting, current_speed);
	tcsetattr(*serial_file_descriptor, TCSADRAIN, &new_setting);

	/* flush the serial port (both input and output) */
	if (sabre_serial_port->disc_flush)
	{
		ret = tcflush(*serial_file_descriptor, TCIOFLUSH);
		if (ret != 0)
		{
			syslog(LOG_ERR, "serial_handle.c: serial_cleanup(): warning: cannot flush modem device %s (%s)",
					sabre_serial_port->device, strerror(errno));
		} else {
			syslog(LOG_INFO, "serial_handle.c: serial_cleanup():flushed modem device %s",
					sabre_serial_port->device);
		}
	}
	/* restore the old termiox settings */
#ifdef USE_TERMIOX
	ioctl(*fd,TCSETXW,&(oldterm->tx));
#endif
	/* restore the old termios settings */
	tcsetattr(*serial_file_descriptor, TCSADRAIN, &old_setting);
	/* close the serial device file */
	close(*serial_file_descriptor);
	*serial_file_descriptor = -1;
	/* release the serial port back to the pool */
	release_serial_port(sabre_serial_port);
	/* close the debug log */
	close_debug();
}

/*
 * Location: serial_handle.c
 * This is to get stop size value from serial file descriptor setting.
*/
unsigned char get_stopsize(int serial_file_descriptor)
{
	struct termios term;				/* current termios */
	int ret;

	ret = tcgetattr(serial_file_descriptor, &term);
	if (ret == 0)
	{
		if (term.c_cflag & CSTOPB) {
			return(CPC_STOPSIZE_2BITS);
		} else {
			return(CPC_STOPSIZE_1BIT);
		}
	} else {
		system_errorlog("tcgetattr() error");
		return(CPC_STOPSIZE_1BIT);
	}
}

/*
 * Location: serial_handle.c
 * This is to set stop size value from serial file descriptor setting.
*/
int set_stopsize(int serial_file_descriptor, unsigned long value)
{
	struct termios term;				/* current termios */
	int ret;

	ret = tcgetattr(serial_file_descriptor, &term);
	if (ret == 0)
	{
		switch (value)
		{
		case CPC_STOPSIZE_2BITS:
			term.c_cflag &= CSTOPB;
			break;
		case CPC_STOPSIZE_15BITS:
			syslog(LOG_ERR, "serial_handle.c(): set_stopsize(): ignoring request to set 1.5 stop bits, will use 1 instead");
			break;
			/* fall through to CPC_STOPSIZE_1BIT */
		case CPC_STOPSIZE_1BIT:
		default:
			term.c_cflag &= ~CSTOPB;
			break;
		}
		ret = tcsetattr(serial_file_descriptor, TCSAFLUSH, &term);
		if (ret != 0) {
			system_errorlog("serial_handle.c(): set_stopsize():  tcsetattr() error");
		}
	} else {
		system_errorlog("serial_handle.c(): set_stopsize(): tcgetattr() error");
	}
	return(ret);
}

/*
 * Location: serial_handle.c
 * This is to get parity value from serial file descriptor setting.
*/
unsigned char get_parity(int serial_file_descriptor)
{
	struct termios term;				/* current termios */
	int ret;

	ret = tcgetattr(serial_file_descriptor, &term);
	if (ret == 0) {
		if (term.c_cflag & PARENB) {	/* if parity is enabled */
			if (term.c_cflag & PARODD) {
				return(CPC_PARITY_ODD);	/* odd parity */
			} else {
				return(CPC_PARITY_EVEN);/* even parity */
			}
		} else {
			return(CPC_PARITY_NONE);	/* no parity */
		}
	} else {
		system_errorlog("tcgetattr() error");
		return(CPC_PARITY_NONE);		/* no parity */
	}
}

/*
 * Location: serial_handle.c
 * This is to set parity value from serial file descriptor setting.
*/
int set_parity(int serial_file_descriptor, unsigned long value)
{
	struct termios term;				/* current termios */
	int ret;

	ret = tcgetattr(serial_file_descriptor, &term);
	if (ret == 0) {
		switch (value) {
		case CPC_PARITY_ODD:
			term.c_cflag |= PARENB;
			term.c_cflag |= PARODD;
			term.c_cflag &= ~CSTOPB;
			break;
		case CPC_PARITY_EVEN:
			term.c_cflag |= PARENB;
			term.c_cflag &= ~PARODD;
			term.c_cflag &= ~CSTOPB;
			break;
		case CPC_PARITY_MARK:
		case CPC_PARITY_SPACE:
			syslog(LOG_ERR, "ignoring request to set %s parity, will use None instead", telnet_cpc_parity2str((unsigned char) value));
			break;
			/* fall through to CPC_PARITY_NONE */
		case CPC_PARITY_NONE:
		default:
			term.c_cflag &= ~PARENB;
			term.c_cflag &= ~CSTOPB;
			break;
		}
		ret = tcsetattr(serial_file_descriptor, TCSAFLUSH, &term);
		if (ret != 0) {
			system_errorlog("tcsetattr() error");
		}
	} else {
		system_errorlog("tcgetattr() error");
	}
	return(ret);
}


/*
 * Location: serial_handle.c
 * This is to get data size value from serial file descriptor setting.
*/
unsigned char get_datasize(int serial_file_descriptor)
{
	struct termios term;				/* current termios */
	unsigned long cs;
	int ret;

	ret = tcgetattr(serial_file_descriptor, &term);
	if (ret == 0) {
		cs = term.c_cflag & CSIZE;
		switch (cs) {
		case CS8:
			return(CPC_DATASIZE_CS8);
		case CS7:
			return(CPC_DATASIZE_CS7);
		case CS6:
			return(CPC_DATASIZE_CS6);
		case CS5:
			return(CPC_DATASIZE_CS5);
		default:
			return(CPC_DATASIZE_CS8);
		}
	} else {
		system_errorlog("tcgetattr() error");
		return(CPC_DATASIZE_CS8);
	}
}

/*
 * Location: serial_handle.c
 * This is to set data size value from serial file descriptor setting.
*/
int set_datasize(int serial_file_descriptor, unsigned long value)
{
	struct termios term;				/* current termios */
	int ret;

	ret = tcgetattr(serial_file_descriptor, &term);
	if (ret == 0) {
		term.c_cflag &= ~CSIZE;
		switch (value) {
		case CPC_DATASIZE_CS8:
			term.c_cflag |= CS8;
			break;
		case CPC_DATASIZE_CS7:
			term.c_cflag |= CS7;
			break;
		case CPC_DATASIZE_CS6:
			term.c_cflag |= CS6;
			break;
		case CPC_DATASIZE_CS5:
			term.c_cflag |= CS5;
			break;
		default:
			term.c_cflag |= CS8;
			break;
		}
		ret = tcsetattr(serial_file_descriptor, TCSAFLUSH, &term);
		if (ret != 0) {
			system_errorlog("tcsetattr() error");
		}
	} else {
		system_errorlog("tcgetattr() error");
	}
	return(ret);
}

/*
 * Location: serial_handle.c
 * This is to get baudrate value from serial file descriptor setting.
*/
unsigned long get_baudrate(int serial_file_descriptor)
{
	struct termios term;										/* current termios */
	speed_t speed;
	int ret;

	ret = tcgetattr(serial_file_descriptor, &term);
	if (ret == 0) {
		speed = cfgetospeed(&term);
		return(speed_t2ulong(speed));
	} else {
		system_errorlog("telnet.c: get_baudrate(): tcgetattr() error");
		return(9600);
	}
}

/*
 * Location: serial_handle.c
 * This is to set baudrate value from serial file descriptor setting.
*/
int set_baudrate(int serial_file_descriptor, unsigned long value)
{
	struct termios term;										/* current termios */
	speed_t speed;
	int ret;

	ret = tcgetattr(serial_file_descriptor, &term);
	if (ret == 0)
	{
		speed = ulong2speed_t(value);
		cfsetospeed(&term, speed);
		ret = tcsetattr(serial_file_descriptor, TCSAFLUSH, &term);
		if (ret != 0)
		{
			system_errorlog("tcsetattr() error");
		}
	} else {
		system_errorlog("tcgetattr() error");
	}
	return(ret);
}







/*
	Location: serial_handle.c
	This is to release serial port setting.
int release_serial_port(SERIAL_INFO *serial_port)
{
	int ret;
	sanity checks
	if (serial_port == NULL) return(1);
	if (serial_port->device == NULL) return(1);

	ret = unlock_uucp_lockfile(serial_port->device);
	if (serial_port->lockfile != NULL) {
		free(serial_port->lockfile);
		serial_port->lockfile = NULL;
	}
	return(ret);
}
*/

/*
	Location: serial_handle.c
	This function is to select a serial port from available serial ports.
	In case of Sabre Lite, just 1 port used.
	returns a SERIAL_INFO ptr on success, NULL on failure.
	After running this function, we can use serial port on sabre with device file descriptor and 1 process
	takes control of this port!
*/
SERIAL_INFO *select_serial_port(struct config_t *conf)
{
	int nbusy;
	SERIAL_INFO *sabre_serial_port;
	int r;
	char *device_lockfile;
	pid_t pid;
	time_t seconds;

	pid = getpid();									/* get our process id */
	syslog(LOG_DEBUG,"serial_handle.c: select_serial_port(): process's pid %d", pid);
	sabre_serial_port = NULL;						/* init return value */
	/*	clear the busy flag on all available serial ports. */
	for (r = 0; r < conf->number_ports; r++) {
		conf->serial_port[r]->busy = 0;
	}
	nbusy = 0;
	/*	seed the random number generator. */
	seconds = time((time_t *) NULL);
	srand(seconds);
	/*	choose a port randomly, until we can lock one or we've tried them all. */
	while (nbusy < conf->number_ports) {
		/*	generate a random integer between 0 and conf->number_ports.
		r = (int) (rand() / ((RAND_MAX / conf->number_ports) + 1));	*/
		/*	Sabre just needs 1 port.	*/
		r = 0;
		syslog(LOG_DEBUG, "serial_handle.c: select_serial_port(): random modem index: %d", r);
		syslog(LOG_DEBUG, "serial_handle.c: select_serial_port(): number of available port: %d, r is: %d", conf->number_ports, r);
		if ((r >= 0) && (r < conf->number_ports)) {
			/* skip this one if we've tried it already */
			if (conf->serial_port[r]->busy) continue;
			syslog(LOG_DEBUG, "serial_handle.c: select_serial_port(): serial port will be assigned %s", conf->serial_port[r]->device);
			/* try to lock serial port. */
			device_lockfile = create_uucp_lockfile(conf->serial_port[r]->device, pid);
			syslog(LOG_DEBUG, "serial_handle.c: select_serial_port(): checking device_lockfile...");
			if (device_lockfile != NULL) {
				conf->serial_port[r]->lockfile = strdup(device_lockfile);
				sabre_serial_port = conf->serial_port[r];
				syslog(LOG_DEBUG, "serial_handle.c: select_serial_port(): serial port is assigned %s", sabre_serial_port->device);
				break;
			} else {
				syslog(LOG_DEBUG, "serial_handle.c: select_serial_port(): serial port is assigned - failed");
				conf->serial_port[r]->busy = 1;
				nbusy++;
			}
		}
	}
	return(sabre_serial_port);
}

/*
	Location: serial_handle.c
	This function is to:
	- selects a serial port from available serial ports on board
    - opens the serial device in non-blocking mode
	- saves the serial line's original termios settings
	- configures the serial line's termios settings for raw i/o
    - puts the serial device into blocking mode again
	- (optionally) flushes the serial port.
	- opens a debug log, named for the selected serial port
	on success, we return a SERIAL_INFO ptr for the selected serial port,
	plus the file descriptor returned by open(), the original termios
	settings, and the new termios settings.
	on failure, a NULL ptr is returned.
*/
SERIAL_INFO *serial_port_init(int *fd, struct termios old_setting, struct termios new_setting)
{
	extern int errno;
	extern char *program_name;							/* our program name */
	extern char *version;								/* version string */
	extern struct config_t conf;						/* built from config file */
	char log[PATH_MAX];									/* name of debug log */
	SERIAL_INFO *sabre_serial_port;
	int flags;
	int ret;

	/* allocate a serial port. */
	sabre_serial_port = select_serial_port(&conf);
	if (sabre_serial_port == NULL)
	{
		syslog(LOG_ERR,"serial_handle.c: serial_port_init(): unable to allocate a serial port");
		return(sabre_serial_port);
	} else {
		syslog(LOG_INFO,"serial_handle.c: serial_port_init(): using serial port %s", sabre_serial_port->device);
	}

	/* open the serial port */
	*fd = open(sabre_serial_port->device, O_RDWR|O_NOCTTY|O_NDELAY);
	if (*fd < 0) {
		syslog(LOG_ERR, "serial_handle.c: serial_port_init(): open(%s,...) error: %s", sabre_serial_port->device, strerror(errno));
		release_serial_port(sabre_serial_port);
		return(NULL);
	}else
		syslog(LOG_INFO, "serial_handle.c: serial_port_init(): open(%s,...) status: ok!", sabre_serial_port->device);
	/* configure the termios settings */
	ret = serial_init_termios(sabre_serial_port,fd, old_setting, new_setting);
	if (ret != 0) {
		close(*fd);
		release_serial_port(sabre_serial_port);
		return(NULL);
	}else
		syslog(LOG_INFO, "serial_handle.c: serial_port_init(): init(%s,...) status: ok!", sabre_serial_port->device);
	/* put the serial port device file back to blocking mode */
	flags = fcntl(*fd, F_GETFL, 0);
	if (flags != -1) {
		flags &= ~O_NONBLOCK;
		flags = fcntl(*fd, F_SETFL, flags);
		syslog(LOG_INFO, "serial_handle.c: serial_port_init(): Set blocking mode for (%s,...) status: ok!", sabre_serial_port->device);
	}
	if (flags == -1) {
		syslog(LOG_ERR, "serial_handle.c: serial_port_init(): warning: cannot remove O_NONBLOCK from modem device %s (%s)",
				sabre_serial_port->device, strerror(errno));
	}
	/* flush the serial port (both input and output) */
	if (sabre_serial_port->conn_flush) {
		ret = tcflush(*fd,TCIOFLUSH);
		if (ret != 0) {
			syslog(LOG_ERR, "serial_handle.c: serial_port_init(): warning: cannot flush serial device %s (%s)",
					sabre_serial_port->device, strerror(errno));
		} else {
			syslog(LOG_INFO, "serial_handle.c: serial_port_init(): flushed serial device %s", sabre_serial_port->device);
		}
	}
	/* open the debug log */
	if ((conf.debuglog == NULL) || (*conf.debuglog == '\0') || (strcasecmp(conf.debuglog, "syslog") == 0))
	{
		log[0] = '\0';										/* send debug output to syslog */
	} else {
		snprintf(log, sizeof(log), conf.debuglog, get_program_name(sabre_serial_port->device));
	}
	open_debug(log, conf.debuglevel, program_name);
	write_to_debuglog(DBG_ERR,"%s %s",program_name, version);
	syslog(LOG_INFO, "serial_handle.c: serial_port_init(): Sabre's (%s) is ready to use!", sabre_serial_port->device);
	/* return the SERIAL_INFO ptr */
	return(sabre_serial_port);
}

/*
	Location: serial_handler.c
	This is to free the memory used by a SERIAL_INFO for a specific serial port.
	We need free() each dynamically allocated element before doing the whole serial port itself.
*/
void free_serial_port(SERIAL_INFO *serial_port)
{
	/* sanity checks */
	if (serial_port == NULL) return;

	if (serial_port->device != NULL)
		free(serial_port->device);
	if (serial_port->lockfile != NULL)
		free(serial_port->lockfile);
	free(serial_port);
}

/*
	Location: serial_handle.c
	This function is to free the memory used by the array of SERIAL_INFO's
*/
void free_all_serial_ports(SERIAL_INFO *serial_ports[])
{
	int i;

	/* sanity checks */
	if (serial_ports == NULL)
		return;

	for (i = 0; i < MAX_SPORTS; i++) {
		free_serial_port(serial_ports[i]);
	}
}

/*	Location: serial_handle.c
	This function is to add a serial port information to the array of SERIAL_INFO's within the config_t structure
	returns 0 on success, 1 on failure.	*/
int add_serial_port_info(struct config_t *conf, char *device_path)
{
	extern int errno;
	SERIAL_INFO *new_serial;
	int i;

	/* sanity checks */
	if (conf == NULL)
	{
		syslog(LOG_ERR,"serial_handle.c: failed add_serial_port_info() %s; Unable to read configuration file", device_path);
		return(1);
	}
	if (device_path == NULL)
	{
		syslog(LOG_ERR,"serial_handle.c: failed add_serial_port_info() %s; Unable to get device path", device_path);
		return(1);
	}
	if (*device_path == '\0')
	{
		syslog(LOG_ERR,"serial_handle.c: failed add_serial_port_info() %s; Unable to get device path", device_path);
		return(1);
	}
	if (conf->number_ports >= MAX_SPORTS) {
		syslog(LOG_ERR, "serial_handle.c: failed add_serial_port_info() %s; port %d is over max port numbers", device_path, conf->number_ports);
		return(1);
	}else
		syslog(LOG_DEBUG, "serial_handle.c: add_serial_port_info() %s; initialize with %d port(s)", device_path, conf->number_ports);

	new_serial = alloc_serial_port();
	if (new_serial != NULL) {
		i = conf->number_ports;
		conf->serial_port[i] = new_serial;
		conf->number_ports++;
		syslog(LOG_DEBUG, "serial_handle.c: add_serial_port_info(): number port is: %d", conf->number_ports);
		conf->serial_port[i]->device = strdup(device_path);
		if (conf->serial_port[i]->device == NULL) {
			syslog(LOG_ERR,"add_serial_port_info(): strdup() error: %s",strerror(errno));
			return(1);
		}else
			syslog(LOG_DEBUG,"serial_handle.c: add_serial_port_info(): device path is: %s", conf->serial_port[i]->device);
	}
	return(0);
}

/*	Location: serial_handle.c
	This function is to allocate memory for a new serial port.
	returns a ptr to the new SERIAL_INFO, otherwise NULL for error.	*/
SERIAL_INFO *alloc_serial_port(void)
{
	extern int errno;
	SERIAL_INFO *new_serial;

	new_serial = calloc(1,sizeof(SERIAL_INFO));
	if (new_serial == NULL)
		syslog(LOG_ERR,"alloc_serial_port() error: %s",strerror(errno));
	else
		syslog(LOG_DEBUG,"serial_handle.c: alloc_serial_port() - Initialize a serial port successful!");
	return(new_serial);
}

/*	Location: serial_handle.c
	This function is to release a serial port for next use.	*/
int release_serial_port(SERIAL_INFO *sabre_serial_port)
{
	int ret;

	/* sanity checks */
	if (sabre_serial_port == NULL) return(1);
	if (sabre_serial_port->device == NULL) return(1);

	ret = unlock_uucp_lockfile(sabre_serial_port->device);
	if (sabre_serial_port->lockfile != NULL)
	{
		free(sabre_serial_port->lockfile);
		sabre_serial_port->lockfile = NULL;
	}
	return(ret);
}

