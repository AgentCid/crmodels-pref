#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"


hash::hash(head *h)
{	sized=false;
	ready=false;

	source=h;
}

void hash::prepare(void)
{	int i;
	struct node *n;

	int primes[]=
	{	/* list of primes from "Algorithms in Java, Part 5" */
		251,
		509,
		1021,
		2039,
		4093,
		8191,
		16381,
		32749,
		65521,
		131071,
		262139,
		524287,
		1048573,
		2097143,
		4194301,
		8388593,
		16777213,
		33554393,
		67108859,
		134217689,
		268435399,
		536870909,
		1073741789,
// we skip the last one to ensure we fit in 32-bit signed ints
//		2147483647,

		-1
	};

	if (ready) return;

	for(i=0,n=source->next;n!=NULL;n=n->next,i++);
//int listlen=i;
	i=i/10;	/* approximate hash size */
	/* now find the closest prime */
	HASH_SIZE=primes[0];
	for(int j=0;primes[j]>0 && primes[j]<i;j++)
		HASH_SIZE=primes[j];

//fprintf(stderr,"preparing hash of %d; list len is: %d\n",HASH_SIZE,listlen);


	heads=new hash_head* [HASH_SIZE];

	for(i=0;i<HASH_SIZE;i++)
		heads[i]=(struct hash_head *)calloc(1,sizeof(struct hash_head));

	sized=true;	/* HASH_SIZE set and memory allocated */


	for(n=source->next;n!=NULL;n=n->next)
		add(n->data);

	ready=true;	/* all nodes added to hash */
}

int hash::hash_function(char *str)
{	int i;

	if (!sized) prepare();

	for(i=0;*str;str++)
		i=(i+*str) % HASH_SIZE;

	return(i);
}

void hash::add(node_data *d)
{	struct hash_node *hn;
	int hv;

	if (!sized) prepare();

	hv=d->hash_function(this);

	hn=(struct hash_node *)calloc(1,sizeof(struct hash_node));
	hn->data=d;

	if (heads[hv]->next)
		hn->next=heads[hv]->next;
	heads[hv]->next=hn;
}

node_data *hash::find(const void *search,int hash_val)
{	struct hash_node *hn;

	if (!ready) prepare();

	for(hn=heads[hash_val]->next;hn;hn=hn->next)
	{	if (hn->data->matches(search))
			return(hn->data);
	}
	return(NULL);
}


/*------------------------------------------*/


hash_list::hash_list()
{	hash=new class hash(h);
}


int hash_list::_find_atomnumber(const char *str)
{	basic_data n,*d;
	int hv;

	n.value=(char *)str;
	hv=n.hash_function(hash);
	n.value=NULL;	/* prevent ~basic_data from free()ing the string!!! */

	d=(basic_data *)hash->find(str,hv);
	if (d)
		return(d->value1);

	return(0);
}    


/*------------------------------------------*/


tree_list::tree_list()
{	tree=new tree_node();
	ready=false;
}


int tree_list::_find_atomnumber(const char *str)
{	int i;
	tree_node *t;

	if (!ready) prepare();

	t=tree;
	for(i=0;str[i]!=0 && t!=NULL;i++)
		t=t->children[str[i]-1];	/* -1 because chr(0) is not in the array */

	if (t)
		return(t->zero->value1);

	return(0);
}    

void tree_list::add(basic_data *d)
{	char *str;
	int i;
	tree_node *t;

	str=d->value;
	t=tree;
	for(i=0;str[i]!=0;i++)
	{	if (t->children[str[i]-1]==NULL)	/* -1 because chr(0) is not in the array */
			t->children[str[i]-1]=new tree_node();
		t=t->children[str[i]-1];
	}
	t->zero=d;
}

void tree_list::prepare(void)
{	struct node *n;

	if (ready) return;

	for(n=first();n!=NULL;n=n->next)
		add(BASICDATA(n));

	ready=true;	/* all nodes added to hash */
}
