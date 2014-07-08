/*
 * cr2
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
 *   10/31/11 - [2.0.10] add_system_directive() must return a pointer
 *                       to the new last node in the program, or some
 *                       directives will be lost.
 *   06/28/10 - [2.0.8] Bugs corrected in the counting of the atoms in the
 *                      input programs.
 *   10/07/09 - [2.0.4] filterInactiveCRRules() modified to work properly
 *                      when gringo is used as grounder.
 *   10/07/09 - [2.0.3] Added --mkatoms and -a options.
 *   10/06/09 - [2.0.1] Checking of errors during external
 *                      program execution improved.
 *   09/18/09 - [2.0.0] First version.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>

#include <string>
#include <vector>

#include <unistd.h>

#include "compile-flags.h"

#include "crmodels2.h"

#include "keywords.h"
#include "debug.h"
#include "timed_computation.h"
#include "crlist.h"
#include "hash.h"
#include "extended_data.h"
#include "atom_store.h"
#include "model.h"
#include "loader.h"
#include "filter_crrules.h"
#include "generator.h"
#include "tester.h"

#include "cr2.h"
#include "stringutils.h"
#include "fileutils.h"

/*
 * IMPORTANT: all flags have been relocated to compile-flags.h
 *
 */



/* parameters -- FIND A BETTER PLACE!! */
int number_of_models;
int cputime_limit;
bool print_dominating_views;
bool iterate_dominating_views;
bool minimize_card_crrules;
bool use_pareto;
bool find_min_views;
bool cputime_aware_solver;
bool state_aware_solver;
const char *solver_state_opt="";
bool MKATOMS;
bool AFLAG;


atom_store store;
program *P;

generator g;
tester t;
model *M;

int curr_model_num;


char *fileHR=NULL;
char *fileGenerator=NULL;
char *fileTester=NULL;
char *fileS=NULL, *fileC=NULL, *fileF=NULL;
char *fileM=NULL, *fileDV=NULL;
char *fileTestResult=NULL;
char *fileSolverState=NULL;

char *ground_n_load_time=NULL;	/* ptr to string representing the time used for grounding and loading results */


char *solver_path;

void set_solver_path(const char *str)
{	solver_path=strdup(str);
}

void InitializeAllFiles()
{	fileHR=create_temp("hr.XXXXXX");
	fileGenerator=create_temp("gen.XXXXXX");
	fileTester=create_temp("tst.XXXXXX");
	fileTestResult=create_temp("tsr.XXXXXX");
	fileS=create_temp("s.XXXXXX");
	fileC=create_temp("c.XXXXXX");
	fileF=create_temp("f.XXXXXX");
	fileM=create_temp("m.XXXXXX");
	fileDV=create_temp("dv.XXXXXX");
	fileSolverState=create_temp("sst.XXXXXX");
}

void RemoveTempFiles(void)
{	delete_all_temps();
}

void cleantmp(void)
{	const char *tmpFiles[]=
	{	"hr.*",
		"gen.*",
		"tst.*",
		"tsr.*",
		"a.*", "s.*", "c.*", "f.*",
		"m.*", "dv.*",
		"sst.*",
		NULL
	};
	int i;
	char s[1024];
	char *tmpdir;

	tmpdir=get_tmp_dir();
	for(i=0;tmpFiles[i];i++)
	{	//sprintf(s,"rm -f %s/%s",tmpdir,tmpFiles[i]);
		//system(s);
		sprintf(s,"%s/%s",tmpdir,tmpFiles[i]);
		unlink(s);
	}
}

void file_cleanup(void)
{	delete_all_temps();
}

void handleCRDep(basic_list *deps,int atom,char **atom_names,bool *is_applcr,bool *added)
{	char *s;

	if (is_applcr[atom] && !added[atom])
	{	s=strdup(&atom_names[atom][11]);
		s[strlen(s)-1]=0;
		deps->addNode(s);

		added[atom]=true;
	}
}

void getCRDeps(basic_list *deps,char *a,int a_i,char **atom_names,bool *is_applcr,bool *visited,bool *added)
{	struct node *n;

#ifdef DEBUG_CRDEPS
	char s[10240];
#endif

	for(n=P->r->by_head[a_i]->first();n;n=n->next)
	{	int i;

		if ((!visited[RULEDATA(n)->index]) &&
		    (!P->r->crtrans_rules[RULEDATA(n)->index] || strcmp(a,"_false")!=0))
		{	visited[RULEDATA(n)->index]=true;
#ifdef DEBUG_CRDEPS
			sprintf(s,"[%d]%s::> ",RULEDATA(n)->index,a);
#endif

			for(i=0;i<RULEDATA(n)->n_pos;i++)
			{	
#ifdef DEBUG_CRDEPS
				sprintf(&s[strlen(s)],"%s%s{%d}%s",
					(i>0) ? ", ":"",
					atom_names[RULEDATA(n)->pos[i]],
					RULEDATA(n)->pos[i],
					(is_applcr[RULEDATA(n)->pos[i]])? "**":"");
#endif

				handleCRDep(deps,RULEDATA(n)->pos[i],atom_names,is_applcr,added);

				getCRDeps(deps,atom_names[RULEDATA(n)->pos[i]],RULEDATA(n)->pos[i],atom_names,is_applcr,visited,added);
			}

			for(i=0;i<RULEDATA(n)->n_neg;i++)
			{	
#ifdef DEBUG_CRDEPS
				sprintf(&s[strlen(s)],"%snot %s{%d}%s",
					(i>0 || RULEDATA(n)->n_pos>0) ? ", ":"",
					atom_names[RULEDATA(n)->neg[i]],
					RULEDATA(n)->neg[i],
					(is_applcr[RULEDATA(n)->neg[i]])? "**":"");
#endif

				handleCRDep(deps,RULEDATA(n)->neg[i],atom_names,is_applcr,added);

				getCRDeps(deps,atom_names[RULEDATA(n)->neg[i]],RULEDATA(n)->neg[i],atom_names,is_applcr,visited,added);
			}
#ifdef DEBUG_CRDEPS
			fprintf(stderr,"%s\n",s);
#endif
		}
	}
}

