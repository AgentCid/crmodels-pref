#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compile-flags.h"

#include "keywords.h"
#include "model.h"	/* for class atom_utils */
#include "stringutils.h"
#include "fileutils.h"
#include "filter_crrules.h"

#include "loader.h"

void loader::store_literal(char *line, int &number_of_crnames)
{	char *p,*q;
	int idx;

	idx=atoi(line);
	
	 
	for(q=line; *q!= ' '; q++);
	q++;

	/*
	 * In cases I don't entirely understand,
	 * lparse adds "___" in front of the atom name.
	 *
	 */
	if (atom_utils::three_underscore_prefix(q))
		p=&q[3];
	else
		p=q;

	P->a->allatomlist.addNode(q,idx);

//	if (atom_utils::not_cratom(p))
//		P->a->atomlist.addNode(q,idx);

	if (startsWith(p,APPLCR"("))
		P->a->applcrlist.addNode(q,idx);
	 
	if (startsWith(p,BODYTRUE"("))
		P->a->btatom_list.addNode(q,idx);
	
	if (startsWith(p,APPL"("))
		P->a->appllist.addNode(q,idx);
	  
	if (startsWith(p,IS_PREF"("))
		P->a->ispreferredlist.addNode(q,idx);
	  
	if (startsWith(p,IS_PREF2"("))
		P->a->ispreferred2list.addNode(q,idx);
		
	if (startsWith(p,CRNAME))
	{	P->a->crhead.addNode(q,idx);
		number_of_crnames++;
	}
} 

void loader::update_atomcount(int idx)
{	/* Because this may be the last literal in the table,
	 * we assume that it is and initialize the P->a->atomcount etc.
	 * accordingly. If it is not, P->a->atomcount etc. will be
	 * overwritten at the next call to store_literal().
	 *
	 * Because gringo outputs the atoms in the atom list section
	 * in arbitrary order (not ordered by value as in lparse),
	 * we have to identify the max index.
	 */
	if (P->a->atomcount<idx+1)
		P->a->atomcount=idx+1;
}

void loader::add_extra_atoms(void)
{	store->set(store->OK,++P->a->atomcount);
	store->set(store->OK1,++P->a->atomcount);
	store->set(store->BAD,++P->a->atomcount);
	store->set(store->DOMINATE,++P->a->atomcount);
	store->set(store->TRUE,++P->a->atomcount);

	store->set_available(++P->a->atomcount);
}

void loader::store_rule(char *line)
{	int rule_type;
	int i;
	char *rule_start;

	rule_start=line;
				
	sscanf(line,"%d",&rule_type);
	for(i=0;line[i]!=' ';i++); i++;	/* skip rule type */

	switch(rule_type)
	{
		case 1:	/* ``standard'' rule */
		case 2: /* ``constraint'' rule */
		case 3:	/* ``choice'' rule */
		case 5: /* ``weight'' rule */
		case 6: /* ``minimize'' rule */
			{	rule_data *d;
				int total;
				int j;

				d=new rule_data();
				d->index=P->r->rulecount++;
				d->type=rule_type;

				d->string=strdup(rule_start);

				if (rule_type==1 || rule_type==2 || rule_type==5 || rule_type==6)
					d->n_head=1;
				else
				{	sscanf(&line[i],"%d",&d->n_head);
					for(;line[i]!=' ';i++); for(;line[i]==' ';i++);	/* skip n_head */
				}

				d->head=(int *)calloc(d->n_head,sizeof(int));
				for(j=0;j<d->n_head;j++)
				{	sscanf(&line[i],"%d",&d->head[j]);
					update_atomcount(d->head[j]);

					for(;line[i]!=' ';i++); for(;line[i]==' ';i++);	/* skip field */
				}

				if (rule_type==5)
				{	sscanf(&line[i],"%d",&d->bound);

					for(;line[i]!=' ';i++); for(;line[i]==' ';i++);		/* skip bound */
				}

				sscanf(&line[i],"%d %d",&total,&d->n_neg);
				d->n_pos=total-d->n_neg;

				for(;line[i]!=' ';i++); for(;line[i]==' ';i++);		/* skip total */
				for(;line[i]!=' ';i++); for(;line[i]==' ';i++);		/* skip n_neg */


				if (rule_type==2)
				{	sscanf(&line[i],"%d",&d->bound);

					for(;line[i]!=' ';i++); for(;line[i]==' ';i++);		/* skip bound */
				}
				

				d->neg=(int *)calloc(d->n_neg,sizeof(int));
				for(j=0;j<d->n_neg;j++)
				{	sscanf(&line[i],"%d",&d->neg[j]);
					update_atomcount(d->neg[j]);

					for(;line[i]!=' ';i++); for(;line[i]==' ';i++);	/* skip field */
				}
				d->pos=(int *)calloc(d->n_pos,sizeof(int));
				for(j=0;j<d->n_pos;j++)
				{	sscanf(&line[i],"%d",&d->pos[j]);
					update_atomcount(d->pos[j]);

					for(;line[i]!=' ';i++); for(;line[i]==' ';i++);	/* skip field */
				}

				if (rule_type==5 || rule_type==6)
				{	d->weights=(int *)calloc(d->n_neg+d->n_pos,sizeof(int));
					for(j=0;j<d->n_neg+d->n_pos;j++)
					{	sscanf(&line[i],"%d",&d->weights[j]);
						for(;line[i]!=' ';i++); for(;line[i]==' ';i++);	/* skip field */
					}
				}
				P->r->rules.addNode(d);
			}
			break;
		default: /* all others */
			fprintf(stderr,"ERROR: unhandled smodels rule type %d\n",rule_type);
			exit(1);
			break;
		
	}
} 

