#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "model.h"
#include "stringutils.h"

void model::output_in_line(void)
{	struct node *n;
 
	for(n=mlist.first();n!=NULL && BASICDATA(n)->value != NULL;n=n->next)
	{	if(!atom_utils::is_hidden_atom(BASICDATA(n)->value))
			fprintf(stdout,"%s ",BASICDATA(n)->value);
	}
}

void model::output_mkatoms(bool aflag)
{	struct node *n;
 
	for(n=mlist.first();n!=NULL && BASICDATA(n)->value != NULL;n=n->next)
	{	if(!atom_utils::is_hidden_atom(BASICDATA(n)->value))
			fprintf(stdout,"%s%s\n",
				BASICDATA(n)->value,
				(aflag ? ".":""));
	}
	if (aflag)
		fprintf(stdout,"%%endmodel\n");
	else
		fprintf(stdout,"::endmodel\n");
}


/*-------------------------------------*/


int atom_utils::three_underscore_prefix(char *str)
{	return((str[0]=='_') && (str[1]=='_') && (str[2]=='_'));
}


int atom_utils::not_cratom(char *str)
{	if (three_underscore_prefix(str))
		str+=3;

	if(!startsWith(str,"cr__"))
		return(1);

	return(0);
}

bool atom_utils::is_hidden_atom(char *str)
{	if (not_cratom(str) && (!startsWith(str,"_")))
		return(false);
	return(true);
}
