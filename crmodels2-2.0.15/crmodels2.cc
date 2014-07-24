/*
 * crmodels
 *
 * by Marcello Balduccini (marcello.balduccini@gmail.com)
 *
 * Copyright Eastman Kodak Company 2009-2012  All Rights Reserved
 * ----------------------------------------------------------------
 *
 *
 *   see function show_usage() for usage
 *
 *-------------------------------------
 *   
 *
 *   History
 *  ~~~~~~~~~
 *   03/07/12 - [2.0.15] Removed dependency from any standard 
 *                       Unix system-wide program to solve
 *                       problems with Windows/mingw32.
 *   03/05/12 - [2.0.14] Error messages added for Windows/mingw32.
 *   03/05/12 - [2.0.13] Fixes for Windows/mingw32:
 *                        - unlink() now called before rename()
 *                        - "egrep | wc -l" replaced by C code
 *                        - get_total_cputime_string() rewritten
 *   03/01/12 - [2.0.12] hr now uses anonymous variables in #show
 *                       to avoid problems with recent versions of
 *                       gringo.
 *   02/21/12 - [2.0.11] crmodels2 can now be compiled under mingw32.
 *   06/22/10 - [2.0.7] Corrected bug in the counting of the atoms
 *                      in the program.
 *   10/20/09 - [2.0.6] Improved handling of timeouts in crmodels2
 *                      and cr2.
 *   10/16/09 - [2.0.5] --cputime now a crmodels2 option
 *                      and not just cr2 option.
 *   10/07/09 - [2.0.3] Added --mkatoms and -a options.
 *   10/07/09 - [2.0.2] Added clasp-fe frontend. Default solver
 *                      changed from clasp to clasp-fe.
 *   09/18/09 - [2.0.0] First version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <string>
#include <vector>

#include "crmodels2.h"

#include "compile-flags.h"

#include "timed_computation.h"

using namespace std;

void show_usage(void)
{	printf("Usage:\n");
	printf("    crmodels [<options>] [<number of models>] <file1> [<file2> [...]]|--\n");
	printf("         Options:\n");
	printf("           --cputime <secs>: how many secs of CPUTIME is crmodels allowed to run\n");
	printf("           --grounder <path>: ASP grounder to be used (e.g. lparse or gringo)\n");
	printf("           --gopts \"<string>\": options to be passed to the grounder\n");
	printf("           --cr2opts \"<string>\": options to be passed to cr2\n");
	printf("           --solver <path>: underlying ASP solver to be used\n");
//	printf("           --sopts \"<string>\": options to be passed to the ASP solver\n");
//	printf("           any other option accepted by cr2 is also allowed\n");
	printf("           --cputime-aware-solver: solver accepts --cputime\n");
	printf("           --state-aware-solver: solver accepts --load-search-state and\n");
	printf("                                 --save-search-state\n");
	printf("           --mkatoms: format output using the mkatoms format\n");
	printf("           -a: (in conjunction with --mkatoms) format output using the mkatoms -a format\n");
	printf("           --non-exclusive: allows non-exclusive preferences\n");
	printf("              at most one of the following transitivity options can be used:\n");
	printf("       	   		--non-transitive: use non-transitive preferences\n");
	printf("       	   		--ip-transitive: use IP-transitive preferences\n");
	printf("       	   		--pi-transitive: use PI-transitive preferences\n");
	printf("    crmodels -h\n");
	printf("         prints this help\n");
}

void ensure_more_args(int i,int argc,const char *opt)
{	if (argc<=i)
	{	printf("***error: required argument missing for option %s\n\n",opt);
		show_usage();
		exit(1);
	}
}

int main(int argc,char *argv[])
{
	int i;
	vector<char *> files;
	const char *gopts="";
	const char *cr2opts="";
	const char *grounder=DEFAULT_GROUNDER;
	const char *solver=DEFAULT_SOLVER;
	char s[10240];
	
	int number_of_models;
	string cputime_limit;
	int cputime_limit_val;
	bool state_aware_solver;
	bool cputime_aware_solver;
	bool MKATOMS;
	bool AFLAG;
	bool non_exclusive;
	bool non_transitive;
	bool ip_transitive;
	bool pi_transitive;

	fprintf(stderr,"crmodels version "CRMODELS_VERSION"\n"); 

	if (argc==1)
	{	show_usage();
		exit(1);
	}

	if (strcmp(argv[1],"-h")==0)
	{	show_usage();
		exit(0);
	}

	MKATOMS=false;
	AFLAG=false;
	state_aware_solver=false;
	cputime_aware_solver=false;
	non_exclusive=false;
	non_transitive=false;
	ip_transitive=false;
	pi_transitive=false;
	number_of_models=1;
	cputime_limit="";
	cputime_limit_val=0;
	for(i=1;i<argc && argv[i][0]=='-' && strcmp(argv[i],"--")!=0;i++)
	{	if (strcmp(argv[i],"--grounder")==0)
		{	i++;
			ensure_more_args(i,argc,"--grounder");
			grounder=argv[i];
		}
		else
		if (strcmp(argv[i],"--gopts")==0)
		{	i++;
			ensure_more_args(i,argc,"--gopts");
			gopts=argv[i];
		}
		else
		if (strcmp(argv[i],"--solver")==0)
		{	i++;
			ensure_more_args(i,argc,"--solver");
			solver=argv[i];
		}
		else
		if (strcmp(argv[i],"--cputime")==0)
		{	i++;
			ensure_more_args(i,argc,"--cputime");
			cputime_limit=((string)"--cputime ") + argv[i];
			cputime_limit_val=atoi(argv[i]);
		}
		else
		if (strcmp(argv[i],"--cputime-aware-solver")==0)
		{	cputime_aware_solver=true;
		}
		else
		if (strcmp(argv[i],"--state-aware-solver")==0)
		{	state_aware_solver=true;
		}
		else
		if (strcmp(argv[i],"--cr2opts")==0)
		{	i++;
			ensure_more_args(i,argc,"--cr2opts");
			cr2opts=argv[i];
		}
		else
		if (strcmp(argv[i],"--mkatoms")==0)
			MKATOMS=true;
		else
		if (strcmp(argv[i],"-a")==0)
			AFLAG=true;
		else
		if (strcmp(argv[i],"--non-exclusive")==0)
			non_exclusive=true;
		else
		if (strcmp(argv[i],"--non-transitive")==0){
			if(ip_transitive||pi_transitive){
				printf("***error: --non-transitive, --ip-transitive, and --pi-transitive are exclusive options\n");
				show_usage();
				exit(1);
			}
			else
				non_transitive=true;
			}
		else
		if (strcmp(argv[i],"--ip-transitive")==0){
			if(non_transitive||pi_transitive){
				printf("***error: --non-transitive, --ip-transitive, and --pi-transitive are exclusive options\n");
				show_usage();
				exit(1);
			}
			else
				ip_transitive=true;
			}
		else
		if (strcmp(argv[i],"--pi-transitive")==0){
			if(non_transitive||ip_transitive){
				printf("***error: --non-transitive, --ip-transitive, and --pi-transitive are exclusive options\n");
				show_usage();
				exit(1);
			}
			else
				pi_transitive=true;
			}
		{	printf("***error: unknown option \'%s\'\n\n",argv[i]);
			show_usage();
			exit(1);
		}
	}

	if (i<argc && isdigit(argv[i][0]))
		number_of_models=atoi(argv[i++]);


	if (argc<=i)
	{	printf("***error: missing file name.\n");

		show_usage();
		exit(1);
	}

	for(;i<argc;i++)
		files.push_back(argv[i]);


#ifndef RELEASE
	fprintf(stderr,"***WARNING: using grounder %s.\n",grounder);
#endif


	if (cputime_limit_val>0)
		set_cputime_limit(cputime_limit_val);

	sprintf(s,"./hr %s %s %s %s ",
			(non_exclusive ? "--non-exclusive":""),
			(non_transitive ? "--non-transitive":""),
			(ip_transitive ? "--ip-transitive":""),
			(pi_transitive ? "--pi-transitive":""));
	for(i=0;i<(int)files.size();i++)
	{	sprintf(&s[strlen(s)],"%s ",files[i]);
	}
	sprintf(&s[strlen(s)],"| %s %s | ./cr2 --solver \"%s\" %s %s %s %s %s %s %s %d --",
		grounder,
		gopts,
		solver,
		cputime_limit.c_str(),
		(cputime_aware_solver ? "--cputime-aware-solver":""),
		(state_aware_solver ? "--state-aware-solver":""),
		(MKATOMS ? "--mkatoms":""),
		(AFLAG ? "-a":""),
		(non_exclusive ? "--non-exclusive":""),
		cr2opts,
		number_of_models);

#ifndef RELEASE
	fprintf(stderr,"Command: %s\n",s);
#endif

	exit(timed_system(s));
} 
