/*
 * serial_ip.h
 *
 *  Created on: Aug 14, 2015
 *      Author: tientham
 */

#ifndef SERIAL_IP_H_
#define SERIAL_IP_H_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <syslog.h>
#include <getopt.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <stdarg.h>
#include <time.h>


#include <search.h>   			/* hash entry table. */

#include <sys/time.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <memory.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/in_systm.h>
#include <arpa/telnet.h>
#include <arpa/ftp.h>
#include <arpa/inet.h>

#include <termios.h>

#ifndef SABRE_NETWORK_PORT
#define SABRE_NETWORK_PORT				1194
#endif

/*
	defaults for uucp locking.
*/
#ifdef __linux__
#define UUCPLOCK_DIR	"/var/lock"
#define UUCPLOCK_TMPL	"LCK..%s"
#endif


#ifdef __linux__
#define ARGV_TYPE 		char * const *
#endif
#define GETOPT_CAST ARGV_TYPE

#ifndef PATH_MAX
#define PATH_MAX		255
#endif

/* This is to define server type.	*/
#define BLOCKING	0x01			/* second arg to create_server_socket() */
#define NONBLOCKING	0x00


/*
	symbols for shutdown(2) call
*/
#ifndef SHUT_RD
#define SHUT_RD		0x00
#define SHUT_WR		0x01
#define SHUT_RDWR	0x02
#endif


/*
  	  max number of serial port.
  	  In case of SabreLite, we have 1 RS232/485 port.
*/
#define MAX_SPORTS	256

/*
	Location: serial_ip.h
	This structure is to keep track of info on serial devices
*/
struct serial_info_t {
	char *device;				/* path name of device */
	char *lockfile;				/* path name of lock file */
	char *description;			/* device description */
	unsigned long speed;		/* baud rate */
	unsigned int databits;		/* data bits */
	unsigned int parity;		/* parity */
	unsigned int stopbits;		/* stop bits (NOT USED) */
	unsigned int flowcontrol;	/* flow control */
	int conn_flush;				/* flush device on connect? */
	int disc_flush;				/* flush device on disconnect? */
	int busy;					/* modem already in use */
};

typedef struct serial_info_t SERIAL_INFO;


/*
	Location: serial_ip.h
	This is configuration information which come from our serial_ip.conf.
	If there is no serial_ip.conf file, we will use default parameters.
*/
struct config_t {
	char *directory;				/* directory name */
	char *tmpdir;					/* tmp directory name */
	char *debuglog;					/* name of debug log */
	int debuglevel;					/* level of debugging output */
	char *pidfile;					/* name of pid file */
	int timeout;					/* connection timeout in seconds */
	char *user;						/* user name */
	char *group;					/* group name */
	uid_t uid;						/* user id (looked up via user name) */
	gid_t gid;						/* group id (looked up via group name) */
	char *server_type;				/* server type: raw TCP gateway or concurrent or iterative */
	char *lockdir;					/* directory for uucp lock files */
	char *locktemplate;				/* filename template for uucp lock files */
	int ms_pollinterval;			/* polling interval in seconds for modemstate changes */
	int ls_pollinterval;			/* polling interval in seconds for linestate changes */
	int reply_purge_data;			/* reply to Purge Data commands? */
	int idletimer;					/* idle timer */
	int send_logout;				/* send Telnet LOGOUT command? */
	int number_ports;					/* number of serial ports */
	SERIAL_INFO *serial_port[MAX_SPORTS]; /* array of ptrs to SERIAL_INFO's */
};

/*
	Author Note: This defined is derivered from "telnetcpcd" project.

	Token types.  keep these values unique!
*/
#define LONGVALUE		-5
#define BOOLEAN			-4
#define PRINTFSTRING	-3
#define STRING			-2
#define VALUE			-1

