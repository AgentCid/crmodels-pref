#ifndef CRMODELS_FILEUTILS_H
#define CRMODELS_FILEUTILS_H

#include <stdio.h>

#define FILECHUNK 1024000

FILE *open_write(char *f);
FILE *open_append(char *f);
FILE *open_read(char *f);
char *get_tmp_dir(void);
char *create_temp(const char *templ);
void delete_temp(const char *fname);
void delete_all_temps(void);

void appendFile(FILE *fsource,FILE *fdest);
void appendFile(FILE *fsource,char *dest);
void appendFile(char *source,FILE *fdest);
void appendFile(char *source,char *dest);

void catFile(char *file,FILE *fpo);

#endif /* CRMODELS_FILEUTILS_H */
