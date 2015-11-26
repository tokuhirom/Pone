#ifndef PONE_EXC_H_
#define PONE_EXC_H_

#include "pone.h"

jmp_buf* pone_exc_handler_push(pone_world* world);
void pone_exc_handler_pop(pone_world* world);
void pone_throw(pone_world* world, pone_val* msg);
void pone_throw_str(pone_world* world, const char* fmt, ...);
pone_val* pone_exc_class_new_simple(pone_world* world, const char* name, pone_int_t name_len, const char* message);
void pone_warn_str(pone_world* world, const char* fmt, ...);

#endif
