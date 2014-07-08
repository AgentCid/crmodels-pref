#include "atom_store.h"

void atom_store::set(enum STORE atom,int idx)
{	indexes[atom]=idx;
}
	
int atom_store::get(enum STORE atom)
{	return(indexes[atom]);
}

void atom_store::set_available(int idx)
{	available=idx;
}

int atom_store::get_available(void)
{	return(available);
}
