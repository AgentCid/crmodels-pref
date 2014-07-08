#ifndef CRMODELS_ATOM_STORE_H
#define CRMODELS_ATOM_STORE_H

class atom_store
{
public:
	enum STORE
	{	OK=0,
		OK1,
		BAD,
		TRUE,
		DOMINATE,
	
		total
	};

private:
	int indexes[total];
	int available;

public:

	void set(enum STORE atom,int idx);
	int get(enum STORE atom);

	void set_available(int idx);
	int get_available(void);
};


#endif /* CRMODELS_ATOM_STORE_H */
