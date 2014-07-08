#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#else
#  ifndef WEXITSTATUS
#    define WEXITSTATUS(w)   (((w) >> 8) & 0xff)
#  endif
#  ifndef WIFSIGNALED
#    undef _W_INT
#    ifdef _POSIX_SOURCE
#      define	_W_INT(i)	(i)
#    else
#      define	_W_INT(w)	(*(int *)(void *)&(w))	/* convert union wait to int */
#      undef WCOREFLAG
#      define	WCOREFLAG	0200
#    endif
#    undef _WSTATUS
#    define	_WSTATUS(x)	(_W_INT(x) & 0177)
#    undef _WSTOPPED
#    define	_WSTOPPED	0177		/* _WSTATUS if process is stopped */
#    define WIFSIGNALED(x)	(_WSTATUS(x) != _WSTOPPED && _WSTATUS(x) != 0)
#    define WTERMSIG(x)	(_WSTATUS(x))
#  endif
#endif
#if HAVE_SYS_RESOURCE_H
#  include <sys/resource.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "fileutils.h"

#define ALARM_INTERVAL	1	/* secs */
static int cputime_limit=0;		/* secs */

static bool error_on_timeout=false;	/* whether the program should kill itself or just exit with no models on timeout */


/*-------------> Internal Functions <-------------*/

#ifdef _WIN32
#include <time.h>
int get_total_cpu_secs(void)
{	return ((double)clock() / CLOCKS_PER_SEC);
}
#else
int get_total_cpu_secs(void)
{	struct rusage ru;
	int total_sec,total_usec;

	getrusage(RUSAGE_CHILDREN,&ru);
	total_sec=ru.ru_utime.tv_sec+ru.ru_stime.tv_sec;
	total_usec=ru.ru_utime.tv_usec+ru.ru_stime.tv_usec;

	getrusage(RUSAGE_SELF,&ru);
	total_sec+=ru.ru_utime.tv_sec+ru.ru_stime.tv_sec;
	total_usec+=ru.ru_utime.tv_usec+ru.ru_stime.tv_usec;

	return(total_sec+total_usec/1000000);
}
#endif

void verify_cputime(void)
{	if (cputime_limit > 0 && get_total_cpu_secs() >= cputime_limit)
	{	fprintf(stderr,"***crmodels2/cr2: timeout\n");
		exit(2);
//		kill(getpid(),SIGXCPU);	// does not kill children and "|" pipes properly
	}
}

void alarm_function(int sig)
{
#ifdef _WIN32
	fprintf(stderr,"timeouts not supported under Windows!!\n");
#else
	//struct rusage ru;
	//int total_sec,total_usec;

//	printf("ALARM: total time: %d\n",get_total_cpu_secs());
	
	verify_cputime();

	alarm(ALARM_INTERVAL);
#endif
}

/*-------------> Exported Functions <-------------*/

/*
** Returns a string X.XXX representing the
** current USER cputime (INCLUDING children).
**
** WARNING: the string is STATICALLY allocated
*/
#ifdef _WIN32
#include <sys/time.h>
#include <time.h>

timeval *init_cpu_timer(void)
{	static struct timeval tv;

	gettimeofday(&tv,NULL);

	return(&tv);
}

struct timeval *start_tv=init_cpu_timer();

char *get_total_cputime_string(void)
{	static char str[1000];

	struct timeval tv;
	int tot_sec,tot_usec;

	gettimeofday(&tv,NULL);

	if (tv.tv_usec<start_tv->tv_usec)
	{	tv.tv_sec--;
		tv.tv_usec+=1000000;
	}
	tot_usec=tv.tv_usec-start_tv->tv_usec;
	tot_sec=tv.tv_sec-start_tv->tv_sec;

	sprintf(str,"%d.%03d",tot_sec,
			      tot_usec/1000);

	return(str);
}
#else
char *get_total_cputime_string(void)
{	static char str[1000];

	struct rusage ru1,ru2;
	int tot_sec,tot_usec;

	getrusage(RUSAGE_CHILDREN,&ru1);
	getrusage(RUSAGE_SELF,&ru2);

	tot_usec=(ru1.ru_utime.tv_usec+ru2.ru_utime.tv_usec) % 1000000;
	tot_sec=ru1.ru_utime.tv_sec+ru2.ru_utime.tv_sec+
	        (ru1.ru_utime.tv_usec+ru2.ru_utime.tv_usec) / 1000000;

	sprintf(str,"%d.%03d",tot_sec,
			      tot_usec/1000);

	return(str);
}
#endif