int loader::check_for_newline(char *buffer,char *&last_line_beg,int &zerocount,int &i, int &number_of_crnames)
{	int flag =0;

	while(buffer[i])
	{
		if(buffer[i] == '\n')
		{	buffer[i] = '\0';	
	      
			if(strcmp(last_line_beg,"0") == 0)
				zerocount++; 
			else
			{	switch(zerocount)
				{	case 1:	/* inside atom list */
						store_literal(last_line_beg, number_of_crnames);
						break;
/*						if((zerocount == 1) && (strcmp(last_line_beg,"0") != 0))
						{	// arrptr[k] = last_line_beg;
							//k++;
							store_literal(last_line_beg, number_of_crnames);
						}
*/
					case 0:	/* inside rule list */
						store_rule(last_line_beg);
						break;
				}
			}

			last_line_beg = &buffer[i+1];	
			flag =1;
		}
	    
		i++;
	}

	return flag;
}	 
      
int loader::check_for_lastchar(char *buffer,int &i)
{	return(buffer[i-1] == '\n');
}     



int loader::copybuffer(char *src, char *dest)
{	int i;

	for(i=0;src[i];i++)
		dest[i] = src[i];

	return(i);
}

/*
 * Because gringo does not emulate lparse's "-d all" behavior, program hr
 * adds a rule { cr__stop } to prevent it from dropping the
 * "not cr__stop" condition in the hard reduct.
 *
 * Now that the grounding has been performed, here we can safely
 * remove { cr__stop }, as cr2 will take care of making cr__stop
 * true when needed.
 */
void loader::eliminate_cr_stop_rule(void)
{	struct node *n;
	int cr_stop_idx;

	cr_stop_idx=0;
	for(n=P->a->allatomlist.first();n;n=n->next)
	{	if (strcmp(BASICDATA(n)->value,STOP)==0)
		{	cr_stop_idx=BASICDATA(n)->value1;
			break;
		}
	}

	if (cr_stop_idx==0) return;

	for(n=P->r->rules.first();n!=NULL;n=n->next)
	{	rule_data *r=RULEDATA(n);

		if (r->type==3 && r->n_head==1 && r->head[0]==cr_stop_idx && r->n_pos+r->n_neg==0)
		{	P->r->rules.removeNode(r);
			break;
		}
	}
}

int loader::load_program(char *file,program *P_)
{	FILE *fp;
	int i;
	char *buffer=NULL;
	int buffersize;
	size_t bytes_read;
	int last_char_newline,atleast_one_line,initial, num;
	int number_of_crnames;

	int zerocount;
	char *last_line_beg; 


	P=P_;


	P->a->atomcount=0;
	P->r->rulecount=0;

	fp=open_read(file);

	i=0;
	last_char_newline =0;
	atleast_one_line=0;
	initial=1;
	num=0;
	number_of_crnames=0;
	buffersize=102400;
	zerocount=0;
	while(!feof(fp) && (zerocount != 2))
	{
		i=0;   

		/* If it is the start of file read or if the 
		** last character read was a new line
		*/   
		if(last_char_newline || initial)
		{	
			atleast_one_line=0;

			if (buffer!=NULL/* && (zerocount == 0)*/)
				free(buffer);

			buffer = (char *)calloc(buffersize, sizeof(char));
			bytes_read = fread(buffer, 1, buffersize-1, fp);

			last_line_beg = buffer;
	  
			atleast_one_line = check_for_newline(buffer,last_line_beg,zerocount, i, number_of_crnames);
			last_char_newline = check_for_lastchar(buffer,i);
	   
			initial=0;
		}
		else
		/* If the last character in the buffer was not
		** a new line and if there was atleast one line in 
		** the buffer
		*/

		if(!last_char_newline )
		{	char *buffer2;

			if(!atleast_one_line)
				buffersize = buffersize *2;

			atleast_one_line =0;

			buffer2 = (char *)calloc(buffersize, sizeof(char));

			/* copy the elements from the previous
			   incomplete line to the new buffer */
			num = copybuffer(last_line_beg, buffer2);

			/*if (zerocount == 0)*/
			free(buffer);

			buffer=buffer2;
			int diff = buffersize-num;
			bytes_read = fread(&buffer[num], 1, diff-1, fp);

			last_line_beg = buffer;

			atleast_one_line = check_for_newline(buffer,last_line_beg,zerocount, i, number_of_crnames);
			last_char_newline = check_for_lastchar(buffer,i);         
		}
	}

	fclose(fp);

	add_extra_atoms();

	eliminate_cr_stop_rule();

	number_of_crnames=filterInactiveCRRules(number_of_crnames);

	return(number_of_crnames);
} 