basic_list *getAllCRDeps(char *file)
{	FILE *fp;
	char s[10240];

	char **names;
	bool *is_applcr;
	bool *visited;
	bool *added;

	basic_list *deps;

	deps=new basic_list();

	names=(char **)calloc(P->a->atomcount,sizeof(char *));
	for(int i=0;i<P->a->atomcount;i++)
		names[i]=strdup("#noname#");
	for(struct node *n=P->a->allatomlist.first();n;n=n->next)
	{	free(names[BASICDATA(n)->value1]);
		names[BASICDATA(n)->value1]=BASICDATA(n)->value;
	}
	is_applcr=(bool *)calloc(P->a->atomcount,sizeof(bool *));
	for(struct node *n=P->a->applcrlist.first();n;n=n->next)
		is_applcr[BASICDATA(n)->value1]=true;
	visited=(bool *)calloc(P->r->rulecount,sizeof(bool *));
	added=(bool *)calloc(P->r->rulecount,sizeof(bool *));

	fp=open_read(file);
	while(!feof(fp))
	{	if (fgets(s,10240,fp)==NULL) break;

		for(int i=strlen(s)-1;(i>=0) && (s[i]=='\n' || s[i]=='\r');i--)
			s[i]=0;

		int a_i=P->a->allatomlist.find_atomnumber(s);
		if (a_i==0) continue;

		getCRDeps(deps,s,a_i,names,is_applcr,visited,added);
	}
	fclose(fp);

	/* add dependencies for false */
	getCRDeps(deps,s,1,names,is_applcr,visited,added);
	
	return(deps);
}

bool is_consistent(char *file)
{	FILE *fp;
	char s[10240];
	bool consistent;
   
	fp=open_read(file);

	consistent=true;
	if ((fgets(s,10240,fp)!=NULL) &&
	    (startsWith(s,"%*** ")))
		consistent=false;

	fclose(fp);

	return(consistent);
}

void AppendOtherGeneratorFiles()
{	FILE *fp;
   
	fp=open_append(fileGenerator);

	appendFile(fileS,fp);
	appendFile(fileC,fp);

	printTimestamp("AppendOtherGeneratorFiles: static part");
	P->output_grounding(fp,true);

	fclose(fp);
}

void AppendOtherTesterFiles()
{
	appendFile(fileHR,fileTester);		//PROBLEM!!!! choice rule {cr__stop} from hr.exe is in here!!!!!

/*
	FILE *fp;

	fp=open_append(fileTester);

	P->output_grounding(fp,true);

	fclose(fp);
*/
}

void AppendCardConstraint( char *fileName, int j)
{	FILE *fp;
	struct node *n;
	int applcrnum =0, k;

	fp=open_append(fileName);

	applcrnum=P->a->applcrlist.len();

	//fprintf(stderr,"THE NUMBER OF APPLCR ATOMS  %d\n",applcrnum);

	// rule ok1 :- lower {applcr ... applcr }
	fprintf(fp,"2 ");
	fprintf(fp,"%d ",store.get(store.OK1));
	fprintf(fp,"%d ",applcrnum);
	fprintf(fp,"0 ");
	fprintf(fp,"%d ",j);

	for(n=P->a->applcrlist.first();n!=NULL && BASICDATA(n)->value!=NULL;n=n->next)
		fprintf(fp,"%d ",BASICDATA(n)->value1);
      
	fprintf(fp,"\n");
      

	// for rule bad :- upper +1{applcr ...applcr}
	k = j+1;
	if(k <=  applcrnum)
	{	fprintf(fp,"2 ");
		fprintf(fp,"%d ",store.get(store.BAD));
		fprintf(fp,"%d ",applcrnum);
		fprintf(fp,"0 ");
		fprintf(fp,"%d ",k);
	
		for(n=P->a->applcrlist.first();n!= NULL && BASICDATA(n)->value!=NULL;n=n->next)
			fprintf(fp,"%d ",BASICDATA(n)->value1);
		fprintf(fp,"\n");
	}	

	// rule ok :- ok1, not bad
	fprintf(fp,"1 ");
	fprintf(fp,"%d ",store.get(store.OK));
	fprintf(fp,"2 ");
	fprintf(fp,"1 ");
	fprintf(fp,"%d ",store.get(store.BAD));
	fprintf(fp,"%d ",store.get(store.OK1));
	fprintf(fp,"\n");
	
	// rule :- not ok
	fprintf(fp,"1 ");
	fprintf(fp,"1 ");
	fprintf(fp,"1 ");
	fprintf(fp,"1 ");
	fprintf(fp,"%d ",store.get(store.OK));
	fprintf(fp,"\n");
	
	fclose(fp);
}    

