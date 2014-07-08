/*
 * Temporary front-end for clasp.
 *
 * Clasp uses non-standard (from a Unix perspective) exit status values.
 * This front-end runs clasp and returns a standard exit status.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

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

#include <string>

using namespace std;

int main(int argc,char *argv[])
{	string args;
	int i,ret;

	args="clasp";
	for(i=1;i<argc;i++)
		args+=" \""+((string)argv[i])+"\"";

	ret=system(args.c_str());

	switch(WEXITSTATUS(ret))
	{	case EXIT_FAILURE:
		case 107:	/* S_MEMORY */
			return(EXIT_FAILURE);
		default:
			return(EXIT_SUCCESS);
	}

}
