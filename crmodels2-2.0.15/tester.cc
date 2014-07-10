#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compile-flags.h"

#include "tester.h"

#include "keywords.h"
#include "fileutils.h"
#include "stringutils.h"


#ifdef DO_SETPREFS

void tester::store_match(char *s,char *orig,regmatch_t *m)
{	if (m->rm_so!=-1)
	{	memcpy(s,&orig[m->rm_so],m->rm_eo-m->rm_so);
		s[m->rm_eo-m->rm_so]=0;
	}
	else
		s[0]=0;
}

void tester::doSetPrefs(FILE *fp,model *M,int *tester_atomcount)	/* marcy [062106] */
{	struct node *n;
	basic_list m_ispreferred2list;
	regex_t r;
	regmatch_t matches[3];

	for (n=M->mlist.first();n != NULL && BASICDATA(n)->value != NULL;n=n->next)
	{	if (startsWith(BASICDATA(n)->value,IS_PREF2))
		{	/*
			s = (char *)calloc(strlen(BASICDATA(n)->value) +4, sizeof(char));
			sprintf(s,"o_%s.", BASICDATA(n)->value);
       
			fprintf(fp,"1 ");
			fprintf(fp,"%d ",*tester_atomcount);
			fprintf(fp,"0 ");
			fprintf(fp,"0 ");
			fprintf(fp,"\n");
       
			addNodeWithInfo(s,*tester_atomcount,o_prefer2list);
			free(s);
			(*tester_atomcount)++;
			addNodeWithInfo(s,*tester_atomcount,o_prefer2list);
			*/
			m_ispreferred2list.addNode(BASICDATA(n));
		}
	}

	fprintf(stderr,"m_ispreferred2list:\n");
	fprintf(stderr,"----\n");
	m_ispreferred2list.print_pairs(stderr);
	fprintf(stderr,"----\n");

	for (n=m_ispreferred2list.first();n != NULL && BASICDATA(n)->value != NULL;n=n->next)
	{	char s1[1024],s2[1024];
		char rel1[1024],rel2[1024];
		int ret;
		struct node *n2;
		
		int k1,k2;
		int rel1_applcr,rel2_applcr;
		int is_preferred2idx;

		is_preferred2idx=ispreferred2list->find_atomnumber(BASICDATA(n)->value);
fprintf(stderr,"is_preferred2idx=%d\n",is_preferred2idx);
		if (is_preferred2idx==0) continue;

		if ((ret=regcomp(&r,IS_PREF2"\\((.*),(.*)\\)",REG_EXTENDED)))
		{	fprintf(stderr,"doSetPrefs: regcomp: ERROR\n");
			regerror(ret,&r,s1,1024);
			fprintf(stderr,"%s\n",s1);
			exit(1);
		}
		
		if ((ret=regexec(&r,BASICDATA(n)->value,3,matches,0)))
		{	fprintf(stderr,"doSetPrefs: regexec: ERROR\n");
			regerror(ret,&r,s1,1024);
			fprintf(stderr,"%s\n",s1);
			exit(1);
		}

		for(int i=0;i<3;i++)
		{	store_match(s1,BASICDATA(n)->value,&matches[i]);
			fprintf(stderr,"match %d: %s\n",i,s1);
		}
		store_match(rel1,BASICDATA(n)->value,&matches[1]);
		store_match(rel2,BASICDATA(n)->value,&matches[2]);

		sprintf(s1,APPLCR"(%s",rel1);	// TODO: take care of ending, i.e. '(' or ')'
		sprintf(s2,APPLCR"(%s",rel2);	// TODO: take care of ending, i.e. '(' or ')'

		k1=0; k2=0;
		for (n2=M->mlist.first();n2 != NULL && BASICDATA(n2)->value != NULL;n2=n2->next)
		{	if (startsWith(BASICDATA(n2)->value,s1))
{ fprintf(stderr,"%s: %s\n",s1,BASICDATA(n2)->value);
				k1++;
}
			else
			if (startsWith(BASICDATA(n2)->value,s2))
{ fprintf(stderr,"%s: %s\n",s2,BASICDATA(n2)->value);
				k2++;
}
		}
fprintf(stderr,"k1: %d k2: %d\n",k1,k2);

		rel1_applcr=0; rel2_applcr=0;
		for (n2=applcrlist->first();n2 != NULL;n2=n2->next)
		{	if (startsWith(BASICDATA(n2)->value,s1))
				rel1_applcr++;
			else
			if (startsWith(BASICDATA(n2)->value,s2))
				rel2_applcr++;
		}
fprintf(stderr,"rel1_applcr: %d rel2_applcr: %d\n",rel1_applcr,rel2_applcr);
//fp=stderr;

		/* c1_xxx :- (k1+1){applcr(rel1(X1,...))}. */
		if (rel1_applcr >= k1+1)
		{	fprintf(fp,"2 %d %d 0 %d ",*tester_atomcount,rel1_applcr,k1+1);
			for (n2=applcrlist->first();n2 != NULL;n2=n2->next)
			{	if (startsWith(BASICDATA(n2)->value,s1))
					fprintf(fp,"%d ",BASICDATA(n2)->value1);
			}
			fprintf(fp,"\n");
		}

		/* c2_xxx :- {applcr(rel1(X1,...))}(k2-1). */
		if (rel2_applcr >= rel2_applcr-k2+1)
		{	fprintf(fp,"2 %d %d %d %d ",*tester_atomcount+1,rel2_applcr,rel2_applcr,rel2_applcr-k2+1);
			for (n2=applcrlist->first();n2 != NULL;n2=n2->next)
			{	if (startsWith(BASICDATA(n2)->value,s2))
					fprintf(fp,"%d ",BASICDATA(n2)->value1);
			}
			fprintf(fp,"\n");
		}

		/* dominates :- c1_xxx, c2_xxx, cr__is_preferred2(rel1,rel2). */
		fprintf(fp,"1 %d 3 0 %d %d %d\n",store->get(store->DOMINATE),*tester_atomcount,*tester_atomcount+1,is_preferred2idx);
		*tester_atomcount+=2;

		regfree(&r);
		
	}

}
#endif