/*
	Author Note: This defined is derivered from "telnetcpcd" project.

	Token values.  keep these values unique!  they don't really need
	to be bit masks.
	These are used while parsing the configiration file serial_ip.conf.
	See also keyword_table[].
*/
#define UNKNOWN			0x00000001		/* keyword is unknown */
#define INVALID			0x00000002		/* missing/invalid value for keyword */
#define COMMENT			0x00000004
#define BLANKLINE		0x00000008
#define DEBUGLOG		0x00000010
#define DEBUGLEVEL		0x00000020
#define DIRECTORY		0x00000040
#define TMPDIR			0x00000080
#define PIDFILE			0x00000100
#define TIMEOUT			0x00000200
#define SERVERTYPE		0x00000400
#define USERNAME		0x00000800
#define GROUP			0x00001000
#define LOCKDIR			0x00002000
#define LOCKTEMPLATE	0x00004000
#define DEVICE			0x00008000
#define SPEED			0x00010000
#define DATABITS		0x00020000
#define PARITY			0x00040000
#define STOPBITS		0x00080000
#define FLOWCONTROL		0x00100000
#define MS_POLLINTERVAL	0x00200000
#define LS_POLLINTERVAL	0x00400000
#define DESCRIPTION		0x00800000
#define REPLYPURGEDATA	0x01000000
#define CONNFLUSH		0x02000000
#define DISCFLUSH		0x04000000
#define IDLETIMER		0x08000000
#define SENDLOGOUT		0x10000000

/*
	parity symbols
*/
#define PARITY_NONE		0x00
#define PARITY_ODD		0x01
#define PARITY_EVEN		0x02

/*
	flow control symbols
*/
#define HARDWARE_FLOW	0x04
#define SOFTWARE_FLOW	0x08
#define NO_FLOWCONTROL	0x10

#define SIZE_BUFFER					4096
#define SIZE_KEYWORD 				64
#define MAXHOSTNAME_LEN				99
#define CHUNK						16

struct buffer_t {
	int size;						/* buffer size */
	int nbuffered;					/* bytes in buffer */
	int eof;						/* read() eof flag */
	unsigned char *label;			/* name of buffer */
	unsigned char *buffp;			/* ptr to buffer */
	unsigned char *tailp;			/* ptr past end of buffer */
	unsigned char *readp;			/* read ptr */
	unsigned char *writep;			/* write ptr */
};
typedef struct buffer_t BUFFER;



/* ----------------------------RAW TCP MODE----------------------------------- */
/* Common header for all the kinds of PDUs. */

#define SERIAL_IP_VERSION	(0x80 << 8)

#define MAX_MESSAGE_LENGTH 30
#define MAX_BUFFER_LENGTH 200
/* -------------------------------------------------- */
/* Define Serial-IP Protocol Format                   */
/* -------------------------------------------------- */

/* ---------------------------------------------------------------------- */
/* Common header for all the kinds of PDUs. */
struct op_pdu {
	char message[MAX_MESSAGE_LENGTH];
	char data[MAX_BUFFER_LENGTH];
	uint16_t version;
	//uint16_t code;
} __attribute__((packed));

#define PACK_OP_PDU(pack, op_pdu)  do {\
	pack_uint16_t(pack, &(op_pdu)->version);\
} while (0)

/* ----------------------------END- RAW TCP MODE------------------------------ */

