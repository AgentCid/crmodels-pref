#include <string.h>

#include "stringutils.h"

int startsWith(char *str,const char *beg)
{	return(strncmp(str,beg,strlen(beg))==0);
}

int endsWith(char *str,char *end)
{	return(strncmp(&str[strlen(str)-strlen(end)],end,strlen(end))==0);
}
