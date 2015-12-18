/*
 * pidfile_handle.c
 *	This is to handle pid files.
 *  Created on: Aug 7, 2015
 *      Author: tientham
 */

#include "serial_ip.h"


/*	Author note:
 * 		1. Make change on uucp_tmp_filename.
 * 		2. Make change on verify_device_lock_state(). We do not need to handle a symbolic link.
 */


/*
	Location: pidfile_handle.c

	This is to remove the uucp lock file for the specified serial device.

	returns 0 on success, non-zero on failure.
*/
int unlock_uucp_lockfile(char *device)
{
	extern int errno;
	char *lockfile;						/* lock file name */
	int ret;							/* gp int */

	lockfile = get_uucp_lock_device_file(device); /* ptr to lock file name */
	ret = unlink(lockfile);
	if (ret == 0) {
		write_to_systemlog(LOG_INFO,"removed uucp lock file %s", lockfile);
	} else {
		write_to_systemlog(LOG_ERR,"unlink(%s) error: %s", lockfile, strerror(errno));
	}
	return(ret);
}


/*
	Location: pidfile_handle.c
	This is to read the uucp lock file whose pathname is specified 	and verify that the pid contained within, is valid.
	If we can open and read the lock file: returns 0 if the lock is valid (ie. the pid still exists), returns 1 if
	the lock is invalid (ie. the pid doesn't exist).
	Returns -1 if we can't open or read the lock file.
*/
int verify_device_lock_state(char *device_lockfile, pid_t *ret_pid)
{
	extern int errno;
	int fd;															/* fd for file */
	struct stat stat_buf;											/* stat buffer */
	char *p;
	int ret;														/* gp int */
	pid_t pid;														/* process id */
	int error;														/* error flag */

	syslog(LOG_DEBUG, "pidfile_handle.c: verify_dls(): inside verify function");
	if (ret_pid != NULL) {
		*ret_pid = 0;												/* init returned pid */
	}

	fd = open(device_lockfile, O_RDONLY);							/* open file */
	if (fd < 0) {
		syslog(LOG_DEBUG, "pidfile_handle.c: verify_dls(): open(%s,O_RDONLY) error: %s",
				device_lockfile, strerror(errno));
		return(-1);													/* fail */
	}else
		syslog(LOG_DEBUG, "pidfile_handle.c: verify_dls(): open %s okey with O_RDONLY flag!", device_lockfile);

	if (fstat(fd, &stat_buf) != 0) {									/* get file status */
		syslog(LOG_DEBUG, "pidfile_handle.c: verify_dls(): fstat() error: %s", strerror(errno));
		close(fd);													/* close file */
		return(-1);													/* fail */
	}else
		syslog(LOG_DEBUG, "pidfile_handle.c: verify_dls(): fstat() ok");

	error = 0;														/* clear error flag */
	syslog(LOG_DEBUG, "pidfile_handle.c: verify_dls(): size of %s is %jd and sizeof a %d is: %jd", device_lockfile, (intmax_t) stat_buf.st_size, pid, (intmax_t) sizeof(pid));

	if (stat_buf.st_size == sizeof(pid))							/* binary pid - correspond to hard link file (regular file)*/
	{
		ret = read(fd, &pid, sizeof(pid));							/* read device lock file. */
		if (ret != sizeof(pid))
		{
			++error;
			write_to_debuglog(LOG_ERR,"pidfile_handle.c: verify_dls(): error reading pid from %s: %s",
					device_lockfile, strerror(errno));
		}
	} else{
		//return(-1);
		;
	}
	syslog(LOG_DEBUG, "pidfile_handle.c: verify_dls(): now close %s file descriptor", device_lockfile);
	close(fd);														/* close file */
	if (error)
		return(-1);													/* fail */

	if (ret_pid != NULL) {
		*ret_pid = pid;												/* return the pid */
	}
	syslog(LOG_DEBUG, "pidfile_handle.c: verify_dls(): lock file %s contains pid %d", device_lockfile, pid);
	//write_to_debuglog(LOG_INFO,"pidfile_handle.c: verify_dls(): lock file %s contains pid %d", device_lockfile, pid);
	if (pid < 1) return(-1);										/* fail */
	syslog(LOG_DEBUG, "pidfile_handle.c: verify_dls(): terminating pid %d....", pid);
	ret = kill(pid,0);												/* signum = 0 to check the validaty of process pid. */
	if ((ret == -1) && (errno == ESRCH)){
		syslog(LOG_DEBUG, "pidfile_handle.c: verify_dls(): pdi doesnot exist, verify return 1");
		return(1);													/* pid doesn't exist */
	}
	syslog(LOG_DEBUG, "pidfile_handle.c: verify_dls(): pdi exist, verify return 0");
	return(0);														/* pid exists */
}