void printTrueFalse(int status,int models_found)
{	if (MKATOMS)
	{	if (models_found==0)
			fprintf(stdout,"%s*** no models found.\n",
				(AFLAG ? "%":""));
	}
	else
	{	fprintf(stdout,"%s\n",(status) ? "True":"False");
		fprintf(stdout,"Duration: %s\n",get_total_cputime_string());
		fprintf(stdout,"Ground+Load Time: %s\n",ground_n_load_time);
	}
}

bool generatePhase(int i,int *activeCRRules,bool *filterCRStuff_done)
{	bool generator_inconsistent;
	char s[5120];

	printTimestamp("outer iteration %d",i);

	if (i==0)
	{	filterAllCRStuff();
		printTimestamp("filterAllCRStuff");
	}
	else
	{	
		if (!*filterCRStuff_done)
		{	
#ifdef RELEVANT_CRRULES		/* [marcy 062106] */
			filterCRStuffBUTForCRRules(active_crrules_list);
			printTimestamp("filterCRStuffBUTForCRRules");
#else
			filterNoCRStuff();
			printTimestamp("filterNoCRStuff");
#endif

			*filterCRStuff_done=true;
		}
	}

	if (i==0)
		g.output(i,P->a->removed_atoms); 	/* at the first iteration, the number of usable cr-rules is 0 anyway */
	else
		*activeCRRules=g.output(i,P->a->removed_atoms); 	/* CreateGenerator() returns the updated number of usable cr-rules */

	printTimestamp("CreateGenerator");

	AppendOtherGeneratorFiles();
	printTimestamp("AppendGeneratorFiles");

#ifdef RELEVANT_CRRULES		/* [marcy 041906] */
	if (i>0)
	{	sprintf(s,"%s %s %s < %s",solver_path,solver_cputime_opt(cputime_aware_solver),solver_state_opt,fileGenerator);
		timed_mkatoms_system_err(s,fileM);
	}
	else
	{	sprintf(s,"%s %s %s -printconflicts < %s",solver_path,solver_cputime_opt(cputime_aware_solver),solver_state_opt,fileGenerator);
		timed_mkatoms_system_err(s,fileM);

/*		sprintf(s,"%s %s %s -printconflicts < %s > %s.2",solver_path,solver_cputime_opt(cputime_aware_solver),solver_state_opt,fileGenerator,fileGenerator);
		if (timed_system(s) != 0) // error
		{	char f[10240];

			fprintf(stderr,"***Error while executing %s. Command output:\n",s);
			sprintf(f,"%s.2",fileGenerator);
			catFile(f,stderr);
			exit(1);
		}
		sprintf(s,"mkatoms -a < %s.2 > %s",fileGenerator,fileM);
		if (timed_system(s) != 0) // error
		{	fprintf(stderr,"***Error while executing %s. Command output:\n",s);
			catFile(fileM,stderr);
			exit(1);
		}
*/
	}
#else
	sprintf(s,"%s %s %s < %s",solver_path,solver_cputime_opt(cputime_aware_solver),solver_state_opt,fileGenerator);
	timed_mkatoms_system_err(s,fileM);
#endif

	generator_inconsistent = !is_consistent(fileM);	/* res=true -> last call to the solver returned INCONSISTENCY */

	return(generator_inconsistent);
}

void Add_MtoS(void)
{	FILE *fp;
  
	struct node *n;
  
	int total,array_sz,pos;

	int *in_aset;	/* for each atom in P->a->allatomlist, 1 means atoms is in answer set, -1 means it is not, 0 means it is an auxiliary atom and should be skipped */
	int i;	/* used to scan in_aset */


	fp=open_append(fileS);

	//fprintf(stderr,"msize is = %d \n",M->mmlist.len());

	/* we use get_max_atomindex() instead of len() because
	 * later we will be accessing the corresponding array by
	 * atom index, and atom indexes are 1- and sometimes even
	 * 2-based.
	 */
	array_sz = P->a->allatomlist.get_max_atomindex();

        /* Marcy 09/08/04: fill in_aset */
	total=0;
	in_aset=(int *)calloc(array_sz,sizeof(int));
	for(n=P->a->allatomlist.first();n!=NULL  && BASICDATA(n)->value!=NULL;n=n->next)
	{	if (!atom_utils::is_hidden_atom(BASICDATA(n)->value))
		{	in_aset[BASICDATA(n)->value1-1]=-1;
			total++;
		}
	}

	pos=0;
	for(n=M->mlist.first();n!=NULL && BASICDATA(n)->value!=NULL;n=n->next)
	{	int j;

		if (atom_utils::is_hidden_atom(BASICDATA(n)->value))
			continue;

		j=P->a->allatomlist.find_atomnumber(BASICDATA(n)->value);
		if (j!=0)
		{	in_aset[j-1]=1; /* NOTE: array is 0-based */
			pos++;
		}
	}

	fprintf(fp,"1 ");
	fprintf(fp,"1 ");
	fprintf(fp,"%d ",total);
	fprintf(fp,"%d ",total-pos);

	/* Marcy 09/08/04: output negative part of the body */
	for(i=0;i<array_sz;i++)
	{	if (in_aset[i]==-1)
			fprintf(fp,"%d ",i+1);
	}

	/* Marcy 09/08/04: output positive part of the body */
	for(i=0;i<array_sz;i++)
	{	if (in_aset[i]==1)
			fprintf(fp,"%d ",i+1);
	}

	fprintf(fp,"\n");   	 	
  
	free(in_aset);
 
 /* fprintf(stderr,"printing mmlist\n"); 
 fprintf(stderr,"Printing the model\n");
 mmlist.print(stderr);
 fprintf(stderr,"End of Printing the model\n");
 
 fprintf(stderr,"printing atlist\n"); 
 P->a->allatomlist.print();
 
 fprintf(stderr,"file SS = %s\n",fileS); 
 exit(0); */
 
	fclose(fp); 
}

