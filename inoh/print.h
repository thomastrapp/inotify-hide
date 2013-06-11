#ifndef INOH_PRINT_H
#define INOH_PRINT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

void va_print_error(const char *, va_list);
void print_error(const char *, ...);

#endif
