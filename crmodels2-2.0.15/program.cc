#include <stdio.h>
#include <stdlib.h>

#include "program.h"

#include "debug.h"

void program::output(FILE *fp,int *array,int len)
{	int i;

	for(i=0;i<len;i++)
		fprintf(fp,"%d ",array[i]);
}

void program::output_grounding(FILE *fp,rule_data *rule)
{	if (rule->type==1 && r->removed_rules && r->removed_rules[rule->index]) return;

	if (rule->string)
		fprintf(fp,"%s\n",rule->string);
	else
	{	fprintf(fp,"%d ",rule->type);

		if (rule->type==3)
			fprintf(fp,"%d ",rule->n_head);
		output(fp,rule->head,rule->n_head);

		if (rule->type==5)
			fprintf(fp,"%d ",rule->bound);

		fprintf(fp,"%d %d ",rule->n_pos+rule->n_neg,rule->n_neg);

		if (rule->type==2)
			fprintf(fp,"%d ",rule->bound);

		output(fp,rule->neg,rule->n_neg);
		output(fp,rule->pos,rule->n_pos);

		if (rule->type==5 || rule->type==6)
			output(fp,rule->weights,rule->n_neg+rule->n_pos);

		fprintf(fp,"\n");
	}
}

void program::output_grounding(FILE *fp,basic_data *atom)
{	if (r->removed_rules==NULL || !a->removed_atoms[atom->value1])
		fprintf(fp,"%d %s\n",atom->value1,atom->value);
}

void program::output_grounding(FILE *fp,bool output_compute)
{	struct node *n;

	for(n=r->rules.first();n;n=n->next)
		output_grounding(fp,RULEDATA(n));

	fprintf(fp,"0\n");
	printTimestamp("printRules");

	for(n=a->allatomlist.first();n;n=n->next)
		output_grounding(fp,BASICDATA(n));

	fprintf(fp,"0\n");
	printTimestamp("printAtoms");

	if (output_compute)
		fprintf(fp,"B+\n0\nB-\n1\n0\n1\n");
}