void AddMto_A()
{	if (MKATOMS)
		M->output_mkatoms(AFLAG);
	else
	{	fprintf(stdout,"Answer: %d\n",curr_model_num); 
		fprintf(stdout,"Stable Model: ");

		M->output_in_line();

		fprintf(stdout,"\n");
	}

	curr_model_num++;
}

void AddMto_F(void)
{	FILE *fp; 
	struct node *n;

	fp=open_append(fileF);

	if(M->appllist.len()==0)
	{	// manual grounding of rule true.
		fprintf(fp,"1 ");
		fprintf(fp,"%d ",store.get(store.TRUE));
		fprintf(fp,"0 ");
		fprintf(fp,"0 ");
		fprintf(fp,"\n ");
       
		// manual grounding of rule :-true.
		fprintf(fp,"1 ");
		fprintf(fp,"1 ");
		fprintf(fp,"1 ");
		fprintf(fp,"0 ");
		fprintf(fp,"%d ",store.get(store.TRUE));
		fprintf(fp,"\n ");
	}
	else 
	{	//for manual grounding
		fprintf(fp,"1 ");
		fprintf(fp,"1 ");
		fprintf(fp,"%d ",M->appllist.len());
		fprintf(fp,"0 ");

		for(n=M->appllist.first();
		    n!=NULL && BASICDATA(n)->value!= NULL;
		    n=n->next)
		{	int idx;

			idx=P->a->allatomlist.find_atomnumber(BASICDATA(n)->value);
			if (idx!=0)
				fprintf(fp,"%d ",idx);
		}  
		fprintf(fp,"\n");
	}

	fclose(fp);
}

void AddConstrMto_C()
{	appendFile(fileF,fileC);
}

void CreateM_fromFile(char *fname)
{	FILE *fp;
	char s[10240];

	fp=open_read(fname);

	if (M!=NULL)
	{	delete(M);
		M=NULL;
	}

	M=new model();
   
	while(fgets(s,512,fp) != NULL) 
	{	while((s[strlen(s)-1] == '\n') || (s[strlen(s)-1] == '\r') ||(s[strlen(s)-1] == '.'))
		s[strlen(s)-1]=0;

//		s=strstr(s2, "cr__applcr(");
//		if(s != NULL)
//			M->appllist.addNode(s);
		if (startsWith(s, APPLCR"("))
			M->appllist.addNode(s);

//		s=strstr(s2,"%%endmodel");
//		if(s == NULL)
		if (strcmp(s,"%%endmodel")!=0)
		{	M->mlist.addNode(s);
//			if (atom_utils::not_cratom(s2))
//				M->mmlist.addNode(s2);
		}
	}

//	fprintf(stderr,"Printing the model\n");
//	mlist.print(stderr);
//	fprintf(stderr,"End of Printing the model\n");

	fclose(fp);

	printTimestamp("CreateM");
} 

void CreateM()
{	CreateM_fromFile(fileM);
}

int countAppliedCRRules_fromFile(char *fname)
{	char s[FILECHUNK];
	int num,read;
	FILE *fp;

	fp=open_read(fname);
	num=0;
	while(!feof(fp) && ((read=fread(s,sizeof(char),FILECHUNK,fp))>0))
	{	char *ptr;

		/* count the number of occurrences of APPLCR */
		for(ptr=strstr(s,APPLCR);ptr < &s[read] && ptr!=NULL;ptr=strstr(ptr+1,APPLCR),num++);
	}
	fclose(fp);

	return(num);
}

/*
 * version that uses external **UNIX** programs
int countAppliedCRRules_fromFile(char *fname)
{	char s[5120],*tfile;
	int num;
	FILE *fp;

	tfile=create_temp("wcl.XXXXXX");

	sprintf(s,"egrep '"APPLCR"' %s | wc -l > %s",fname,tfile);
	if (timed_system(s) != 0) // error
	{	fprintf(stderr,"error invoking %s\n",s);
		exit(1);
	}

	printTimestamp("countCRRulesApplied");

	//sprintf(s,"%s.wc-l",fname);

	fp=open_read(tfile);
   
	if (fgets(s,5120,fp)==NULL)
	{	fprintf(stderr,"Unable to read from \"%s\".\n",tfile);
		exit(1);
	}

	num=atoi(s);

	fclose(fp);

	delete_temp(tfile);
	free(tfile);

	return(num);
} 
*/

