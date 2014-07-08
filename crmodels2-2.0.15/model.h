#ifndef CRMODELS_MODEL_H
#define CRMODELS_MODEL_H

#include "extended_data.h"

class model
{
public:
	basic_list mlist;
	basic_list appllist;
//	basic_list mmlist;

	void output_in_line(void);
	void output_mkatoms(bool aflag);
};

class atom_utils
{
public:
	static int three_underscore_prefix(char *str);
	static int not_cratom(char *str);
	static bool is_hidden_atom(char *str);
};


#endif /* CRMODELS_MODEL_H */
