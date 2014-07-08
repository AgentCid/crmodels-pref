#ifndef CRMODELS_LOADER_H
#define CRMODELS_LOADER_H

#include "atom_store.h"
#include "extended_data.h"
#include "program.h"

class loader
{
private:
	void store_literal(char *line, int &number_of_crnames);
	void update_atomcount(int idx);
	void add_extra_atoms(void);
	void store_rule(char *line);
	int check_for_newline(char *buffer,char *&last_line_beg,int &zerocount,int &i, int &number_of_crnames);
	int check_for_lastchar(char *buffer,int &i);
	int copybuffer(char *src, char *dest);
	void eliminate_cr_stop_rule(void);

	program *P;

public:
	atom_store *store;

	int load_program(char *file,program *P_);
};

#endif /* CRMODELS_LOADER_H */
