#ifndef CRMODELS_PROGRAM_H
#define CRMODELS_PROGRAM_H

#include <stdio.h>

#include "extended_data.h"
#include "hash.h"


class atom_lists
{
public:
	int atomcount;
	basic_list ispreferredlist;
	basic_list appllist;
	basic_list applcrlist;
	basic_list ispreferred2list;
	basic_list crhead;
#if 0
	hash_list allatomlist;
	hash_list btatom_list;
#else
	tree_list allatomlist;
	tree_list btatom_list;
#endif

	bool *removed_atoms;
	
	atom_lists()
	{	removed_atoms=NULL;
	}
};

class rule_lists
{
public:
	int rulecount;
	rule_list rules;
	rule_list **by_head;		/* rules grouped by their heads */
	rule_list **by_head_n_body;	/* rules grouped by their heads and bodies */

	basic_list *active_crrules_list;

	bool *removed_rules;
	bool *crtrans_rules;

	rule_lists()
	{	active_crrules_list=NULL;
		removed_rules=NULL;
		crtrans_rules=NULL;
	}
};

class program
{
private:
	void output(FILE *fp,int *array,int len);
	void output_grounding(FILE *fp,rule_data *rule);
	void output_grounding(FILE *fp,basic_data *atom);

public:
	atom_lists *a;
	rule_lists *r;

	program()
	{	a=new atom_lists();
		r=new rule_lists();
	}

	void output_grounding(FILE *fp,bool output_compute);
};


#endif /* CRMODELS_PROGRAM_H */
