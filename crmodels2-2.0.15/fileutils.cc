#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <vector>

#include "fileutils.h"

#if !defined(HAVE_MKSTEMP) && defined(HAVE_MKSTEMPS)
extern "C" int mkstemps (char *pattern, int suffix_len); 
#endif

using namespace std;

vector<char *> tfiles;


FILE *open_write(char *f)
{	FILE *fp;

	fp=fopen(f,"w");
	if (fp==NULL)
	{	fprintf(stderr,"Unable to open \"%s\" for writing...\n",f);
		exit(1);
	}
	return(fp);
}

FILE *open_append(char *f)
{	FILE *fp;

	fp=fopen(f,"a");
	if (fp==NULL)
	{	fprintf(stderr,"Unable to open \"%s\" for append...\n",f);
		exit(1);
	}
	return(fp);
}

FILE *open_read(char *f)
{	FILE *fp;

	fp=fopen(f,"r");
	if (fp==NULL)
	{	fprintf(stderr,"Unable to open \"%s\" for reading...\n",f);
		exit(1);
	}
	return(fp);
}

void delete_temp(const char *fname)
{	for(int i=0;i<(int)tfiles.size();i++)
	{	if (strcmp(fname,tfiles[i])==0)
		{	char *s;

			tfiles.erase(tfiles.begin()+i);
			s=tfiles[i];
			free(s);
			break;
		}
	}

	unlink(fname);
}

void delete_all_temps(void)
{	for(int i=(int)tfiles.size()-1;i>=0;i--)
	{	char *s;

		unlink(tfiles[i]);
		s=tfiles[i];
		tfiles.erase(tfiles.begin()+i);
		free(s);
	}
}

char *get_tmp_dir(void)
{	char *tmpdir;
	static char *defdir="/tmp";
#ifdef __MINGW32__
	tmpdir=getenv("TEMP");
	if (tmpdir==NULL)
#endif
	tmpdir=defdir;

	return(tmpdir);
}

char *create_temp(const char *templ)
{	int h;
	FILE *fp;
	char *fname;
	char *tmpdir;

	tmpdir=get_tmp_dir();
	fname=(char *)calloc(strlen(templ)+strlen(tmpdir)+1+1,sizeof(char));
	//sprintf(fname,"%s/tmp.XXXXXX",tmpdir);
	sprintf(fname,"%s/%s",tmpdir,templ);
	//fname=strdup(templ);


#ifdef HAVE_MKSTEMP
	h=mkstemp(fname);
#else
#  ifdef HAVE_MKSTEMPS
        h=mkstemps(fname,0);
#  else
#    error "Either mkstemp() or mkstemps() must be available."
#  endif
#endif

	if (h<0)
	{	fprintf(stderr,"Unable to create temporary file from template \"%s\"...\n",templ);
		exit(1);
	}

	/* same as close(), but does not require include file unistd.h */
	fp=fdopen(h,"w");
	fclose(fp);

	tfiles.push_back(strdup(fname));

	return(fname);
}

void appendFile(FILE *fsource,FILE *fdest)
{	char s[FILECHUNK];
	int read;

	while(!feof(fsource) && ((read=fread(s,sizeof(char),FILECHUNK,fsource))>0))
		fwrite(s,sizeof(char),read,fdest);
}

void appendFile(FILE *fsource,char *dest)
{	FILE *fpdest;

	fpdest=open_append(dest);
	appendFile(fsource,fpdest);
	fclose(fpdest);
}

void appendFile(char *source,FILE *fdest)
{	FILE *fp;

	fp=open_read(source);
	appendFile(fp,fdest);
	fclose(fp);
}

void appendFile(char *source,char *dest)
{	FILE *fpdest;

	fpdest=open_append(dest);
	appendFile(source,fpdest);
	fclose(fpdest);
}

void catFile(char *file,FILE *fpo)
{	appendFile(file,fpo);
}
