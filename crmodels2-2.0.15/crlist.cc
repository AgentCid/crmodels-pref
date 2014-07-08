#include <stdlib.h>

#include "crlist.h"
#include "hash.h"


head::head()
{	next=NULL;
	last=NULL;
}

/*------------------------------------------*/



list::list()
{	h=new head();
}


list::~list()
{	struct node *curr, *next;

	for(curr=h->next; curr !=NULL; curr=next)
	{	next=curr->next;

		//if (curr->data) delete curr->data;

		free(curr);
	}
	free(h);
}

struct node *list::first(void)
{	return(h->next);
}

void list::addNode(node_data *d)
{	struct node *newnode;

	newnode=(struct node *)calloc(1,sizeof(struct node));
	newnode->data=d;

	if (h->next==NULL)
		h->next=newnode;
	else
		h->last->next=newnode;

	h->last=newnode;

//	if (h->hash) hash_add(d,this);
}

void list::removeNode(node_data *r)
{	struct node *n1,*n2;

	for(n1=h->next,n2=NULL;n1;n2=n1,n1=n1->next)
	{	if (n1->data==r)
		{	if (h->last==n1)
				h->last=n2;

			if (n2==NULL)
			{	h->next=n1->next;
			}
			else
				n2->next=n1->next;
		}
	}
}

int list::len(void)
{	int sz;
	struct node *n;

	for(sz=0,n=h->next;n != NULL;sz++,n=n->next);

	return(sz);
}    