int countAppliedCRRules()
{	return(countAppliedCRRules_fromFile(fileM));
}

void recordView(void)
{	if (!state_aware_solver)
		Add_MtoS();
	printTimestamp("Add_MtoS");
}

void storeAndPrintAnswerSet(int *models_found)
{	AddMto_A();
	(*models_found)++;
	printTimestamp("MODELS FOUND: %d",*models_found);
}

bool testPhase(int i)
{	bool testedOK;
	char s[5120];

	/* Marcy 09/08/04
	**
	** Running the tester is unnecessary
	** if i==0.
	*/
	if (i>0 && !find_min_views && (P->a->ispreferredlist.len()>0 || P->a->ispreferred2list.len()>0))
	{	t.output(M);
		printTimestamp("CreateTester");
		AppendOtherTesterFiles();
		printTimestamp("AppendOtherTesterFiles");
    
		sprintf(s,"%s %s < %s",solver_path,solver_cputime_opt(cputime_aware_solver),fileTester); 
		timed_mkatoms_system_err(s,fileTestResult);

		printTimestamp("nth tester solver");


		// system(test);
		// fprintf(stderr,"fileTestResult = %s\n",fileTestResult);
		// if(tm > 1)
		//exit(0);
    
		testedOK = !(is_consistent(fileTestResult));
		printTimestamp("CheckifConsistent");
	}
	else
		testedOK=true;

	return(testedOK);
}

void Print_TheDominatingView(char *file)
{	FILE *fp;
	char s2[512];
	int i;

	fp=open_read(file);

	while(fgets(s2,512,fp) != NULL) 
	{	if (s2[0]=='%') continue;

		i=strlen(s2);
		while((s2[i-1] == '\n') ||
		      (s2[i-1] == '\r') ||
		      (s2[i-1] == '.') ||
		      (s2[i-1] == ','))
			i--;
		s2[i]='\0';

		if (!atom_utils::is_hidden_atom(s2))
			printf("%s ",s2);
	} 
	printf("\n"); 

	fclose(fp);
}

void iterateDominatingViews(int i,int *models_found)
{	char s[5120];
	int j;
	bool testedOK;

	/* experimental iteration over dominating view -- see final report for USA (01/31/06) */
	do
	{	for(j=i;;j++)
		{	t.output(M);
			AppendCardConstraint(fileTester,j);
			AppendOtherTesterFiles();
    
			sprintf(s,"%s %s < %s",solver_path,solver_cputime_opt(cputime_aware_solver),fileTester);
			timed_mkatoms_system_err(s,fileDV);
			// system(test);
			// fprintf(stderr,"fileDV = %s\n",fileDV);
			// if(tm > 1)
			//exit(0);
    
			if (is_consistent(fileDV))
			{	/* we found a MODEL!!! (unless we have circular dependencies on preferences) */
				fprintf(stdout,"\nI think this is a model: ");
				Print_TheDominatingView(fileDV);

#if 0
				(*models_found)++;
				printTrueFalse(1,*models_found);	// Print "True"
				return;
#endif
				break;
			}
		}

		printf("sub-iterating\n");
		CreateM_fromFile(fileDV);

		t.output(M);
		AppendOtherTesterFiles();
    
		sprintf(s,"%s %s < %s",solver_path,solver_cputime_opt(cputime_aware_solver),fileTester);
		timed_mkatoms_system_err(s,fileDV);
		// system(test);
		// fprintf(stderr,"fileDV = %s\n",fileDV);
		// if(tm > 1)
		//exit(0);
		testedOK=!(is_consistent(fileDV));
		if (!testedOK)
		{	fprintf(stdout,"\n       dominated by: ");
			Print_TheDominatingView(fileDV);
		}
	} while(!testedOK);
	(*models_found)++;
	printTrueFalse(1,*models_found);	// Print "True"
}

#ifdef RELEVANT_CRRULES
void handle_smodels_conflicts(int i,bool smodels_inconsistent,int *activeCRRules)
{	char s[5120];

	if (i==0)
	{	if (smodels_inconsistent)
		{	sprintf(s,"grep '"CONFLICT_STRING"' %s.2 | sed -e 's/"CONFLICT_STRING"//' > %s.3",fileGenerator,fileGenerator);
			if (timed_system(s) != 0) // error
			{	fprintf(stderr,"error invoking %s\n",s);
				exit(1);
			}

			printTimestamp("grep & sed");

			sprintf(s,"%s.3",fileGenerator);
			//outputAllCRDeps(s);
			active_crrules_list=getAllCRDeps(s);
			//active_crrules_list.print();
			*activeCRRules=active_crrules_list.len();

			printTimestamp("getAllCRDeps");

			sprintf(s,"%s.2",fileGenerator);
			unlink(s);
			sprintf(s,"%s.3",fileGenerator);
			unlink(s);
		}
		else
		{	sprintf(s,"%s.2",fileGenerator);
			unlink(s);
		}
	}
}
#endif /* RELEVANT_CRRULES */


