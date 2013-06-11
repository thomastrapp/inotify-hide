#ifndef INOH_INO_HIDE_H
#define INOH_INO_HIDE_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/inotify.h>

#include "inoh/file.h"
#include "inoh/print.h"

struct ino_hide 
{
  const char * target_fname;
  int target_fd;
  int inotify_fd;
  char * buf_events;
  size_t buf_events_len;
};

bool ih_init(struct ino_hide *);
bool ih_set_file(struct ino_hide *, const char *);
bool ih_create_buf_events(struct ino_hide *);
bool ih_loop_events(struct ino_hide *);
void ih_print_event(struct inotify_event *);
void ih_cleanup(struct ino_hide *);

#endif
