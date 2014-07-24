#ifndef CRMODELS_KEYWORDS_H
#define CRMODELS_KEYWORDS_H

/* Relations */
#define CRNAME "cr__crname"
#define BODYTRUE "cr__bodytrue"
#define APPLCR "cr__applcr"
#define STOP "cr__stop"
#define IS_PREF "cr__is_preferred"
#define APPL "cr__appl"
#define APPLX "cr__applx"
#define PREFER2_REL "cr__prefer2_relation"
#define IS_PREF2 "cr__is_preferred2"

#define PREFER "prefer"
#define PREFER2 "prefer2"

#define NOT_PREFER "not prefer"
#define NOT_EQ "!="

/*
struct relinfo
{	char *name;
	int arity;
};

struct relinfo cr_rels[]=
{	{ (char *)CRNAME, 1 },
	{ (char *)BODYTRUE, 1 },
	{ (char *)APPLCR, 1 },
	{ (char *)STOP, 0 },
	{ (char *)IS_PREF, 2 },
	{ (char *)APPL, 1 },
	{ (char *)APPLX, 1 },
	{ (char *)PREFER2_REL, 1 },
	{ (char *)IS_PREF2, 2 },
	{ (char *)"", -1 }
};
*/

/* Variables */
#define INTERNAL "R_internal"
#define INTERNAL1 "R_internal1"
#define INTERNAL2 "R_internal2"
#define INTERNAL3 "R_internal3"


#endif /* CRMODELS_KEYWORDS_H */
