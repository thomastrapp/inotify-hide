#include "inoh/print.h"

void va_print_error(const char * format, va_list args)
{
  if( format == NULL )
    return;

  fprintf(stderr, "Error: ");
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
}

void print_error(const char * format, ...)
{
  if( format == NULL )
    return;

  va_list args;
  va_start(args, format);
  va_print_error(format, args);
  va_end(args);
}

