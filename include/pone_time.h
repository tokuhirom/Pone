#ifndef PONE_TIME_H_
#define PONE_TIME_H_

#include "pone.h"
#include <sys/time.h>

pone_val* pone_time_from_epoch(pone_world* world, time_t sec, pone_int_t usec);

#endif