/*
** Like system(), but:
**
**  1) if the child process terminates abnormally, the current
**     process is killed with the same signal;
**  2) if a cputime limit was set, the child process is
**     not allowed to run longer than that time.
*/
int timed_system(char *cmd)
{	char *str;
	const char *prefix="ulimit -t ";
	const char *sep=" ; ";

	int ret;

	str=(char *)calloc(strlen(prefix)+100/*max digits for cputime limit*/+
			   strlen(sep)+strlen(cmd)+1,sizeof(char));

	if (cputime_limit>0)
	{
#ifdef _WIN32
		fprintf(stderr,"timeouts not supported under Windows!!\n");
		strcpy(str,cmd);
#else
		verify_cputime();

		sprintf(str,"%s%d ; %s",
				prefix,
				cputime_limit-get_total_cpu_secs(),
				cmd);
#endif
	}
	else
		strcpy(str,cmd);

	//printf("cmd=%s\n",str);
	ret=system(str);

	free(str);

//	process_ret(ret);

#ifndef _WIN32
	if (WIFSIGNALED(ret))	/* child terminated abnormally */
		kill(getpid(),WTERMSIG(ret));
	else if (cputime_limit>0)	/* this second cputime test allows us to catch an exceeded limit */
		verify_cputime();	/* without waiting for another call to timed_system(). */
#endif

	return(WEXITSTATUS(ret));
}

/*
 * Like timed_system(), but runs "mkatoms -a" on the output of the command.
 * 
 * It is better than just using " | mkatoms -a" because this function
 * correctly checks the return status of the command.
 *
 * If an error occurs, outfile contains the output (stdout) of
 * the command that returned error (either cmd or mkatoms).
 */
int timed_mkatoms_system(char *cmd,char *outfile)
{	char *tfile;
	char *s;
	int ret;
	const char *mkatoms_cmd="mkatoms -a";

	tfile=create_temp("smk.XXXXXX");

	if (strlen(cmd)>(strlen(mkatoms_cmd)+1+strlen(outfile)))
		s=(char *)calloc(strlen(cmd)+1+strlen(tfile)+1,sizeof(char));
	else
		s=(char *)calloc(strlen(mkatoms_cmd)+1+strlen(tfile)+1+strlen(outfile)+1,sizeof(char));

	sprintf(s,"%s>%s",cmd,tfile);
	ret=timed_system(s);
	if (ret != 0) // error
	{	sprintf(s,"cp %s %s",tfile,outfile);
		system(s);
	}
	else
	{	sprintf(s,"mkatoms -a<%s>%s",tfile,outfile);
		ret=timed_system(s);
	}

	free(s);
	delete_temp(tfile);
	free(tfile);

	return(ret);
}

/*
 * Like timed_mkatoms_system(), but also displays error information
 * and exits in case of an error during execution of the command.
 */
void timed_mkatoms_system_err(char *cmd,char *outfile)
{	int res;

	if ((res=timed_mkatoms_system(cmd,outfile)) != 0) // error
	{	fprintf(stderr,"***Error while executing %s (%d). Command output:\n",cmd,res);
		catFile(outfile,stderr);
		exit(1);
	}
}

/*
** Used to set the cputime limit.
** If the function is not invoked, the cputime
** is NOT LIMITED by default.
*/
void set_cputime_limit(int cputime_l)
{	cputime_limit=cputime_l;

#ifdef _WIN32
	fprintf(stderr,"timeouts not supported under Windows!!\n");
#else
	if (cputime_limit>0)
	{	signal(SIGALRM,alarm_function);
		alarm(ALARM_INTERVAL);
	}
#endif
}

/*
** Used to set whether we must give error on timeout.
*/
void set_error_on_timeout(bool error_on_timeout_l)
{	error_on_timeout=error_on_timeout_l;
}

const char *solver_cputime_opt(bool cputime_aware_solver)
{	static char s[10240];

	if (cputime_aware_solver && cputime_limit>0)
		sprintf(s,"--cputime %d",cputime_limit-get_total_cpu_secs());
	else
		strcpy(s,"");

	return(s);
}
