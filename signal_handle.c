/*
 * signal_handle.c
 *	This is to manipulate signals.
 *  Created on: Aug 5, 2015
 *      Author: tientham
 */

/*
 	 	 Author note: 1. update pid condition in wait_child_termination()
*/

#include "serial_ip.h"

int signo_child = 0;      /* Global child signal, 0 by default.	*/
/*
	Location: signal_handle.c
	This is a generic signal handler. Log which signal occurred, tidy up a bit, then we terminate.
	does not return.
*/
void child_sighandler(int signal)
{
	extern int gsockfd;					/* global copy of sockfd */

	syslog(LOG_ERR,"signal_handle.c: child_sighandler: received signal %d (%s)", signal, signame(signal));
	if (gsockfd >= 0)
		disconnect(gsockfd);
	_exit(128 + signal);
}
/*
	Location: signal_handle.c
	this function is called from handle_pending_signal() upon receipt of a SIGINT
*/
void telnet_sigint(void)
{
	extern int break_signaled;
	extern unsigned char linestate;

	break_signaled = 1;
	linestate |= CPC_LINESTATE_BREAK_DETECT;
}
/*
	Location: signal_handle.c
	This is to handle a pending signal.
*/
void child_pending_signal_handle(void)
{
	extern int signo_child;									/* what signal did we receive? */

	switch (signo_child) {
	case 0:													/* none */
		break;
	case SIGINT:											/* serial/modem break condition */
		telnet_sigint();
		break;
	case SIGUSR1:
		action_sigusr1(signo_child);						/* increment debug level */
		break;
	case SIGUSR2:
		action_sigusr2(signo_child);						/* decrement debug level */
		break;
	default:
		child_sighandler(signo_child); 						/* does not return */
		break;
	}
	signo_child = 0;										/* clear pending signal; very important! */
}

/*
	Location: signal_handle.c
	this is our generic signal handling function for the child process.	it gets called for any signal we care to handle and simply sets the
	global signo_child variable to the signal number.
	it's then up to code elsewhere in the system to check signo_child and act accordingly.
*/
void child_signal_received(int signal)
{
	extern int signo_child;				/* Global variable for child signal. */
	signo_child = signal;
}

/*
	Location: signal_handle.c
	This is to reset the signal handlers that a child inherited from
	its parent, except for SIGCHLD.
	No return value.
*/
void reset_signals(void)
{
	extern int signo_child;				/* what signal did we receive? */
	struct sigaction act;				/* for signals */

	act.sa_handler = &child_signal_received;	/* function to handle signals */
	sigemptyset(&act.sa_mask);			/* required before sigaction() */
	act.sa_flags = 0;					/* no SA_RESTART */
	sigaction(SIGTERM,&act,NULL);		/* same handler for all 5 signals */
	sigaction(SIGINT,&act,NULL);
	sigaction(SIGQUIT,&act,NULL);
	sigaction(SIGHUP,&act,NULL);
    sigaction(SIGUSR1,&act,NULL);
    sigaction(SIGUSR2,&act,NULL);
	sigaction(SIGPIPE,&act,NULL);

	signo_child = 0;					/* default. */
}

/*
  	  Location: signal_handle.c
  	  This function is to use waitpid().
  	  With the flag WNOHANG and pid = -1;
  	  Return value:
  	  	  -1 on error/unsuccessful.
  	  	  0 if no child status information available which means child does not terminated yet.
  	  	  >0 if sucessful, and the return value is the PID of child terminated.
  	  On sucessful, the status of child terminated will be point to the pointer "status".
*/
pid_t wait_child_termination(pid_t pid, int *child_status)
{
	extern int errno;
	pid_t ret;
	int *status;
	int wstatus;

	/*We need a new status variable for storing child process information.
	 * if child_status is available, create a new status pointer.
	 * Otherwise, use it.*/
	if (child_status != (int *) NULL)
	{
		status = child_status;			/* Use given child_status cose it has no information. */
	} else {
		status = &wstatus;				/* Using new status for storing child process terminated information. */
	}
	/*
	  	  waitpid with flag WNOHANG will put the calling being non-blocked during the systemcall waitpid().
	 */
	ret = waitpid(pid,status,WNOHANG);
	if ((ret == (pid_t) 0) && (pid == (pid_t) -1)) 			/*Author note: update pid condition!!!!*/
	{
		/*
			Status not yet available for specified pid.
			we'll wait a second and try again
		*/
		syslog(LOG_DEBUG,"signal_handle.c: wait_child_termination() returned 0; will try again");
		sleep(1);
		ret = waitpid(pid,status,WNOHANG);
	}
	if (ret == (pid_t) -1)
	{
		if (errno == ECHILD)
		{
			;	/* syslog(LOG_DEBUG,"signal_handle.c: wait_child_termination(): no more child processes"); */
		} else {
			system_errorlog("signal_handle.c: wait_child_termination() error");
		}
	} else {
		if (ret > 0)  /* On successful.*/
		{
			log_termination_status(ret, *status);
		}
	}
	return(ret);
}

