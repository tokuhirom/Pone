#ifndef PONE_MODULE_H_
#define PONE_MODULE_H_

#include "pone.h"

void pone_module_put(pone_world* world, pone_val* self, const char* name, pone_val* val);
pone_val* pone_module_new(pone_world* world, const char* name);
pone_val* pone_module_from_lex(pone_world* world, const char* module_name);

#endif // PONE_MODULE_H_
