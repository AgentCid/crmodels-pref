#ifndef CRLIST_H
#define CRLIST_H

class hash;

class head
{
public:

	struct node *next;
	struct node *last;

	head();
};

class node_data
{
public:
	virtual int hash_function(hash *h)		/* for hashes */
	{	return(0);
	}

	virtual bool matches(const void *search)	/* for hashes */
	{	return(false);
	}

	virtual ~node_data(){}
};

struct node
{	node_data *data;

	struct node *next;
};

class list
{
public:
	head *h;

	struct node *first(void);

	virtual int len(void);

	void addNode(node_data *d);

	void removeNode(node_data *r);

	list();

	virtual ~list();
};

#endif /* CRLIST_H */