/*
	Location: signal_handle.c
	interpret a child's exit status and send the
	result to the syslog
*/
void log_termination_status(pid_t pid, int status)
{
	char *str;

	str = "child PID";
	if (WIFEXITED(status))
	{
    syslog(LOG_ERR, "signal_handle.c: log_termination_status(): %s %d terminated normally by calling exit(%d)",
    			str, pid, WEXITSTATUS(status));
	} else if (WIFSIGNALED(status))
	{
    syslog(LOG_ERR,"signal_handle.c: log_termination_status(): %s %d terminated by unhandled %s signal (number %d)",
    			str, pid, signame(WTERMSIG(status)), WTERMSIG(status));
	} else
	{
    syslog(LOG_ERR,"signal_handle.c: log_termination_status(): %s %d unknown termination status, value: %d",
    			str, pid, status);
	}
}

/*
	Location: signal_handle.c
	This function is to define a propriate action for SIGTERM, SIGINT, SIGQUIT.
	returns nothing.
*/
void action_sigterm(int signal)
{
	struct sigaction sa;
	/* log this at the ERR level to be sure it reaches the log */
	syslog(LOG_ERR,"signal_handle.c: action_sigterm() received signal %d (%s)",signal,signame(signal));

	program_clean_up();				/* close everything */
	sa.sa_handler = SIG_DFL;			/* set default handler for signal*/
	sigemptyset(&sa.sa_mask);			/* required before sigaction() */
	sa.sa_flags = 0;					/* signal is fatal, so no SA_RESTART */
	sigaction(SIGTERM,&sa,NULL);

	/* kill parent process.*/
	kill(0,SIGTERM);					/* kill(int pid, int signum) */
}

/*
	Location: signal_handle.c
	This function is to define a propriate action for SIGHUP.
	SIGHUP is to re-initialize our program.
	returns nothing.
*/
void action_sighup(int signal)
{
		/* log this at the ERR level to be sure it reaches the log */
		syslog(LOG_ERR,"received signal %d (%s)",signal,signame(signal));

		program_clean_up();				/* close everything */
		serial_ip_init();					/* initialization program. */
}

/*
	Location: signal_handle.c
	This function is to handle the SIGCLD signal.
	We collect the child process's termination status and log it.
	This is important to keep track of zoombie processes.
	returns nothing.
*/
void action_sigchild(int signal)
{
	pid_t pid;							/* pid of child */
	int status;							/* child's termination status */

	/* log this at the ERR level to be sure it reaches the log */
	syslog(LOG_ERR,"received signal %d (%s)",signal,signame(signal));

	while (1) {
    	pid = wait_child_termination((pid_t) -1, &status);	/* get pid and child's status (note: status now is not available yet).
    	 	 	 	 	 	 	 	 	 	 	 -1 for the calling process wait for any child process terminate.	 */
		if (pid == -1) {
			if (errno == EINTR) {		/* wait() interrupted by a signal */
				continue;				/* wait() again */
			} else {					/* errno == ECHILD, no more children */
				break;
			}
		} else if (pid == (pid_t) 0) {	/* no status available */
			break;
		} else {						/* got pid and status */
			;							/* nothing to do */
		}
	}
}

/*
	Location: signal_handle.c
	This function is to handle the SIGPIPE signal.
	This is signal comming from kernel, we just need to get a system log for debugging.
	returns nothing.
*/
void action_sigpipe(int signal)
{
	/* log this at the ERR level to be sure it reaches the log */
	syslog(LOG_ERR,"signal_handle.c: action_sigpipe() received signal %d (%s)",signal,signame(signal));
}

/*
	Location: signal_handle.c
	sigusr1() and sigusr2() provide a means for us to change
	the debug level on the fly.
*/

/*
	Location: signal_handle.c
	This is to handle SIGUSR1.
	Action: Increment the debug level.
*/
void action_sigusr1(int signal)
{
	int level;
	/*
		increment the debug level
	*/
	level = get_debug_level();
	if (level < DBG_LV9) {
		++level;
		set_debug_level(level);
	}
	/* log this at the LOG_ERR level to be sure it reaches the log */
    syslog(LOG_ERR,"Received %s signal.  Debug level now %d.",signame(signal),level);
}

