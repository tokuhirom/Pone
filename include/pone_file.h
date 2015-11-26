#ifndef PONE_FILE_H_
#define PONE_FILE_H_

#include <stdio.h>
#include "pone.h"

pone_val* pone_file_new(pone_world* world, FILE* fh, bool auto_close);
FILE* pone_file_fp(pone_world* world, pone_val* file);

#endif
