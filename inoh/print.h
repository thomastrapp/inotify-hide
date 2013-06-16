#ifndef INOH_PRINT_H
#define INOH_PRINT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/inotify.h>

void va_print_error(const char *, va_list);
void print_error(const char *, ...);
void va_print_info(const char *, va_list);
void print_info(const char *, ...);
void print_event(const struct inotify_event *);

#endif