/*
	Location: serial_ip.h
	The config file keyword table.
	Remember, it is just hash table, not configuration file. We use hash table to validate parameters.
	The entry and config keyword table are based on "telnetcpcd" project of Thosmas J Pinkl.
*/
struct config_entry {
	char name[SIZE_KEYWORD];
	unsigned long token;
	int type;
	char *value;
};
#ifdef INIT_KEYWORDS
struct config_entry keyword_table[] = {
/*  {   <name>,					    <TOKEN>,	    <Type>,		 <Value>}, */
	{"description",					DESCRIPTION,	STRING,			NULL},
	{"serial device",				DEVICE,			STRING,			NULL},
	{"speed",						SPEED,			LONGVALUE,		NULL},
	{"databits",					DATABITS,		VALUE,			NULL},
	{"parity",						PARITY,			STRING,			NULL},
	{"stopbits",					STOPBITS,		VALUE,			NULL},
	{"flow control",				FLOWCONTROL,	STRING,			NULL},
	{"flush on connect",			CONNFLUSH,		BOOLEAN,		NULL},
	{"flush on disconnect",			DISCFLUSH,		BOOLEAN,		NULL},
	{"directory",					DIRECTORY,		STRING,			NULL},
	{"tmp directory",				TMPDIR,			STRING,			NULL},
	{"pid file",					PIDFILE,		STRING,			NULL},
	{"server type",					SERVERTYPE,		STRING,			NULL},
	{"timeout",						TIMEOUT,		VALUE,			NULL},
	{"reply Purge Data",			REPLYPURGEDATA,	BOOLEAN,		NULL},
	{"user",						USERNAME,		STRING,			NULL},
	{"group",						GROUP,			STRING,			NULL},
	{"modemstate poll interval",	MS_POLLINTERVAL,VALUE,			NULL},
	{"linestate poll interval",		LS_POLLINTERVAL,VALUE,			NULL},
	{"debug log",					DEBUGLOG,		STRING,			NULL},
	{"debug level",					DEBUGLEVEL,		VALUE,			NULL},
	{"lock directory",				LOCKDIR,		STRING,			NULL},
	{"lock template",				LOCKTEMPLATE,	STRING,			NULL},
	{"idle timer",					IDLETIMER,		VALUE,			NULL},
	{"send Telnet LOGOUT",			SENDLOGOUT,		BOOLEAN,		NULL},
	{"username",					USERNAME,		STRING,			NULL},
	{"device",						DEVICE,			STRING,			NULL},
	{"port",						DEVICE,			STRING,			NULL},
	{"speed",						SPEED,			LONGVALUE,		NULL},
	{"baudrate",					SPEED,			LONGVALUE,		NULL},
	{"",							UNKNOWN,		STRING,			NULL}	/* this entry must be last */
};
#else
extern struct config_entry keyword_table[];
#endif

/*
	debug levels.
	Incresing by signal sigusr1, decreasing by signal sigusr2.
*/
#define DBG_ERR			0
#define DBG_INF			3
#define DBG_VINF		6
#define DBG_MEM			7
#define DBG_LV0			0
#define DBG_LV1			1
#define DBG_LV2			2
#define DBG_LV3			3
#define DBG_LV4			4
#define DBG_LV5			5
#define DBG_LV6			6
#define DBG_LV7			7
#define DBG_LV8			8
#define DBG_LV9			9

/*
	telnet options
*/
struct telnet_options_t {
	int sent_will:1;
	int sent_do:1;
	int sent_wont:1;
	int sent_dont:1;
	int server:1;
	int client:1;
};
typedef struct telnet_options_t TELNET_OPTIONS;

/* Telnet commands */

/* telnet options.	*/

#ifndef TELOPT_COM_PORT_OPTION
#define TELOPT_COM_PORT_OPTION					44
#endif
#ifndef TELOPT_KERMIT
#define TELOPT_KERMIT							47
#endif

#define MAX_TELNET_OPTIONS						256
#define MAX_TELNET_CPC_COMMAND_LEN				256

/*	telnet mode: ASCII or Binary	*/
#define ASCII	0x00
#define BINARY	0x01

/*	session state: Suspend or Resume 	*/
#define SUSPEND									0x00
#define RESUME									0x01

/*	carrier detect state	*/
#define NO_CARRIER								0x00
#define GOT_CARRIER								0x01
#define LOST_CARRIER							0x02

/*	Telnet Com Port Control (CPC) option values (from rfc2217)	*/
#define CPC_SIGNATURE_C2S						0		/* C2S = client to server */
#define CPC_SET_BAUDRATE_C2S					1
#define CPC_SET_DATASIZE_C2S					2
#define CPC_SET_PARITY_C2S						3
#define CPC_SET_STOPSIZE_C2S					4
#define CPC_SET_CONTROL_C2S						5
#define CPC_SET_CONTROL_C2S						5
#define CPC_NOTIFY_LINESTATE_C2S				6
#define CPC_NOTIFY_MODEMSTATE_C2S				7
#define CPC_FLOWCONTROL_SUSPEND_C2S				8
#define CPC_FLOWCONTROL_RESUME_C2S				9
#define CPC_SET_LINESTATE_MASK_C2S				10
#define CPC_SET_MODEMSTATE_MASK_C2S				11
#define CPC_PURGE_DATA_C2S						12

