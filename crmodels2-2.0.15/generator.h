#ifndef CRMODELS_GENERATOR_H
#define CRMODELS_GENERATOR_H

#include "atom_store.h"
#include "extended_data.h"

class generator
{
public:

	char *file;

	basic_list *applcrlist;
	basic_list *allatomlist; /* or simply store the index of cr__stop */

	atom_store *store;


	int skipLowerBound;

	generator();

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
	int output(int j,bool *removed_atoms);
};

#endif /* CRMODELS_GENERATOR_H */
