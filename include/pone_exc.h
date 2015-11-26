#ifndef PONE_EXC_H_
#define PONE_EXC_H_

#include "pone.h"

jmp_buf* pone_exc_handler_push(pone_world* world);
void pone_exc_handler_pop(pone_world* world);
void pone_throw(pone_world* world, pone_val* msg);
void pone_throw_str(pone_world* world, const char* fmt, ...);
void pone_warn_str(pone_world* world, const char* fmt, ...);

#endif
