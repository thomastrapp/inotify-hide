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

void va_print_info(const char * format, va_list args)
{
  if( format == NULL )
    return;

  fprintf(stdout, " * ");
  vfprintf(stdout, format, args);
  fprintf(stdout, "\n");
}

void print_info(const char * format, ...)
{
  if( format == NULL )
    return;

  va_list args;
  va_start(args, format);
  va_print_info(format, args);
  va_end(args);
}

void print_event(struct inotify_event * i)
{
  if( i == NULL )
    return;

  printf(" * Event ");
  if (i->mask & IN_ACCESS)        printf("IN_ACCESS ");
  if (i->mask & IN_ATTRIB)        printf("IN_ATTRIB ");
  if (i->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
  if (i->mask & IN_CLOSE_WRITE)   printf("IN_CLOSE_WRITE ");
  if (i->mask & IN_CREATE)        printf("IN_CREATE ");
  if (i->mask & IN_DELETE)        printf("IN_DELETE ");
  if (i->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
  if (i->mask & IN_IGNORED)       printf("IN_IGNORED ");
  if (i->mask & IN_ISDIR)         printf("IN_ISDIR ");
  if (i->mask & IN_MODIFY)        printf("IN_MODIFY ");
  if (i->mask & IN_MOVE_SELF)     printf("IN_MOVE_SELF ");
  if (i->mask & IN_MOVED_FROM)    printf("IN_MOVED_FROM ");
  if (i->mask & IN_MOVED_TO)      printf("IN_MOVED_TO ");
  if (i->mask & IN_OPEN)          printf("IN_OPEN ");
  if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW ");
  if (i->mask & IN_UNMOUNT)       printf("IN_UNMOUNT ");
  printf("\n");
}

