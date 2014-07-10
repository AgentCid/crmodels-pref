#ifndef CRMODELS_TESTER_H
#define CRMODELS_TESTER_H

#include <regex.h>	/* for doSetPrefs() */

#include "atom_store.h"
#include "extended_data.h"
#include "model.h"


class tester
{
private:
	int look_for_j(char *s1, char *s2);
	char *extract_i(char *st1, char *st2);
	int get_ispreferred_index(char *strg);
	void store_match(char *s,char *orig,regmatch_t *m);
	void doSetPrefs(FILE *fp,model *M,int *tester_atomcount);


public:

	char *file;

	basic_list *appllist;
	basic_list *ispreferred2list;
	basic_list *applcrlist;
	basic_list *ispreferredlist;

	atom_store *store;

	int use_pareto;
	bool non_exclusive;

	void output(model *M);
};

#endif /* CRMODELS_TESTER_H */
