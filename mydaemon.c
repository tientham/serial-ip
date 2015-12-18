/*
 * mydaemon.c
 *	This is derived from "telnetcpcd" project of Thosmas J Pinkl.
 *  Created on: Aug 4, 2015
 *      Author: tientham
 */

#include "serial_ip.h"

#ifndef NOFILE
#define NOFILE	60
#endif
#ifndef DAEMON_STDIN
#define DAEMON_STDIN	"/dev/null"
#endif
#ifndef DAEMON_STDOUT
#define DAEMON_STDOUT	"/dev/console"
#endif
#ifndef DAEMON_STDERR
#define DAEMON_STDERR	DAEMON_STDOUT
#endif
#ifndef FAILSAFE
#define FAILSAFE		"/dev/null"
#endif

/*
	Detach a daemon process from login session context.
	ignore_sigcld nonzero means handle SIGCLD so zombies
	don't clog up the process table.
*/
void mydaemon(int ignore_sigcld,char *dir)
{
	int childpid;						/* child pid */
	struct sigaction act;

	/*
	 * If we were started by init (process 1) from the /etc/inittab file
	 * there's no need to detach.
	 * This test is unreliable due to an unavoidable ambiguity
 	 * if the process is started by some other process and orphaned
	 * (i.e., if the parent process terminates before we are started).
  	 */
	if (getppid() == 1)
  		goto out;
	/*
	 * Ignore the terminal stop signals
	 */
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);			/* required before sigaction() */

#ifdef SIGTTOU
	sigaction(SIGTTOU,&act,NULL);
#endif
#ifdef SIGTTIN
	sigaction(SIGTTIN,&act,NULL);
#endif
#ifdef SIGTSTP
	sigaction(SIGTSTP,&act,NULL);
#endif

	/*
	 * If we were not started in the background, fork and
	 * let the parent exit.  This also guarantees the first child
	 * is not a process group leader.
	 */
	if ((childpid = fork()) < 0) {
		system_errorlog("can't fork() first child");
		exit(1);
	} else {
		if (childpid > 0)
			_exit(0);					/* parent */
	}

	/*
	 * First child process.
	 *
	 * Disassociate from controlling terminal and process group.
	 * Ensure the process can't reacquire a new controlling terminal.
	 */

#ifdef BSD
#if defined(__osf__)
	if (setpgrp((pid_t) 0, getpid()) == -1) {
#else
	if (setpgrp() == -1) {
#endif
		syslog_perror("can't change process group");
		exit(1);
	}

	/*
		lose controlling tty
	*/
	if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
		ioctl(fd, TIOCNOTTY, (char *)NULL);
		close(fd);
	}

#else	/* System V */

#ifndef NO_SETSID
	if (setsid() < 0) {					/* create a new session and become */
										/* the process group leader */
		system_errorlog("setsid() error");
		exit(1);
	}
#else
	if (setpgrp() == -1) {
		system_errorlog("setpgrp() error");
		exit(1);
	}
#endif	/* NO_SETSID */

	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);			/* required before sigaction() */
	sigaction(SIGHUP,&act,NULL);		/* immune from pgrp leader death */

	if ((childpid = fork()) < 0) {
		system_errorlog("can't fork() second child");
		exit(1);
	} else {
		if (childpid > 0)
			_exit(0);					/* first child */
	}

	/* second child continues */
#endif	/* BSD or System V */

out:
	/*
		reopen stdin/stdout/stderr
	*/
	reopen_stdfds();

	/*
		Close any remaining open file descriptors.
	*/
	close_all_fds(3);

	/*
	 * Move the current directory to root (probably), to make sure we
	 * aren't on a mounted filesystem.
	 */
	if ((dir == NULL) || (*dir == 0x00)) {
		chdir("/");
	} else {
		chdir(dir);
	}

	/*
	 * Clear any inherited file mode creation mask.
	 */
	umask(0);

	/*
	 * See if the caller isn't interested in the exit status of its
	 * children, and doesn't want to have them become zombies and
	 * clog up the system's process table.
	 * With System V all we need do is ignore the signal.
	 * With BSD, however, we have to catch each signal
	 * and execute the wait3() system call.
	 */
	if (ignore_sigcld) {
#if defined(BSD) && !defined(__osf__)
		int	sig_child();

		signal(SIGCLD, sig_child);			/* BSD */
#else
		act.sa_handler = SIG_IGN;
		sigemptyset(&act.sa_mask);			/* required before sigaction() */
		sigaction(SIGCLD,&act,NULL);		/* System V */
#endif
	}
}

void daemon_start(int ignore_sigcld)
{
	mydaemon(ignore_sigcld,"/");
}

/*
	reopen stdin/stdout/stderr
*/
void reopen_stdfds(void)
{
	close(0);
	close(1);
	close(2);

	/* stdin */
	reopen_fd(DAEMON_STDIN,O_RDONLY|O_NOCTTY|O_NONBLOCK);
	reopen_FILE(DAEMON_STDIN,"r",stdin);

	/* stdout */
	reopen_fd(DAEMON_STDOUT,O_WRONLY|O_APPEND|O_NOCTTY|O_NONBLOCK);
	reopen_FILE(DAEMON_STDOUT,"a",stdout);

	/* stderr */
	reopen_fd(DAEMON_STDERR,O_WRONLY|O_APPEND|O_NOCTTY|O_NONBLOCK);
	reopen_FILE(DAEMON_STDERR,"a",stderr);
}

void reopen_fd(char *file,int mode)
{
	extern int errno;
	int fd;

	fd = open(file,mode);
	if (fd == -1) {
		syslog(LOG_ERR,"open(%s,%d) error: %s",file,mode,strerror(errno));
		fd = open(FAILSAFE,mode);
		if (fd == -1) {
			syslog(LOG_ERR,"open(%s,%d) error: %s",FAILSAFE,mode,strerror(errno));
		} else {
			/* use LOG_ERR for this too */
			syslog(LOG_ERR,"open(%s,%d) succeeded",FAILSAFE,mode);
		}
	}
}

void reopen_FILE(char *file,char *mode,FILE *stream)
{
	extern int errno;
	FILE *fp;

	fp = freopen(file,mode,stream);
	if (fp == NULL) {
		syslog(LOG_ERR,"freopen(%s,%s,...) error: %s",file,mode,strerror(errno));
		fp = freopen(FAILSAFE,mode,stream);
		if (fp == NULL) {
			syslog(LOG_ERR,"freopen(%s,%s,...) error: %s",FAILSAFE,mode,strerror(errno));
		} else {
			/* use LOG_ERR for this too */
			syslog(LOG_ERR,"freopen(%s,%s,...) succeeded",FAILSAFE,mode);
		}
	}
}

void close_all_fds(int begin)
{
	extern int errno;
	int fd;								/* file descriptor */
	long open_max;						/* max open fd's */

	open_max = sysconf(_SC_OPEN_MAX);
	if (open_max == -1) {
		system_errorlog("sysconf(_SC_OPEN_MAX) error");
		open_max = NOFILE;
	}
	closelog();
	for (fd = begin; fd < open_max; fd++) {
		close(fd);
	}
	errno = 0;							/* probably got EBADF from close() */
}