/*
	Location: signal_handle.c
	This is to handle SIGUSR2.
	Action: Decrement the debug level.
*/
void action_sigusr2(int signal)
{
	int level;

	/*
		decrement the debug level
	*/
	level = get_debug_level();
	if (level > DBG_LV0) {
		--level;
		set_debug_level(level);
	}
	/* log this at the LOG_ERR level to be sure it reaches the log */
    syslog(LOG_ERR,"Received %s signal.  Debug level now %d.",signame(signal),level);
}

/*
 	Location: signal_handler.c
	This is to handle signal received inside parent process.
	Receive the signal and return nothing.
*/
void parent_signal_received(int signal)
{
	extern int signo;					/* signo is global variable for signal we get */

	switch (signal)
	{
		case SIGTERM:
		case SIGINT:
		case SIGQUIT:
			action_sigterm(signal);					/* does not return */
			break;
		case SIGHUP:
			action_sighup(signal);					/* re-read config file */
			break;
		case SIGCLD:
			action_sigchild(signal);					/* child process died */
			break;
		case SIGPIPE:
			action_sigpipe(signal);
			break;
		case SIGUSR1:
			action_sigusr1(signal);					/* increment debug level */
			break;
		case SIGUSR2:
			action_sigusr2(signal);					/* decrement debug level */
			break;
	}
	signo = signal;						/* save signal number in global var */
}

/*
	Location: signal_handle.c
	This function is to install signal handlers for SIGTERM, SIGINT, SIGQUIT, SIGHUP, SIGCLD
	SIGUSR1, SIGUSR2, and SIGPIPE.
	a sigaction(SIGTERM,&sa,NULL) means we define signal handler for SIGTERM which will perform "sa" function.
	Procedure here is:
	1. Define a signal struct "sa" which is a function action for a specific signal.
	2. Initialization by allocating "sa" structure with block of '0'.
	3. Direct sa_handler field of "sa" to our defined function action which we want a specific signal do.
	4. Calling function "sigaction(SIGNAL, &sa, NULL).
	5. ...Our code...
*/
void install_signal_handlers(void)
{
	struct sigaction sa;

	//memset(sa, 0, sizeof(sa));
	sa.sa_handler = &parent_signal_received;	/* function to handle all signals */

	/* we NEED signals to interrupt system calls, so don't specify SA_RESTART */
	sigemptyset(&sa.sa_mask);			/* Initialize signal set, it exclude all predefined signal, required before sigaction() */
	sa.sa_flags = 0;					/* no SA_RESTART */
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGHUP,  &sa, NULL);
	sigaction(SIGCLD,  &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);

	syslog(LOG_INFO,"signal_handle.c: installed signal handlers");
}

/*
	Location: signal_handle.c
	This is to return a pointer to a name of a supplied signal.
*/
char *signame(int signal)
{
	register char *p = NULL;

	switch (signal) {
	case SIGHUP:	p = "SIGHUP"; break;
	case SIGINT:	p = "SIGINT"; break;
	case SIGQUIT:	p = "SIGQUIT"; break;
	case SIGILL:	p = "SIGILL"; break;
	case SIGTRAP:	p = "SIGTRAP"; break;
	case SIGABRT:	p = "SIGABRT"; break;
	case SIGFPE:	p = "SIGFPE"; break;
	case SIGKILL:	p = "SIGKILL"; break;
	case SIGBUS:	p = "SIGBUS"; break;
	case SIGSEGV:	p = "SIGSEGV"; break;
	case SIGPIPE:	p = "SIGPIPE"; break;
	case SIGALRM:	p = "SIGALRM"; break;
	case SIGTERM:	p = "SIGTERM"; break;
	case SIGUSR1:	p = "SIGUSR1"; break;
	case SIGUSR2:	p = "SIGUSR2"; break;
	case SIGCLD:	p = "SIGCLD"; break;
	case SIGPWR:	p = "SIGPWR"; break;
	case SIGWINCH:	p = "SIGWINCH"; break;
	case SIGPOLL:	p = "SIGPOLL"; break;
	case SIGSTOP:	p = "SIGSTOP"; break;
	case SIGTSTP:	p = "SIGTSTP"; break;
	case SIGCONT:	p = "SIGCONT"; break;
	case SIGTTIN:	p = "SIGTTIN"; break;
	case SIGTTOU:	p = "SIGTTOU"; break;
	case SIGVTALRM:	p = "SIGVTALRM"; break;
	case SIGPROF:	p = "SIGPROF"; break;
	default:		p = "unexpected"; break;
	}
	return(p);
}