/*
	Location: pidfile_handle.c
	This function is to return a ptr to the uucp lock file name for the specified
	device.
*/
char *get_uucp_lock_device_file(char *device)
{
	static char device_lockfile[PATH_MAX];		/* lock file name */
	char *template;
	char *name;
	struct stat st;

	syslog(LOG_DEBUG, "pidfile_handle.c: get_uucp_lock_device_file(): starting get_uucp_template_path()...");
	template = get_uucp_template_path();	/* get ptr to "/var/lock/LCK..%s" */
	syslog(LOG_DEBUG, "pidfile_handle.c: get_uucp_lock_device_file(): template_path is: %s", template);
	device_lockfile[0] = '\0';						/* init lockfile */
	if (device != NULL)
	{
		name = strrchr(device,'/');		/* point to last '/' in device name */
		if (name != NULL)
			name++;						/* point to char after the last '/' */
		else
			name = device;
	if (strstr(template,"%s") != NULL){       /* point to %s in template.*/
		sprintf(device_lockfile,template,name);			/* template contains "%s" */
		syslog(LOG_DEBUG, "pidfile_handle.c: get_uucp_lock_device_file(): device lock file is: %s", device_lockfile);
	}
	if (device_lockfile[0] == '\0')
		sprintf(device_lockfile,template,name);
	} else {
		sprintf(device_lockfile,template,"null");
	}
	syslog(LOG_DEBUG, "pidfile_handle.c: get_uucp_lock_device_file(): device lock file is: %s", device_lockfile);
	return(device_lockfile);
}

/*
	Location: pidfile_handle.c
	This file is to get the template path for uucp lock file.
	Return a pointer to this lock file.
*/
char *get_uucp_template_path(void)
{
	extern struct config_t conf;
	static char template[PATH_MAX];		/* file name template */
	char *p;
	/* uucp lock directory */
	if (conf.lockdir != NULL) {
		strcpy(template,conf.lockdir);
	} else {
		strcpy(template,UUCPLOCK_DIR);
	}

	/* append '/' */
	strcat(template,"/");

	/* append the lock template, eg. "LCK..%s" */
	if (conf.locktemplate != NULL) {
		strcat(template,conf.locktemplate);
	} else {
		strcat(template,UUCPLOCK_TMPL);
	}
	return(template);
}

/*
	Location: pidfile_handle.c
	This file is to return a ptr to a temporary file in the uucp lock directory.
*/
char *uucp_tmp_filename(pid_t pid)
{
	static char tmpname[PATH_MAX];		/* tmp file name */
	char *template;
	char *p;							/* gp ptr */

	template = get_uucp_template_path();	/* get ptr to "/var/lock/LCK..%s" */
	sprintf(tmpname, template, "tmp.");
	syslog(LOG_DEBUG, "pidfile_handle.c: uucp_tmp_filename(): tmpname is: %s", tmpname);
	p = tmpname + strlen(tmpname);		/* point to the null */
	sprintf(p, "%d", pid);
	syslog(LOG_DEBUG, "pidfile_handle.c: uucp_tmp_filename(): pointer p of tmpname is: %s", p);
	return(tmpname);					/*	Author note: update.	*/
}

