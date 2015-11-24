#ifndef PONE_TMPFILE_H_
#define PONE_TMPFILE_H_

pone_val* pone_tmpfile_new(pone_world* world);
const char* pone_tmpfile_path_c(pone_world* world, pone_val* self);
FILE* pone_tmpfile_fp(pone_world* world, pone_val* self);

#endif // PONE_TMPFILE_H_
