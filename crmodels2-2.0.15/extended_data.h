#ifndef EXTENDED_DATA_H
#define EXTENDED_DATA_H

#include <stdio.h>

#include "crlist.h"

class basic_data: public node_data
{
public:
	char *value;
	int value1;

	virtual int hash_function(hash *h);

	virtual bool matches(const void *search);

	virtual ~basic_data();
};
#define BASICDATA(n) ((basic_data *)((n)->data))


class rule_data: public node_data
{
public:
	int index;

	int type;

	int n_head;
	int *head;

	int n_pos;
	int n_neg;
	int bound;	/* for constraint rules and weight rules */
	int *pos;
	int *neg;
	int *weights;	/* for weight rules */

	char *string;	/* smodels-format representation of the rule */

	virtual ~rule_data();
};
#define RULEDATA(n) ((rule_data *)((n)->data))

class rule_list : public list
{
public:
	void addNode(rule_data *r);
	void removeNode(rule_data *r);

	virtual ~rule_list() {}
};


class basic_list : public list
{
protected:
	virtual int _find_atomnumber(const char *str);

public:

	virtual int len(void);

	using list::addNode;
	void addNode(char *str,int idx=0);

	virtual int find_atomnumber(const char *str);

	int get_max_atomindex();

	int find_arity1_atom(const char *str);

	void print(FILE *fp);
	void print_pairs(FILE *fp);

	virtual ~basic_list() {}
};

#endif /* EXTENDED_DATA_H */