int tester::look_for_j(char *s1, char *s2)
{	int i,j;

	i = strlen(s1)-1;
	j = strlen(s2)-1;

	/* ignore the ending dots, if present */
	if (s1[i]=='.') i--;	// s1 may be dot-terminated
	if (s2[j]=='.') j--;	// s2 may be dot-terminated
  
	if (i <= j) return(0);

	// check if s1 terminates with s2
	for(;i >=0 && j >=0;i--,j--)
	{	if(s1[i] != s2[j])
			return(0);
	}

	// check if in s1, s2 is preceded by '/,[ ]*/'
	while(s1[i]==' ') i--;

	return(s1[i]==',');
}

#define BUFFER_LENGTH 20480

char *tester::extract_i(char *st1, char *st2)
{	static char  b[BUFFER_LENGTH];
	char *p;
	int plen, llen, difflen;

	p = strstr(st1,"(");
	p++;
  
	llen = strlen(st2);
	plen = strlen(p);
	difflen = plen - llen;
  
	while(p[difflen] != ',')
		difflen--;

	strncpy(b,p,difflen);
	b[difflen] = '\0';
  
	return b;
}

int tester::get_ispreferred_index(char *str)
{	char *sg1, *sg2;
	int i,j;
	struct node *n;
  
	//printf("THE STRING SENT IS %s\n",str);
	sg1 = strstr(str,"(");
	i=strlen(sg1);
	while (sg1[i-1]=='.') i--;	/* ignore ending dot, if present */
  
	for(n=ispreferredlist->first();n!=NULL && BASICDATA(n)->value!=NULL;n=n->next)
	{	sg2 = strstr(BASICDATA(n)->value,"(");

		j=strlen(sg2);
		while (sg2[j-1]=='.') j--;	/* ignore ending dot, if present */

		//printf("THE STRINGS ARE %s and %s\n",sg2,sg1);

		if ((i==j) && (strncmp(sg1,sg2,i) == 0))
			return BASICDATA(n)->value1;
	}
	return(-1);
}

