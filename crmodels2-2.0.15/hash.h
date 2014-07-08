#ifndef HASH_H
#define HASH_H

#include "crlist.h"
#include "extended_data.h"

class hash
{
private:
	int HASH_SIZE;

	head *source;

	bool sized;	/* HASH_SIZE set and memory allocated */
	bool ready;	/* all nodes added to hash */

	void prepare(void);
	void add(node_data *d);

public:
	struct hash_head **heads;

	hash(head *h);
	virtual int hash_function(char *str);

	node_data *find(const void *search,int hash_val);

	virtual ~hash() {}
};

struct hash_head
{	struct hash_node *next;
};

struct hash_node
{	node_data *data;

	struct hash_node *next;
};

class hash_list : public basic_list
{
protected:
	virtual int _find_atomnumber(const char *str);

public:
	class hash *hash;

	hash_list();

	virtual ~hash_list() {}
};


/*---------------------------------*/

class tree_node
{
public:
	struct basic_data *zero;

	/* array of children nodes, one for each possible
	 * char in the string.
	 * The elements are 254 instead of 255 because
	 * the pointer for chr(0) is in field zero.
	 */
	tree_node *children[254];
};

class tree_list : public basic_list
{
private:
	bool ready;

	tree_node *tree;

	void add(basic_data *d);
	void prepare(void);

protected:
	virtual int _find_atomnumber(const char *str);

public:
	tree_list();

	virtual ~tree_list() {}
};


#endif /* HASH */