void InitFiles()
{	FILE *fp;
	char *files[]=
	{	fileS, fileF, NULL
	};

	for(int i=0;files[i]!=NULL;i++)
	{	fp=open_write(files[i]);
		fclose(fp);
	}
}


void InitializeC()
{	FILE *fp;

	fp=open_write(fileC);
	fclose(fp);
}


int doBinarySearch(int start,int *end,bool *filterCRStuff_done,bool *solver_output_stored)
{	int i,n;
	bool generator_inconsistent;

	int intval_beg,intval_end;
	char fileMBackup[10240];

	n=*end;
 
	printTimestamp("in doBinarySearch(%d,%d)",start,n);

	g.skipLowerBound=true;	/* set CreateGenerator() to look for solutions w/ AT MOST i cr-rules */

	sprintf(fileMBackup,"%s.backup",fileM);
	*solver_output_stored=false;

	intval_beg=start;
	intval_end=n;
	while(intval_beg<intval_end)
	{
		i=(intval_beg+intval_end)/2;

		InitFiles(); // S:= { }, F :={ }
		printTimestamp("InitFiles");

		generator_inconsistent=generatePhase(i,&n,filterCRStuff_done); /* res=true -> last call to the solver returned INCONSISTENCY */
#ifdef DEBUG_BINARY_SEARCH
		printTimestamp("binary search solver: at most %d cr-rules", i);
#endif

		if (!generator_inconsistent)
		{	/* go down the left branch */

			int applied_crrules;

#ifdef DEBUG_BINARY_SEARCH
			fprintf(stderr,"<%d,%d> succeeded: going down the LEFT branch\n",intval_beg,i);
#endif

			applied_crrules=countAppliedCRRules();
#ifdef DEBUG_BINARY_SEARCH
			fprintf(stderr,"applied cr-rules: %d\n",applied_crrules);
#endif

			*solver_output_stored=true;
			unlink(fileMBackup);
			rename(fileM,fileMBackup);
			
			/* Instead of iterating with the conventional interval:
			 *
			 *    <intval_beg, (intval_beg+intval_end)/2>
			 *
			 * we can take into account applied_crrules.
			 * Since we *know* that a solution exists for applied_crrules
			 * cr-rules, we can iterate with the interval
			 *
			 *    <intval_beg, applied_crrules>
			 *
			 * Notice that, by definition of the CreateGenerator()
			 * (with CreateGenerator_skipLowerBound=true)
			 *
			 *    applied_crrules <= (intval_beg+intval_end)/2.
			 *
			 */
			 //intval_end=i-1;	/* conventional method */
			 intval_end=applied_crrules;	/* WARNING
			 				 *
							 * We DON'T use "-1" (as per conventional
							 * method) because this allows us to
							 * ensure that intval_end corresponds to
							 * the cached answer set. Hence, when
							 * the loop terminates, we are sure that
							 * the answer set for the computed value
							 * of i has already been cached.
			 				 *
							 */
		}
		else
		{	/* go down the right branch */
#ifdef DEBUG_BINARY_SEARCH
			fprintf(stderr,"<%d,%d> failed: going down the RIGHT branch\n",intval_beg,i);
#endif

			//last_test_succeeded=false;
			intval_beg=i+1;
		}
	}

#ifdef DEBUG_BINARY_SEARCH
	fprintf(stderr,"********************We start iterating from %d\n",intval_beg);
#endif

	if (*solver_output_stored)
	{	unlink(fileM);
		rename(fileMBackup,fileM);
	}

	g.skipLowerBound=false;	/* set CreateGenerator() to look for solutions w/ EXACTLY i cr-rules */

	*end=n;
	return(intval_beg);
}


