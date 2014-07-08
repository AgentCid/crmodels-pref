#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compile-flags.h"

#include "keywords.h"
#include "extended_data.h"
#include "stringutils.h"
#include "program.h"

#include "filter_crrules.h"

extern program *P;

int filterInactiveCRRules(int n_crnames)
{	struct node *n;

	int i;

	P->r->by_head=new rule_list*[P->a->atomcount];
	for(i=0;i<P->a->atomcount;i++)
		P->r->by_head[i]=new rule_list();

	P->r->by_head_n_body=new rule_list*[P->a->atomcount];
	for(i=0;i<P->a->atomcount;i++)
		P->r->by_head_n_body[i]=new rule_list();

	for(struct node *rn=P->r->rules.first();rn!=NULL;rn=rn->next)
	{	for (i=0;i<RULEDATA(rn)->n_head;i++)
		{	P->r->by_head[RULEDATA(rn)->head[i]]->addNode(RULEDATA(rn));
			P->r->by_head_n_body[RULEDATA(rn)->head[i]]->addNode(RULEDATA(rn));
		}
		for (i=0;i<RULEDATA(rn)->n_pos;i++)
			P->r->by_head_n_body[RULEDATA(rn)->pos[i]]->addNode(RULEDATA(rn));
		for (i=0;i<RULEDATA(rn)->n_neg;i++)
			P->r->by_head_n_body[RULEDATA(rn)->neg[i]]->addNode(RULEDATA(rn));
	}

	for(n=P->a->applcrlist.first();n!=NULL;n=n->next)
	{	char name[1024],bodytrue[1024];
		int bt_i;
		struct node *rn;

		if (BASICDATA(n)->value)
		{	strcpy(name,&BASICDATA(n)->value[11]);
			name[strlen(name)-1]=0;
			sprintf(bodytrue,BODYTRUE"(%s)",name);

			//fprintf(stderr,"%s -> ",name);

			bt_i=P->a->btatom_list.find_atomnumber(bodytrue);

			/*
			 * Cases when we know that the cr-rule r is inactive:
			 *
			 *   1) the cr__bodytrue(r) isn't even in the atom section
			 *      output by the grounder;
			 *   2) there is no rule with cr__bodytrue(r) in the head.
			 */

			if (bt_i>0)
				rn=P->r->by_head[bt_i]->first();
			else
				rn=NULL;

			/*
			if (rn!=NULL)
			{	fprintf(stderr,"FOUND (%d)!\n",bt_i);
			}
			else
			{	fprintf(stderr,"NOT found.\n");
			}
			*/

			if (rn==NULL) n_crnames--;
		}
	}

/*	for(i=0;i<P->a->atomcount;i++) free(P->r->by_head[i]);
	free(P->r->by_head);
*/

	return(n_crnames);
}


void filterCRAtomByIndex(int atom)
{	struct node *n;

	for(n=P->r->by_head_n_body[atom]->first();n;n=n->next)
		if (RULEDATA(n)->type==1)
		{	P->r->removed_rules[RULEDATA(n)->index]=true;
			P->r->crtrans_rules[RULEDATA(n)->index]=true;
		}

	P->a->removed_atoms[atom]=true;
}

void filterCRAtoms(basic_list *list)
{	struct node *n;

	for(n=list->first();n;n=n->next)
		filterCRAtomByIndex(BASICDATA(n)->value1);
}

void filterAllCRStuff(void)
{	P->r->removed_rules=(bool *)calloc(P->r->rulecount,sizeof(bool));
	P->r->crtrans_rules=(bool *)calloc(P->r->rulecount,sizeof(bool));
	P->a->removed_atoms=(bool *)calloc(P->a->atomcount,sizeof(bool));

	filterCRAtoms(&P->a->applcrlist);
	filterCRAtoms(&P->a->btatom_list);
	filterCRAtoms(&P->a->appllist);
	filterCRAtoms(&P->a->crhead);
}

void extractAtomsBUTForCRRules(basic_list *source,basic_list *active_crrules,basic_list *dest)
{	struct node *n;

	for(n=source->first();n;n=n->next)
	{	if (BASICDATA(n)->value[strlen(BASICDATA(n)->value)-1]==')')
		{	bool for_active_rule;
			struct node *n2;

			for_active_rule=false;
			for(n2=active_crrules->first();n2;n2=n2->next)
			{	char s[10240];

				sprintf(s,"%s)",BASICDATA(n2)->value);
				if (endsWith(BASICDATA(n)->value,s))
				{	for_active_rule=true;
					break;
				}
			}
			if (!for_active_rule) dest->addNode(BASICDATA(n));
		}
	}
}

void filterNoCRStuff(void)
{	if (P->r->removed_rules)
	{	free(P->r->removed_rules);
		P->r->removed_rules=NULL;
	}
	if (P->a->removed_atoms)
	{	free(P->a->removed_atoms);
		P->a->removed_atoms=NULL;
	}

	P->r->removed_rules=(bool *)calloc(P->r->rulecount,sizeof(bool));
	P->a->removed_atoms=(bool *)calloc(P->a->atomcount,sizeof(bool));
}

void filterCRStuffBUTForCRRules(basic_list *active_crrules)
{	basic_list cratom_list;

	if (P->r->removed_rules)
	{	free(P->r->removed_rules);
		P->r->removed_rules=NULL;
	}
	if (P->a->removed_atoms)
	{	free(P->a->removed_atoms);
		P->a->removed_atoms=NULL;
	}

	P->r->removed_rules=(bool *)calloc(P->r->rulecount,sizeof(bool));
	P->a->removed_atoms=(bool *)calloc(P->a->atomcount,sizeof(bool));

	if (active_crrules==NULL) return;

	extractAtomsBUTForCRRules(&P->a->applcrlist,active_crrules,&cratom_list);
	extractAtomsBUTForCRRules(&P->a->btatom_list,active_crrules,&cratom_list);
	extractAtomsBUTForCRRules(&P->a->appllist,active_crrules,&cratom_list);
	extractAtomsBUTForCRRules(&P->a->crhead,active_crrules,&cratom_list);

	filterCRAtoms(&cratom_list);
}
