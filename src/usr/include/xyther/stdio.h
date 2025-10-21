#pragma once

#include "printf.h"

#define __stdin_fileno  0
#define __stdout_fileno 1
#define __stderr_fileno 2

extern long __std_in(char *buf, unsigned long len);
extern long __std_out(char *buf, unsigned long len);
extern long __std_err(char *buf, unsigned long len);

extern void __clear_stdio(void);
extern int  __open_stdio(void);
extern void __close_stdio(void);