void MainAlgm(int n)
{	int i, tm, models_found;
	bool generator_inconsistent,testedOK;
 	bool solver_output_stored;
	bool filterCRStuff_done;

	printTimestamp("in MainAlgm");

	curr_model_num=1;

	InitializeC(); // C:= { }
	printTimestamp("InitializeC");

	models_found = 0;
	filterCRStuff_done=false;
	tm=0;

	g.skipLowerBound=false;	/* set CreateGenerator() to look for solutions w/ EXACTLY i cr-rules */


#ifdef USE_BINARYSEARCH_STARTUP
/* --------------- binary search ----------- */

	/* first we figure out which cr-rules are active */
	i=0;
	InitFiles(); // S:= { }, F :={ }
	printTimestamp("InitFiles");
	do
	{	generator_inconsistent=generatePhase(i,&n,&filterCRStuff_done); /* res=true -> last call to the solver returned INCONSISTENCY */
		printTimestamp("1st solver");
#ifdef RELEVANT_CRRULES
		handle_smodels_conflicts(i,generator_inconsistent,&n);
#endif /* RELEVANT_CRRULES */
		if (!generator_inconsistent)
		{	/* testing is unnecessary */
			CreateM(); 

			storeAndPrintAnswerSet(&models_found);

			if (models_found >= number_of_models &&
			    number_of_models != 0)	/* we found all we had to find */
			{	printTrueFalse(1,models_found);	// Print "True"
				return;
			}
			recordView();
			AddMto_F();

		}
	} while (!generator_inconsistent);  //end of inner while loop
	AddConstrMto_C();
	/* --- active cr-rules found --- */

#  ifdef USE_CONTINUOUS_BINARYSEARCH
	i=1;
	solver_output_stored=false;
#  else

	i=doBinarySearch(1,&n,&filterCRStuff_done,&solver_output_stored);
#  endif /* USE_CONTINUOUS_BINARYSEARCH */

/* ----------------------------------------- */
#else /* USE_BINARYSEARCH_STARTUP */
	i=1;
	solver_output_stored=false;
#endif /* USE_BINARYSEARCH_STARTUP */


 
	for (;i<=n;i++)
	{	

#ifdef USE_CONTINUOUS_BINARYSEARCH
		i=doBinarySearch(i,&n,&filterCRStuff_done,&solver_output_stored);
#endif /* USE_CONTINUOUS_BINARYSEARCH */

#ifdef USE_BINARYSEARCH_STARTUP
		if (!solver_output_stored)
		{
#endif /* USE_BINARYSEARCH_STARTUP */
			InitFiles(); // S:= { }, F :={ }
			printTimestamp("InitFiles");
#ifdef USE_BINARYSEARCH_STARTUP
		}
#endif /* USE_BINARYSEARCH_STARTUP */

		do
		{	tm++;

#ifdef USE_BINARYSEARCH_STARTUP
			if (!solver_output_stored)
			{
#endif /* USE_BINARYSEARCH_STARTUP */
				generator_inconsistent=generatePhase(i,&n,&filterCRStuff_done); /* res=true -> last call to the solver returned INCONSISTENCY */
				printTimestamp((i==0) ? "1st solver":"nth generation solver");
#ifdef USE_BINARYSEARCH_STARTUP
			}
			else
			{	generator_inconsistent=false;	/* generation was successful during */
								/* binary search */

				solver_output_stored=false;	/* we can re-use the result of binary */
								/* search only the first time */
								/* we get here */
			}
#endif /* USE_BINARYSEARCH_STARTUP */

#ifdef RELEVANT_CRRULES
			handle_smodels_conflicts(i,generator_inconsistent,&n);
#endif /* RELEVANT_CRRULES */

			if (!generator_inconsistent)
			{	CreateM();

				testedOK=testPhase(i);

				if (testedOK)
				{	storeAndPrintAnswerSet(&models_found);

					if (models_found >= number_of_models &&
					    number_of_models != 0)	/* we found all we had to find */
					{	printTrueFalse(1,models_found);	// Print "True"
						return;
					}
					AddMto_F();
				}
				else if (print_dominating_views)
				{	fprintf(stdout,  "     rejected model: ");
					//AddMto_A();
					M->output_in_line();
					fprintf(stdout,"\n       dominated by: ");
					Print_TheDominatingView(fileDV);

					/* experimental iteration over dominating view -- see final report for USA (01/31/06) */
					if (iterate_dominating_views)
					{	iterateDominatingViews(i,&models_found);
						return;
					}
				}
				recordView();
			}
		} while (!generator_inconsistent);  //end of inner while loop
    
		if ((models_found > 0) && (minimize_card_crrules))
		{	printTrueFalse(0,models_found);		// Print "True"
			return;
		}
		AddConstrMto_C();

	} //end of outer for

	printTrueFalse(0,models_found);			// Print "False"

} //end of Algm

void sig_termination_handler(int signum)
{	fprintf(stderr,"*** Aborted\n");
	exit(1);
}

