#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extended_data.h"
#include "hash.h"

basic_data::~basic_data()
{	if (value) free(value);
}

int basic_data::hash_function(hash *h)
{	return(h->hash_function(value));
}

bool basic_data::matches(const void *search)
{	return(strcmp((const char *)search,value)==0);
}

/*------------------------------------------*/


rule_data::~rule_data()
{	if (pos) free(pos);
	if (neg) free(neg);
}


/*------------------------------------------*/

void basic_list::addNode(char *str,int idx)
{	basic_data *d;
	
	d=new basic_data();
	d->value = strdup(str);
	d->value1 = idx;

	list::addNode(d);
}

/*
 * Determine the length of a list containing
 * of basic_data data items.
 */
int basic_list::len(void)
{	int sz;
	struct node *n;

	for(sz=0,n=h->next;n != NULL && BASICDATA(n)->value != NULL;sz++,n=n->next);

	return(sz);
}    

int basic_list::_find_atomnumber(const char *str)
{	struct node *t1;

	for(t1 = first();t1 != NULL && BASICDATA(t1)->value1 != 0 && BASICDATA(t1)->value != NULL;t1 = t1->next)
	{	if(strcmp(BASICDATA(t1)->value, str) == 0)
			return(BASICDATA(t1)->value1);
	}
   
	return(0);
}    

int basic_list::find_atomnumber(const char *str)
{	int idx;
	char *s;

	idx=_find_atomnumber(str);

	if (idx>0) return(idx);

	/*
	 * In cases I don't entirely understand,
	 * lparse adds "___" in front of the atom name.
	 *
	 */
	s=(char *)calloc(strlen(str)+3+1,sizeof(char));
	sprintf(s,"___%s",str);

	idx=_find_atomnumber(s);
	if (idx>0) return(idx);

/*
 * We no longer give error when the atom number was not found [marcy 052109]
 * This is because now crmodels supports ezcsp, anc ezcsp's models may
 * contain cspeq/2 atoms which didn't occur in the grounding of the program.
 *
	fprintf(stderr," Internal Error: Cannot find number for atom %s\n",strn);
	exit(1);
*/
	return(0);
}

int basic_list::get_max_atomindex()
{	struct node *n;
	int mx;

	mx=0;
	for(n=first();n!=NULL && BASICDATA(n)->value1!=0 && BASICDATA(n)->value!=NULL;n=n->next)
	{	if(mx<BASICDATA(n)->value1)
			mx=BASICDATA(n)->value1;
	}

	return(mx);
}    


int basic_list::find_arity1_atom(const char *str)
{	struct node *ptt;
	char *st, *st1;
   
	st = (char *)calloc(strlen(str) +2, sizeof(char));
	sprintf(st,"%s)",str);
  
  
	ptt = first();

	while(ptt != NULL && BASICDATA(ptt)->value != NULL)
	{	st1 = strstr(BASICDATA(ptt)->value,"(");
		st1++;
		//fprintf(stderr,"THE GETINDEX ST1 %s and ST2 %s\n",st1,st);
     
		if ((strncmp(st,st1,strlen(st)) == 0) &&
		    ((st1[strlen(st)]=='.') ||
		     (st1[strlen(st)]=='\0')))
		{	free(st);
			return(BASICDATA(ptt)->value1);
		}

		ptt = ptt->next;
	}  

	free(st);
	return(-1);
}

void basic_list::print_pairs(FILE *fp)
{	struct node *p;

	for(p=first();p!=NULL && BASICDATA(p)->value!=NULL;p=p->next)
		fprintf(fp,"%s %d\n", BASICDATA(p)->value, BASICDATA(p)->value1);
}

void basic_list::print(FILE *fp)
{	struct node *p;

	for(p=first();p!=NULL && BASICDATA(p)->value!=NULL;p=p->next)
		fprintf(fp,"%s %d\n", BASICDATA(p)->value, BASICDATA(p)->value1);
}

/*------------------------------------------*/


void rule_list::addNode(rule_data *r)
{	list::addNode(r);
}

void rule_list::removeNode(rule_data *r)
{	list::removeNode(r);
}