void tester::output(model *M)
{	FILE *fp;

	struct node *n1,*n2; 
	int ind1,ind2,ind3,ind4;
 
 	basic_list oispreferredlist,oapplcrlist;
 
 	int tester_atomcount;

	fp=open_write(file);

	tester_atomcount=store->get_available();

	for (n1=M->appllist.first();n1!=NULL && BASICDATA(n1)->value!=NULL;n1=n1->next)
	{	char *name;

		name=BASICDATA(n1)->value;
		int fred = BASICDATA(n1)->value1;

//		name=strstr(BASICDATA(n1)->value,"cr__applcr(");
//		if(name!=NULL)
		if (startsWith(name,APPLCR"("))
		{	char *s;

			// manual grounding of o_appl(R) atoms
			s = (char *)calloc(strlen(name)+4, sizeof(char));
			sprintf(s,"o_"APPL"(%s.", &name[strlen(APPLCR"(")]);
       
			fprintf(fp,"1 ");
			fprintf(fp,"%d ",tester_atomcount);
			fprintf(fp,"0 ");
			fprintf(fp,"0 ");
			fprintf(fp,"\n");
       
			oapplcrlist.addNode(s,tester_atomcount);
			free(s);
			tester_atomcount++;
		}
	}

	for (n1=M->mlist.first();n1!=NULL && BASICDATA(n1)->value!=NULL;n1=n1->next)
	{	char *name;

		name=BASICDATA(n1)->value;
//		name=strstr(BASICDATA(n1)->value,"cr__is_preferred(");
//		if(name!=NULL)
		if (startsWith(name,IS_PREF"("))
		{	char *s;

			// manual grounding of o_is_preferred(R1,R2) atoms
			s=(char *)calloc(strlen(name)+4, sizeof(char));
			sprintf(s,"o_%s.",name);

			fprintf(fp,"1 ");
			fprintf(fp,"%d ",tester_atomcount);
			fprintf(fp,"0 ");
			fprintf(fp,"0 ");
			fprintf(fp,"\n");

			oispreferredlist.addNode(s,tester_atomcount);
			free(s);
			tester_atomcount++;
		}
	}

	// manual grounding for rule :- not dominates.
	fprintf(fp,"1 ");
	fprintf(fp,"1 ");      
	fprintf(fp,"1 ");
	fprintf(fp,"1 ");   
	fprintf(fp,"%d ",store->get(store->DOMINATE));
	fprintf(fp,"\n");

	// manual grounding of rule 
	//	dominates :- appl(I),o_appl(J),is_preferred(I,J), o_is_preferred(I,J).
	for(n1=oapplcrlist.first();n1!=NULL && BASICDATA(n1)->value!=NULL;n1=n1->next)
	{	char *s;

		// Extract J from o_applcr(J)
		ind1 = BASICDATA(n1)->value1;
		s = BASICDATA(n1)->value;
		s = strstr(s,"(");
		if (s!= NULL) s++;
	
		//fprintf(stderr,"APP IS %s\n",s);
		n2=oispreferredlist.first();
	  
	  
		/* printf("IN THE TESTER PRINTING OISPREFERE\n");
	  
		   fprintf(stderr,"----\n");
		   oispreferredlist.print_pairs(stderr);
		   fprintf(stderr,"----\n");
	 
		   printf("ENDDDIN THE TESTER PRINTING OISPREFERE\n");
		*/

		for(n2=oispreferredlist.first();n2!= NULL  && BASICDATA(n2)->value!=NULL;n2 = n2->next)
		{	char *ivalue;
			char *cp;
			int ind5;

			cp = BASICDATA(n2)->value;
			if (!look_for_j(cp,s)) continue;

			ind2 = BASICDATA(n2)->value1;

			//printf("THE CP VALUE IS %s\n",cp);
			ivalue=extract_i(cp,s);
	      
	     
			//printf("THE I value is %s\n",i); 
			ind3 = appllist->find_arity1_atom(ivalue);
			if (ind3<0) continue;
	      
			//printf("THE PTR@_>VALUE is %s\n",cp);
	      		ind4 = get_ispreferred_index(cp);
			if (ind4<0) continue;

			// manual grounding of rule
			//	dominates :- appl(I), not appl(J), o_appl(J), is_preferred(I,J), o_is_preferred(I,J).
			if(non_exclusive){
				//strip trailing ). from s
				s[strlen(s)-2]=0;

				//find the appl(J) atom
				ind5 = appllist->find_arity1_atom(s);

				fprintf(fp,"1 ");
				fprintf(fp,"%d ",store->get(store->DOMINATE));
				fprintf(fp,"5 ");
				fprintf(fp,"1 ");
				fprintf(fp,"%d ",ind5);
				fprintf(fp,"%d ",ind1);
				fprintf(fp,"%d ",ind2);
				fprintf(fp,"%d ",ind3);
				fprintf(fp,"%d ",ind4);
				fprintf(fp,"\n");
			}
			else{
				//printf("THE APPLCR INDEX3 IS %d\n",ind3);
				//printf("THE APPLCR INDEX4 IS %d\n",ind4);

				// write the rule now
				fprintf(fp,"1 ");
				fprintf(fp,"%d ",store->get(store->DOMINATE));
				fprintf(fp,"4 ");
				fprintf(fp,"0 ");
				fprintf(fp,"%d ",ind1);
				fprintf(fp,"%d ",ind2);
				fprintf(fp,"%d ",ind3);
				fprintf(fp,"%d ",ind4);
				fprintf(fp,"\n");
	/*
				fprintf(stdout,"1 ");
				fprintf(stdout,"%d ",store->get(store->DOMINATE));
				fprintf(stdout,"4 ");
				fprintf(stdout,"0 ");
				fprintf(stdout,"%d ",ind1);
				fprintf(stdout,"%d ",ind2);
				fprintf(stdout,"%d ",ind3);
				fprintf(stdout,"%d ",ind4);
				fprintf(stdout,"\n");
	*/		}
		}
	}


#ifdef DO_SETPREFS
	doSetPrefs(fp,M,&tester_atomcount);	/* marcy [062106] */
#endif


/* PARETO-style preference
 *
 * To implement Pareto-style preferences, we need to add the requirement
 * that the new answer set is NOT dominated by the one being tested.
 *
 * This can be accomplished by adding a constraint:
 *
 *      :- appl(J),o_appl(I),is_preferred(I,J),o_is_preferred(I,J).
 *
 */
	if (use_pareto)
	{	// manual grounding of constraint:
		//   :- appl(J),o_appl(I),is_preferred(I,J),o_is_preferred(I,J).
    
		for(n1=appllist->first();n1!=NULL && BASICDATA(n1)->value != NULL;n1=n1->next)
		{	char *s;

			// Extract J from appl(J)
			ind1 = BASICDATA(n1)->value1;
			s = BASICDATA(n1)->value;
			s = strstr(s,"(");
			if(s!= NULL) s++;
	
			//printf("APP IS %s IND is %d\n",s,ind1);

			n2 = oispreferredlist.first();
	  
	  
			/*printf("IN THE TESTER PRINTING OISPREFERE\n");
		  
			  fprintf(stderr,"----\n");
			  oispreferredlist.print_pairs(stderr);
			  fprintf(stderr,"----\n");
		 
			  printf("ENDDDIN THE TESTER PRINTING OISPREFERE\n");
			*/
	  
			for(n2 = oispreferredlist.first();n2 != NULL  && BASICDATA(n2)->value != NULL;n2 = n2->next)
			{	char *ivalue;
				char *cp;

				cp = BASICDATA(n2)->value;
//printf("%s  %s\n",cp,s);

				if (!look_for_j(cp,s)) continue;

				ind2 = BASICDATA(n2)->value1;

				//printf("THE CP VALUE IS %s IND=%d\n",cp,ind2);
				ivalue=extract_i(cp,s);
	      
	     
				//printf("THE I value is %s\n",ivalue);
				ind3=oapplcrlist.find_arity1_atom(ivalue);
				if (ind3<0) continue;
	      
				//printf("THE PTR@_>VALUE is %s\n",cp);
	      
				ind4 = get_ispreferred_index(cp);
				if (ind4<0) continue;
	      
				//printf("THE APPLCR INDEX3 IS %d\n",ind3);
				//printf("THE APPLCR INDEX4 IS %d\n",ind4);

				// write the rule now
				fprintf(fp,"1 ");
				fprintf(fp,"1 ");
				fprintf(fp,"4 ");
				fprintf(fp,"0 ");
				fprintf(fp,"%d ",ind1);
				fprintf(fp,"%d ",ind2);
				fprintf(fp,"%d ",ind3);
				fprintf(fp,"%d ",ind4);
				fprintf(fp,"\n");
/*
				fprintf(stdout,"1 ");
				fprintf(stdout,"1 ");
				fprintf(stdout,"4 ");
				fprintf(stdout,"0 ");
				fprintf(stdout,"%d ",ind1);
				fprintf(stdout,"%d ",ind2);
				fprintf(stdout,"%d ",ind3);
				fprintf(stdout,"%d ",ind4);
				fprintf(stdout,"\n");
*/
			}
		}
	}	/* end if (use_pareto) */

   fclose(fp);
} 
