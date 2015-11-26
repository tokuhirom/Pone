#ifndef PONE_COMPILE_H_
#define PONE_COMPILE_H_

#include "pone.h"
#include "pone_module.h"
#include "pvip.h"

struct pone_compile_ctx;

void pone_compile_c(pone_world* world, const char* so_filename, const char* c_filename);
pone_val* pone_compile_fp(pone_world* world, FILE* srcfp, const char* filename);
pone_val* pone_compile_str(pone_world* world, const char* src);

pone_node* pone_parse_string(pone_world* world, pvip_t* pvip, const char* src, bool yy_debug);
pone_node* pone_parse_fp(pone_world* world, pvip_t* pvip, FILE* fp, bool yy_debug);

#endif // PONE_COMPILE_H_
