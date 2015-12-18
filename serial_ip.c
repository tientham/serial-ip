/*
 ============================================================================
 Name        : serial_ip.c
 Author      : TienTham
 Version     : 1.0
 Copyright   : TienTham
 Description : This is program for serial-ip
 ============================================================================
 */


/*
  	  Provides the main function for server program. Its responsibility is to parse
  	  command-line options, detect, and report command-line errors, and config and
  	  run server.
*/

extern char *version = "Sabre Serial-IP v1.1 11.08.2015 ToMinhTien";

#define INIT_KEYWORDS
#include "serial_ip.h"

/* Global variables.*/

char *program_name  	= NULL;
char *config_file  		= NULL;
int  sabre_network_port	= SABRE_NETWORK_PORT;
int Daemon				= 0;
int signo				= 0;							/* received signal number */
int useconds 			= 0;
int noquote 			= 0;							/* No quote 'IAC' chars from serial line.*/
int raw_flag			= 0;							/* Flag for active raw_TCP_gateway mode. */
struct config_t conf	   ;

/* Local functions. */
int get_options_from_input(int, char**);
void usage(void);


/* Default configure parameters. */
char *def_servertype       = "raw TCP gateway";	/* or "concurrent" or "iterative" */
char *def_configfile       = "/etc/serial_ip.conf";
char *def_pidfile          = "/var/run/serial_ip.pid";
int   def_timeout          = 30;
char *def_directory        = ".";
char *def_tmpdir           = "/tmp";
int   def_ms_pollinterval  = 10;
int   def_ls_pollinterval  = 30;
int   def_reply_purge_data = 0;
int   def_idletimer        = 0;
int   def_send_logout      = 0;

int main(int argc, char** argv)
{

	/*Store the program name, which we will use in error message.	*/
	program_name = get_program_name(argv[0]);

	if(get_options_from_input(argc, argv) != 0)
	{
		usage();
		exit(1);
	}

	if(Daemon)
		mydaemon(0, ".");
	umask(0066);
	serial_ip_init();
	install_signal_handlers();
	create_pidfile();					/* write our pid to a file */
	server_init(sabre_network_port);
	program_clean_up();
	exit(0);
	return 0;
}

/*
 	 Location: serial_ip.c
 	 This function is to parse options from command line.
 */

int get_options_from_input(int argc, char** argv)
{
	extern char *config_file;			/* configuration file */
	extern int sabre_network_port;		/* tcp port number for server */
	extern int Daemon;					/* flag; become a daemon */
	extern int noquote;					/* flag; don't quote IAC chars from serial lines */
	extern int useconds;				/* i/o wait in u-seconds */
	extern char *optarg;
	int next_option;
	int error_flag 				= 0;
	/*Parse options.	*/
	while ((next_option = getopt(argc,(GETOPT_CAST) argv,"udnc:p:w:")) != EOF)
	{
			switch(next_option) {
			case 'u':
				error_flag++;
				break;
			case 'd':
				Daemon++;
				break;
			case 'n':
				noquote++;
				break;
			case 'c':
				config_file = malloc(strlen(optarg)+1);
				if (config_file != NULL)
					strcpy(config_file,optarg);
				break;
			case 'p':
				sabre_network_port = atoi(optarg);
				break;
			case 'w':
				useconds = atoi(optarg);
				break;
			case '?':
			default:
				error_flag++;
				break;
			} /* switch */
		} /* while */

		return(error_flag);
}

/*
  	  Location: serial_ip.c
  	  Print usage information and exit. If IS_ERROR is nonzero, write to stderr and use an error exit code.
  	  Otherwise, write to stdout and use a non-error termination code. Does not return.
 */

void usage(void)
{
	extern char *program_name;				/* program name */
	extern char *version;				/* version string */

	fprintf(stderr,"%s %s\n\n", program_name, version);
	fprintf(stderr,"usage: %s [-dn] [-c file] [-p port] [-w u-seconds]\n",program_name);
	fprintf(stderr,"\n");
	fprintf(stderr,"    -d  become a daemon\n");
	fprintf(stderr,"    -n  no quoting of IAC char from serial lines\n");
	fprintf(stderr,"    -c  configuration file name.  default is %s\n",def_configfile);
	fprintf(stderr,"    -p  tcp port number for server.  default is %d\n",sabre_network_port);
	fprintf(stderr,"    -w  wait before reading socket and serial port, specified in u-seconds\n");
}

/*
 	 Location: serial_ip.c
 	 Initializing parent process for reading a configuration file.
 	 If something is wrong, exit.
*/
void serial_ip_init()
{
	extern struct config_entry keyword_table[];	/* keyword table */
	extern char *program_name;				/* program name */
	extern char *version;				/* version string */
	extern char *config_file;			/* config file name. This is serial_ip.conf by default. */
	extern char *def_configfile;		/* serial_ip.conf */
	extern struct config_t conf;		/* This is configuration file - serial_ip.conf*/
	extern int def_timeout;
	extern int def_ms_pollinterval;
	extern int def_ls_pollinterval;
	extern int def_reply_purge_data;
	extern int def_idletimer;
	extern int def_send_logout;

	openlog(program_name, LOG_PID|LOG_CONS|LOG_PERROR,LOG_DAEMON);
	syslog(LOG_INFO,"serial_ip.c: serial_ip_init(): restart, %s",version);
/*
	Build a hash table from our keyword table. By this way, we put our predefined table into
	a hash table.
*/
	if (keyword_table_init(keyword_table) == -1)
	{
		syslog(LOG_ERR, "serial_ip.c(): Unable to initialize hash table. keyword_table_init().");
		exit(1);
	}
/*
	Set defaults for configuration file
*/
	memset(&conf,(int) '\0',sizeof(conf));
	conf.timeout = def_timeout;
	conf.ms_pollinterval = def_ms_pollinterval;
	conf.ls_pollinterval = def_ls_pollinterval;
	conf.reply_purge_data = def_reply_purge_data;
	conf.idletimer = def_idletimer;
	conf.send_logout = def_send_logout;

	if (config_file == NULL)
		config_file = def_configfile;	/* use default name. */
	if (read_configuration_file(config_file, &conf) != 0)
	{
		syslog(LOG_ERR,"serial_ip.c(): Unable to read configuration file. read_configuration_file().");
		exit(1);
	}
	if (sane_config(&conf) != 0)		/* insure config params are sane */
	{
		syslog(LOG_ERR,"serial_ip.c(): Unable to read configuration file. sane_config().");
		exit(1);
	}

}

/*
	Location: serial_ip.c
	This function is to clean up operations on the parent process when it receives SIGTERM, SIGINIT, SIGQUIT.
	returns nothing.
*/
void program_clean_up(void)
{
	extern struct config_t conf;		/* Global variable config file */
	closelog();							/* close the syslog. */
	/*
		free the memory allocated for items in the config file structure.
	*/
	if (conf.directory != NULL)
		free(conf.directory);
	if (conf.tmpdir != NULL)
		free(conf.tmpdir);
	if (conf.pidfile != NULL)
		free(conf.pidfile);
	if (conf.user != NULL)
		free(conf.user);
	if (conf.group != NULL)
		free(conf.group);
	if (conf.server_type != NULL)
		free(conf.server_type);
	if (conf.lockdir != NULL)
		free(conf.lockdir);
	if (conf.locktemplate != NULL)
		free(conf.locktemplate);
	free_all_serial_ports(conf.serial_port);

	/*Destroy keyword table*/
	hdestroy();
}
