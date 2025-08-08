#pragma once

#include <core/types.h>

long sleep(long);

int park(void);
int unpark(tid_t);