#define CPC_SIGNATURE_S2C						100		/* S2C = server to client */
#define CPC_SET_BAUDRATE_S2C					101
#define CPC_SET_DATASIZE_S2C					102
#define CPC_SET_PARITY_S2C						103
#define CPC_SET_STOPSIZE_S2C					104
#define CPC_SET_CONTROL_S2C						105
#define CPC_NOTIFY_LINESTATE_S2C				106
#define CPC_NOTIFY_MODEMSTATE_S2C				107
#define CPC_FLOWCONTROL_SUSPEND_S2C				108
#define CPC_FLOWCONTROL_RESUME_S2C				109
#define CPC_SET_LINESTATE_MASK_S2C				110
#define CPC_SET_MODEMSTATE_MASK_S2C				111
#define CPC_PURGE_DATA_S2C						112

#define CPC_DATASIZE_REQUEST					0
#define CPC_DATASIZE_CS5						5
#define CPC_DATASIZE_CS6						6
#define CPC_DATASIZE_CS7						7
#define CPC_DATASIZE_CS8						8

#define CPC_PARITY_REQUEST						0
#define CPC_PARITY_NONE							1
#define CPC_PARITY_ODD							2
#define CPC_PARITY_EVEN							3
#define CPC_PARITY_MARK							4
#define CPC_PARITY_SPACE						5

#define CPC_STOPSIZE_REQUEST					0
#define CPC_STOPSIZE_1BIT						1
#define CPC_STOPSIZE_2BITS						2
#define CPC_STOPSIZE_15BITS						3

#define CPC_SET_CONTROL_FLOW_REQUEST			0
#define CPC_SET_CONTROL_FLOW_NONE				1
#define CPC_SET_CONTROL_FLOW_XONXOFF			2
#define CPC_SET_CONTROL_FLOW_HARDWARE			3
#define CPC_SET_CONTROL_BREAK_REQUEST			4
#define CPC_SET_CONTROL_BREAK_ON				5
#define CPC_SET_CONTROL_BREAK_OFF				6
#define CPC_SET_CONTROL_DTR_REQUEST				7
#define CPC_SET_CONTROL_DTR_ON					8
#define CPC_SET_CONTROL_DTR_OFF					9
#define CPC_SET_CONTROL_RTS_REQUEST				10
#define CPC_SET_CONTROL_RTS_ON					11
#define CPC_SET_CONTROL_RTS_OFF					12
#define CPC_SET_CONTROL_INFLOW_REQUEST			13
#define CPC_SET_CONTROL_INFLOW_NONE				14
#define CPC_SET_CONTROL_INFLOW_XONXOFF			15
#define CPC_SET_CONTROL_INFLOW_HARDWARE			16
#define CPC_SET_CONTROL_FLOW_DCD				17
#define CPC_SET_CONTROL_INFLOW_DTR				18
#define CPC_SET_CONTROL_FLOW_DSR				19

#define CPC_LINESTATE_TIMEOUT_ERROR				0x80			/*128 d*/
#define CPC_LINESTATE_TSR_EMPTY					0x40			/*64 d*/
#define CPC_LINESTATE_THR_EMPTY					0x20			/*32 d*/
#define CPC_LINESTATE_BREAK_DETECT				0x10			/*16 d*/
#define CPC_LINESTATE_FRAMING_ERROR				0x08			/*8 d*/
#define CPC_LINESTATE_PARITY_ERROR				0x04			/*4 d*/
#define CPC_LINESTATE_OVERRUN_ERROR				0x02			/*2 d*/
#define CPC_LINESTATE_DATA_READY				0x01			/*1 d*/

#define CPC_MODEMSTATE_CD						0x80			/*128 d*/
#define CPC_MODEMSTATE_RI						0x40			/*64 d*/
#define CPC_MODEMSTATE_DSR						0x20			/*32 d*/
#define CPC_MODEMSTATE_CTS						0x10			/*16 d*/
#define CPC_MODEMSTATE_DELTA_CD					0x08			/*8 d*/
#define CPC_MODEMSTATE_TRLEDGE_RI				0x04			/*4 d*/
#define CPC_MODEMSTATE_DELTA_DSR				0x02			/*2 d*/
#define CPC_MODEMSTATE_DELTA_CTS				0x01			/*1 d*/