void setup_termination_traps(void)
{
#ifndef _WIN32
	struct sigaction new_action, old_action;
#endif

	/* Set up atexit() trap so that we can remove
	 * the temp files when the program exits with exit().
	 */
	atexit(file_cleanup);


	/* Now let's take care of termination signals. */     

#ifndef _WIN32
	/* Set up the structure to specify the new action. */
	new_action.sa_handler = sig_termination_handler;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;
     
	sigaction (SIGINT, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction (SIGINT, &new_action, NULL);
	sigaction (SIGHUP, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction (SIGHUP, &new_action, NULL);
	sigaction (SIGTERM, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction (SIGTERM, &new_action, NULL);
#endif
}

void show_usage(void)
{	printf("Usage:\n");
	printf("    cr2 [<options>] [<number of models>] <file.hr>|--\n");
	printf("         processes file file.hr or console input (if '--' specified)\n");
	printf("         Options:\n");
	printf("           --cputime <secs>: how many secs of CPUTIME is crmodels allowed to run\n");
	printf("           --solver <solver path>: underlying ASP solver to be used\n");
	printf("           --min-card: minimize the cardinality of cr-rules\n");
	printf("           --min-views: compute set theoritically minimal views\n");
	printf("           --print-dominating-views: prints rejected models and dominating views\n");
	printf("           --iterate-dominating-views: re-use dominating views in the search\n");
	printf("                                       for models (EXPERIMENTAL)\n");
	printf("           --pareto: use Pareto preference instead of binding preference\n");
//	printf("           --cleantmp: deletes all the crmodels-related files in /tmp and exit.\n");
	printf("           -m <number of models>: number of models to output\n");
	printf("                                  (retained for backward compatibility)\n");
	printf("           --cputime-aware-solver: solver accepts --cputime\n");
	printf("           --state-aware-solver: solver accepts --load-search-state and\n");
	printf("                                 --save-search-state\n");
	printf("           --mkatoms: format output using the mkatoms format\n");
	printf("           -a: (in conjunction with --mkatoms) format output using the mkatoms -a format\n");
	printf("    cr2 -h\n");
	printf("         prints this help\n");
}

void ensure_more_args(int i,int argc,const char *opt)
{	if (argc<=i)
	{	printf("***error: required argument missing for option %s\n\n",opt);
		show_usage();
		exit(1);
	}
}

/*
 * Intended use:
 *
 *   Given a CR-Prolog program p1.cr:
 *    1. make_hr p1.cr | gringo-or-lparse <opts> > p1.hr
 *       where typically <opts> is --true-negation -d all
 *    2. cr2 p1.hr
 *
 */
int main(int argc,char *argv[])
{
	int num,i;
	char *file;

	const char *slv=DEFAULT_SOLVER;


	fprintf(stderr,"cr2/crmodels version "CRMODELS_VERSION"\n"); 

	if (argc==1)
	{	show_usage();
		exit(1);
	}

	if (strcmp(argv[1],"-h")==0)
	{	show_usage();
		exit(0);
	}

	/* default parameter values */
	number_of_models=1;
	print_dominating_views=false;
	iterate_dominating_views=false;
	minimize_card_crrules=false;
	use_pareto=false;
	find_min_views=false;
	cputime_aware_solver=false;
	state_aware_solver=false;
	MKATOMS=false;
	AFLAG=false;
	cputime_limit=0;
	for(i=1;i<argc && argv[i][0]=='-' && strcmp(argv[i],"--")!=0;i++)
	{	if (strcmp(argv[i],"--solver")==0)
		{	i++;
			ensure_more_args(i,argc,"--solver");
			slv=argv[i];
		}
		else
		if (strcmp(argv[i],"--min-card")==0)
			minimize_card_crrules=true;
		else
		if (strcmp(argv[i],"--min-views")==0)
			find_min_views=true;
		else
		if (strcmp(argv[i],"--print-dominating-views")==0)
			print_dominating_views=true;
		else
		if (strcmp(argv[i],"--iterate-dominating-views")==0)
			iterate_dominating_views=true;
		else
		if (strcmp(argv[i],"--pareto")==0)
			use_pareto=true;
		else
		if (strcmp(argv[i],"--cputime")==0)
		{	i++;
			ensure_more_args(i,argc,"--cputime");
			cputime_limit=atoi(argv[i]);
		}
		else
		if (strcmp(argv[i],"-m")==0)
		{	i++;
			ensure_more_args(i,argc,"-m");
			number_of_models=atoi(argv[i]);
		}
		else
		if (strcmp(argv[i],"--cputime-aware-solver")==0)
			cputime_aware_solver=true;
		else
		if (strcmp(argv[i],"--state-aware-solver")==0)
			state_aware_solver=true;
		else
		if (strcmp(argv[i],"--mkatoms")==0)
			MKATOMS=true;
		else
		if (strcmp(argv[i],"-a")==0)
			AFLAG=true;
		else
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

	file=argv[i++];

	if (argc>i)
	{	printf("***error: extra parameters after file name.\n\n");

		show_usage();
		exit(1);
	}

#ifndef RELEASE
	fprintf(stderr,"***WARNING: using solver %s.\n",slv);
#endif
	set_solver_path(slv);

	if (cputime_limit>0)
		set_cputime_limit(cputime_limit);


	setup_termination_traps();

	InitializeAllFiles();
	//  fprintf(stderr,"After InitializeAllFiles()\n");

	{	//char s[10240];

		unlink(fileHR);
		if (strcmp(file,"--")==0)
		//	sprintf(s,"cat >%s",fileHR);
			appendFile(stdin,fileHR);
		else
		//	sprintf(s,"cp %s %s",file,fileHR);
			appendFile(file,fileHR);
		//system(s);
	}
	//computeHR();  // now computed externally
	//  fprintf(stderr,"After computeHR()\n");

	if (state_aware_solver)
	{	char s[10240];

		sprintf(s,"--load-search-state %s --save-search-state %s",fileSolverState,fileSolverState);
		solver_state_opt=strdup(s);
	}

	P=new program();

	loader l;
	l.store=&store;
	num = l.load_program(fileHR,P);
	//  fprintf(stderr,"After load_program(fileHR)\n");

	ground_n_load_time=strdup(get_total_cputime_string());

	/* initialize the generator */
	g.file=fileGenerator;
	g.applcrlist=&P->a->applcrlist;
	g.allatomlist=&P->a->allatomlist;
	g.store=&store;

	/* initialize the tester */
	t.file=fileTester;
	t.appllist=&P->a->appllist;
	t.ispreferred2list=&P->a->ispreferred2list;
	t.applcrlist=&P->a->applcrlist;
	t.ispreferredlist=&P->a->ispreferredlist;
	t.store=&store;
	t.use_pareto=use_pareto;

	M=NULL;

	MainAlgm(num);
	//  fprintf(stderr,"After MainAlgm(num)\n");

	RemoveTempFiles();
} 