/*
	Location: pidfile_handle.c
	This is to create a uucp lock file for serial port and write the specified pid to it.
	- Firstly, we need to create a temp file in which we can write process id of which process holding this function.
	- Secondly, we also need to create a device lock file for sabre lite with the port name ttycmx0.
	- Finally, link temp file system and device lock file together.
	returns a ptr to the lock file name on success, NULL on failure.
*/
char *create_uucp_lockfile(char *device, pid_t pid)
{
	extern int errno;
	signed int ret;														/* gp int */
	mode_t mode;													/* file mode */
	pid_t lockpid;													/* pid which owns lock file */
	char *tmpname;													/* tmp file name */
	char *device_lockfile;											/* lock file name of device */
	signed int verify_status;

	tmpname = uucp_tmp_filename(pid);								/* ptr to tmp file name */
	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): tmpname is %s", tmpname);
	mode = 0644;													/* must be writable by owner */

	/* create tmp file and write process pid (which carry this function) to it */
	ret = write_pidfile(tmpname, O_WRONLY|O_CREAT, mode, pid);
	if (ret != 0) return(NULL);

	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): starting get_uucp_lock_device_file");
	device_lockfile = get_uucp_lock_device_file(device); 			/* ptr to lock file name */
	ret = link(tmpname, device_lockfile);							/* link(old_path,new_path) is atomic */
	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): ret = %d", ret);
	if(ret == 0)
		syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link tnmpname and device_lockfile - ok!");
	if (ret != 0) {													/* if link() failed */
		syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link tnmpname and device_lockfile - failed!");
		if (errno == EEXIST) {										/* failed cos existing lock file. Try again. */
			syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 3!");
			syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): starting verify....!");

			verify_status = verify_device_lock_state(device_lockfile, &lockpid);
			syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): verify return value = %d", verify_status);

			if (verify_status == 1)
			{
				syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): verify_device_lock_state =1!");
				ret = unlink(device_lockfile);						/* remove stale lock */
				if (ret == 0) {
					write_to_debuglog(LOG_INFO,"pidfile_handle.c: create_ulf():removed stale uucp lock %s, owned by PID %d",
							device_lockfile, lockpid);
				} else {
					write_to_debuglog(LOG_ERR,"pidfile_handle.c: create_ulf(): unlink(%s) error: %s",
							device_lockfile, strerror(errno));
				}
				sleep(3);											/* avoid race condition */
				syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): starting link again!");
				ret = link(tmpname,device_lockfile);				/* try link() again */
			}
		}
	}
	if (ret != 0) 													/* if link() failed */
	{
		syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): still error...!");
		if (errno != EEXIST) 										/* not due to existing lock file */
		{
			syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 3!");
			write_to_debuglog(LOG_ERR,"pidfile_handle.c: create_ulf(): link(%s,%s) error: %s",
					tmpname, device_lockfile, strerror(errno));
			write_to_debuglog(LOG_ERR,"pidfile_handle.c: create_ulf(): error creating uucp lock file %s",
					device_lockfile);
		}
		syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): Unlinking.. and return NULL!");
		unlink(tmpname);											/* remove tmp file */
		return(NULL);												/* fail */
	}

	if(ret == -1)
	{
		if (errno == EACCES)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 1!");
		if (errno == EDQUOT)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 2!");
		if (errno == EEXIST)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 3!");
		if (errno == EFAULT)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 4!");
		if (errno == EIO)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 5!");
		if (errno == ELOOP)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 6!");
		if (errno == EMLINK)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 7!");
		if (errno == ENAMETOOLONG)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 8!");
		if (errno == ENOENT)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 9!");
		if (errno == ENOMEM)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 10!");
		if (errno == ENOSPC)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 11!");
		if (errno == ENOTDIR)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 12!");
		if (errno == EPERM)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 13!");
		if (errno == EROFS)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 14!");
		if (errno == EXDEV)	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): link errorno - 15!");
	}

	/* else, link() was successful */

	unlink(tmpname);												/* remove tmp file */
	syslog(LOG_DEBUG, "pidfile_handle.c: create_uucp_lockfile(): starting verify_device_lock_state");
	/* verify that we really got the lock */
	ret = verify_device_lock_state(device_lockfile,&lockpid);
	if ((ret == 0) && (lockpid == pid))
	{
		write_to_debuglog(LOG_INFO,"pidfile_handle.c: create_ulf(): created uucp lock file %s",
				device_lockfile);
		return(device_lockfile);									/* success */
	} else
	{
		write_to_debuglog(LOG_INFO,"pidfile_handle.c: create_ulf(): error verifying uucp lock file %s",
				device_lockfile);
		return(NULL);												/* fail */
	}
}

/*
	Location: pidfile_handle.c
	This is to open and write information about a pid to a given pidfile (if this file exists)
	Otherwise return error on syslog.
	Returns 0 on success, non-zero when failed.
*/
int write_pidfile(char *pathname, int flags, mode_t mode, pid_t pid)
{
	extern int errno;
	int fd;															/* fd for file */

	/* flags: O_WRONLY|O_CREAT, mode: 0644*/
	fd = open(pathname, flags, mode);								/* open/creat file */
	if (fd < 0) {
		write_to_systemlog(LOG_ERR, "pidfile_handle.c: write_pidfile(): open(%s,...) error: %s", pathname, strerror(errno));
		return(fd);													/* fail */
	}else
		syslog(LOG_DEBUG, "pidfile_handle.c: writefile(): open(%s,...) successful!", pathname);

	if (write(fd,&pid,sizeof(pid)) != sizeof(pid))
	{
		write_to_systemlog(LOG_ERR, "pidfile_handle.c: write_pidfile(): error writing pid to %s: %s", pathname, strerror(errno));
		close(fd);													/* close file */
		unlink(pathname);											/* remove it */
		return(1);													/* fail */
	}
	close(fd);														/* close file */
	chmod(pathname,mode);											/* set file mode */
	return(0);														/* success */
}

/*
	Location: pidfile_handle.c
	0644 flag mode (octal express): user - read&write; group - read; other - read.
	This is to write our process ID to a file.
*/
void create_pidfile(void)
{
	extern struct config_t conf;									/* built from config file */
	int ret;

	ret = write_pidfile(conf.pidfile, O_WRONLY|O_CREAT|O_TRUNC, 0644, getpid());
	if (ret != 0)
		syslog(LOG_ERR, "pidfile_handle.c: create_pidfile() cannot create pid file %s", conf.pidfile);
	else
		syslog(LOG_DEBUG, "pidfile_handle.c: create_pidfile() - pid file %s is created", conf.pidfile);
}