#define CPC_PURGEDATA_RECVBUFF					1
#define CPC_PURGEDATA_XMITBUFF					2
#define CPC_PURGEDATA_BOTH						3


#ifndef MIN
#define MIN(x,y) (x) > (y) ? (y) : (x)
#endif

#ifndef MAX
#define MAX(x,y) (x) > (y) ? (x) : (y)
#endif


/* 	 Global data references */
extern char *program_name 	 	   						    ;
extern int  sabre_network_port								;
extern int signo											;
extern TELNET_OPTIONS tnoptions[MAX_TELNET_OPTIONS]         ;
extern int useconds											;
extern int noquote											;
extern int raw_flag											;
extern struct config_t conf									;

/* Symbols defined in utilities.c  */
extern char *telnet_cpc_parity2str(unsigned char optcode);
extern char *telnet_cpc_datasize2str(unsigned char datasize);
extern char *telnet_cpc_subopt2str(unsigned char suboptcode);
extern unsigned char *mystrchr(unsigned char *s, int ch, int size);
extern char *telnet_cpc_stopsize2str(unsigned char optcode);
extern char *telnet_option2str(unsigned char option);
extern char *telnet_optcode2str(unsigned char optcode);
extern unsigned long uint2cs(unsigned int data_bits);
extern unsigned long speed_t2ulong(speed_t s);
extern speed_t ulong2speed_t(unsigned long s);
extern char* get_timestamp();
extern int msleep(unsigned long micro_second);
extern char* remove_feedline_from_string(char* str);
extern char* remove_carriage_return_from_string(char* str);
extern char* skip_white_space(char* str);
extern char* retrieve_configuration_entry_name(char *s);
extern char* get_program_name(char* str);
extern int get_options_from_input(int, char* argv[]);
extern void string_to_lower(char* str);
extern char* token_type_to_str(int tokentype);
extern int boolean_string_handle(char* value);
extern int save_value(char* value, int type, void* target);
extern int valid_serial_speed(unsigned long speed);
extern unsigned long parse_entry(char* buffer,struct config_entry *entry_table);

/*
 Symbols defined in escape_sequence_handle.c
*/
extern int escape_sequence(int c);
extern char *escape_sequence_copy(char *s1,char *s2);
extern int retrieve_octal_escape_sequence(char *s, int *len);
extern int retrieve_hex_escape_sequence(char *s, int *len);


/*
 Symbols defined in serial_ip.c
*/
extern void serial_ip_init();
extern void program_clean_up(void);

/*
Symbols defined in mydaemon.c
*/
extern void mydaemon(int ignsigcld, char *directory);
extern void daemon_start(int ignsigcld);
extern void reopen_stdfds(void);
extern void reopen_fd(char *file,int mode);
extern void reopen_FILE(char *file,char *mode, FILE *stream);
extern void close_all_fds(int begin);

/*
Symbols defined in configuration.c
*/
extern int serial_init_termios(SERIAL_INFO *sabre_serial_port, int *fd, struct termios old_setting, struct termios new_setting);
extern int keyword_table_init(struct config_entry keyword_table[]);
extern int read_configuration_file(char *file, struct config_t *conf);
extern int lookup_user(char *user, uid_t *uid);
extern int lookup_group(char *group, gid_t *gid);
extern int is_directory(char *name);
extern int sane_config(struct config_t *conf);

/*
 Symbols defined in serial_handle.c
 */
extern void serial_cleanup(SERIAL_INFO *sabre_serial_port, int *serial_file_descriptor, struct termios old_setting, struct termios new_setting);
extern unsigned char get_stopsize(int serial_file_descriptor);
extern int set_stopsize(int serial_file_descriptor, unsigned long value);
extern unsigned char get_parity(int serial_file_descriptor);
extern int set_parity(int serial_file_descriptor, unsigned long value);
extern unsigned char get_datasize(int serial_file_descriptor);
extern int set_datasize(int serial_file_descriptor, unsigned long value);
extern unsigned long get_baudrate(int serial_file_descriptor);
extern int set_baudrate(int serial_file_descriptor, unsigned long value);
extern SERIAL_INFO *select_serial_port(struct config_t *conf);
extern SERIAL_INFO *serial_port_init(int *fd, struct termios old_setting, struct termios new_setting);
extern void free_serial_port(SERIAL_INFO *serial_port);
extern void free_all_serial_ports(SERIAL_INFO *serial_ports[]);
extern int add_serial_port_info(struct config_t *conf, char *device_path);
extern SERIAL_INFO *alloc_serial_port(void);
extern int release_serial_port(SERIAL_INFO *sabre_serial_port);

