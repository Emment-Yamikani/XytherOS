#pragma once

#include "printf.h"

extern int __stdin_fileno, __stdout_fileno, __stderr_fileno;

extern long __std_in(char *buf, unsigned long len);
extern long __std_out(char *buf, unsigned long len);
extern long __std_err(char *buf, unsigned long len);