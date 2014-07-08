#include <stdio.h>
#include <stdlib.h>

#include "keywords.h"

#include "generator.h"

#include "fileutils.h"


generator::generator()
{	skipLowerBound=false;
}

/* To allow re-use of this function, we allow to enable and disable
 * the generation of the constraint rule for the lower bound using
 * flag CreateGenerator_skipLowerBound.
 *
 * If the lower bound is enabled, the generator looks for solutions
 * with EXACTLY j cr-rules.
 *
 * If the lower bound is disabled, the generator looks for solutions
 * with AT MOST j cr-rules.
 */
int generator::output(int j,bool *removed_atoms)
{	FILE *fp;
	struct node *n;
	int applcrnum =0, k;


	fp=open_write(file);

	if (applcrlist->first()==NULL)	/* no cr-rules -> just create empty generator */
	{	fclose(fp);
		return(0);		/* return 0 cr-rules available */
	}


	for(n=applcrlist->first();n!=NULL && BASICDATA(n)->value!=NULL;n=n->next)
	{	if (!removed_atoms[BASICDATA(n)->value1])
			applcrnum++;
	}
      
	// fprintf(stderr,"THE NUMBER OF APPLCR ATOMS  %d\n",applcrnum);


	/* To allow re-use of this function, we allow to enable and disable
	 * the generation of the constraint rule for the lower bound.
	 *
	 * If the lower bound is enabled, the generator looks for solutions
	 * with EXACTLY j cr-rules.
	 *
	 * If the lower bound is disabled, the generator looks for solutions
	 * with AT MOST j cr-rules.
	 */
	if (!skipLowerBound)
	{
		// for rule ok1 :- lower {applcr ... applcr }
		fprintf(fp,"2 ");
		fprintf(fp,"%d ",store->get(store->OK1));
		fprintf(fp,"%d ",applcrnum);
		fprintf(fp,"0 ");
		fprintf(fp,"%d ",(j>applcrnum) ? applcrnum:j);

		n = applcrlist->first();
		//changed here
		while(n != NULL && BASICDATA(n)->value != NULL)
		{	if (!removed_atoms[BASICDATA(n)->value1])
				fprintf(fp,"%d ",BASICDATA(n)->value1);
			n = n->next;
		}
      
		fprintf(fp,"\n");
	}
	else
		fprintf(fp,"1 %d 0 0\n",store->get(store->OK1));


	//for rule bad :- upper +1{applcr ...applcr}
	k = j+1;
	if (k <=  applcrnum)
	{	fprintf(fp,"2 ");
		fprintf(fp,"%d ",store->get(store->BAD));
		fprintf(fp,"%d ",applcrnum);
		fprintf(fp,"0 ");
		fprintf(fp,"%d ",j+1);
      
		n = applcrlist->first();
	
		//changed here
		while (n != NULL && BASICDATA(n)->value != NULL)
		{	if (!removed_atoms[BASICDATA(n)->value1])
				fprintf(fp,"%d ",BASICDATA(n)->value1);
			n = n->next;
		}
		fprintf(fp,"\n");
	}
	// for rule ok :- ok1, not bad
	
	fprintf(fp,"1 ");
	fprintf(fp,"%d ",store->get(store->OK));
	fprintf(fp,"2 ");
	fprintf(fp,"1 ");
	fprintf(fp,"%d ",store->get(store->BAD));
	fprintf(fp,"%d ",store->get(store->OK1));
	fprintf(fp,"\n");
	
	//for rule :- not ok
	
	fprintf(fp,"1 ");
	fprintf(fp,"1 ");
	fprintf(fp,"1 ");
	fprintf(fp,"1 ");
	fprintf(fp,"%d ",store->get(store->OK));
	fprintf(fp,"\n");


/********************************/
	int i=allatomlist->find_atomnumber(STOP);
	if (i!=0)
		fprintf(fp,"1 %d 0 0\n",i);

	if (applcrnum>0)
	{	fprintf(fp,"3 ");
		fprintf(fp,"%d ",applcrnum);
		for(n = applcrlist->first();n != NULL && BASICDATA(n)->value != NULL;n = n->next)
		{	if (!removed_atoms[BASICDATA(n)->value1])
				fprintf(fp,"%d ",BASICDATA(n)->value1);
		}
		fprintf(fp,"0 0\n");
	}



	fclose(fp);

	return(applcrnum);
}