/*
 Symbols defined in signal_handle.c
*/
extern void child_sighandler(int signal);
extern void child_signal_received(int signal);
extern void reset_signals(void);
extern pid_t wait_child_termination(pid_t pid, int *child_status);
extern void log_termination_status(pid_t pid, int status);
extern void action_sigterm(int signal);
extern void action_sighup(int signal);
extern void action_sigchild(int signal);
extern void action_sigpipe(int signal);
extern void action_sigusr1(int signal);
extern void action_sigusr2(int signal);
extern void parent_signal_received(int signal);
extern void telnet_sigint(void);
extern void child_pending_signal_handle(void);
extern void install_signal_handlers(void);
extern char *signame(int signal);

/*
 Symbols defined in debug_handle.c
 */
extern void debug_linedump(char *buf, int len, FILE *stream);
extern void debug_perror(char *str);
extern FILE *open_debug(char *file, int level, char *program);
extern void write_to_debuglog(int level, char *fmt, ...);
extern void close_debug(void);
extern void set_debug_level(int new_level);
extern int get_debug_level();

/*
 Symbols defined in pidfile_handle.c
*/
extern int unlock_uucp_lockfile(char *device);
extern int verify_device_lock_state(char *device_lockfile, pid_t *ret_pid);
extern char *get_uucp_lock_device_file(char *device);
extern char *uucp_lock_filename(char *device);
extern char *get_uucp_template_path(void);
extern char *uucp_tmp_filename(pid_t pid);
extern char *create_uucp_lockfile(char *device, pid_t pid);
extern int write_pidfile(char *pathname, int flags, mode_t mode, pid_t pid);
extern void create_pidfile(void);

/*
 Symbols defined in systemlog_handle.c
 */
extern char *slinedump(char *str, char *buf, int len);
extern void syslog_linedump(char *buf, int len);
extern void write_to_systemlog(int level, char *fmt, ...);
extern void system_errorlog(char* str);

/*
 Symbols defined in network_handle.c
 */
extern int serial_ip_communication_process(int sockfd, int serial_file_descriptor, struct termios newterm, SERIAL_INFO *sabre_serial_port);
extern void concurrent_server(int sockfd, int (*funct)(int), int *signo, void (*sighandler)(int));
extern void iterative_server(int sockfd, int (*funct)(int), int *signo, void (*sighandler)(int));
extern void raw_TCP_gateway(int sockfd, int (*funct)(int), int *signo, void (*sighandler)(int));
extern int handle_network_connection(int sockfd);
extern int set_nonblocking_mode(int sockfd);
extern int create_server_socket(int tcp_network_port,int block_mode);
extern void server_init(int tcp_network_port);
extern void disconnect(int sockfd);

/*
 Symbols defined in network_controller.c
 */
extern int write_socket(int sockfd, BUFFER *serial_to_socket_buf);
extern int read_serial(int serial_file_descriptor, BUFFER *serial_to_socket_buf);
extern int write_serial(int serial_file_descriptor, BUFFER *socket_to_serial_buf);
extern int read_socket(int sockfd, int serial_file_descriptor, BUFFER *socket_to_serial_buf, BUFFER *sabre_to_socket_buf);
extern int network_init(int sockfd, int blockopt);
extern int parent_accept_socket_connection(int sockfd);

/*
Symbols defined in telnet.c
*/
extern int send_telnet_cpc_suboption(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned char *content, int cmdlen);
extern int respond_telnet_cpc_signature_subopt(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned char *command, int cmdlen);
extern int respond_telnet_cpc_baudrate_subopt(int sockfd, int serial_file_descriptor, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned long value);
extern int respond_telnet_cpc_datasize_subopt(int sockfd, int serial_file_descriptor, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned long value);
extern int respond_telnet_cpc_parity_subopt(int sockfd, int serial_file_descriptor, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned long value);
extern int respond_telnet_cpc_stopsize_subopt(int sockfd, int serial_file_descriptor, BUFFER *sabre_to_socket_buf, unsigned char suboptcode, unsigned long value);
extern void telnet_cpc_log_subopt(char *prefix, unsigned char suboptcode, unsigned long value, unsigned char *command, int cmdlen);
extern int process_telnet_cpc_suboption(int sockfd, int serial_file_descriptor, BUFFER *socket_to_serial_buf, BUFFER *sabre_to_socket_buf, unsigned char *optstr, int optlen);
extern void escape_iac_chars(BUFFER *serial_to_socket_buf);
extern void enable_telnet_client_option(unsigned char option);
extern void disable_telnet_client_option(unsigned char option);
extern void enable_telnet_server_option(unsigned char option);
extern void disable_telnet_server_option(unsigned char option);
extern int respond_telnet_binary_option(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char optcode, unsigned char option);
extern int respond_known_telnet_option(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char optcode, unsigned char option);
extern int respond_telnet_option(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char optcode, unsigned char option);
extern void process_telnet_options(int sockfd, int serial_file_descriptor, BUFFER *socket_to_serial_buf, BUFFER *sabre_to_socket_buf);
extern int telnet_client_option_is_enabled(unsigned char option);
extern int telnet_client_option_is_disabled(unsigned char option);
extern int telnet_server_option_is_enabled(unsigned char option);
extern int telnet_server_option_is_disabled(unsigned char option);
extern int send_telnet_option(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char optcode, unsigned char option);
extern void mark_telnet_option_as_sent(unsigned char optcode, unsigned char option);
extern int telnet_option_was_sent(unsigned char optcode,unsigned char option);
extern int send_init_telnet_option(int sockfd, BUFFER *sabre_to_socket_buf, unsigned char optcode, unsigned char option);
extern int telnet_init(int sockfd, BUFFER *sabre_to_socket_buf);


/*
 Symbols defined in buffer_handle.c
*/
extern int bfinsch(BUFFER *buff, unsigned char *ptr, unsigned char ch);
extern unsigned char *bfstrchr(BUFFER *buff, unsigned char ch);
extern int bfrmstr(BUFFER *buff, unsigned char *ptr, int nbytes);
extern int bf_get_nbytes_active(BUFFER *buff);
extern unsigned char *bf_point_to_active_portion(BUFFER *buff, int *amount);
extern unsigned char *bfstrstr(BUFFER *buff, unsigned char *str);
extern int bfeof(BUFFER *buff);
extern int cal_numbytes_to_read(BUFFER *buff);
extern int read_from_fd_to_buffer(int fd, BUFFER *buff);
extern void buffer_writepointer_position(BUFFER *buff, int nbytes);
extern int cal_numbytes_to_write(BUFFER *buff);
extern int write_from_buffer_to_fd(int fd, BUFFER *buff);
extern void buffer_readpointer_position(BUFFER *buff,int nbytes);
extern int bfstrcat(BUFFER *buff, const char *command);
extern int bfstrncat(BUFFER *buff, const char *command, int nbytes);
extern void memdump(char *buf, int len, FILE *stream);
extern void bfdump(BUFFER *buff, int opt);
extern void bffree(BUFFER *buff);
extern void bfinit(BUFFER *buff);
extern BUFFER *bfmalloc(char *label, int size);

/*
 Symbols defined in raw.c
*/
extern int raw_TCP_socket_to_serial(int sockfd, int serial_file_descriptor, void* buff, size_t bufflen);
extern int read_serial_raw_data(int serial_file_descriptor, BUFFER *serial_to_socket_buf);
extern int raw_data_to_TCP_socket(int serial_file_descriptor, int sockfd, void* buff, size_t bufflen);
extern void pack_uint16_t(int pack, uint16_t *num);
extern ssize_t read_from_raw_tcp_buffer(int sockfd, void *buff, size_t bufflen);
extern ssize_t read_from_raw_serial_socket(int sockfd, void *buff, size_t bufflen);
extern ssize_t serialip_send(int sockfd, void *buff, size_t bufflen);


#endif /* SERIAL_IP_H_ */
