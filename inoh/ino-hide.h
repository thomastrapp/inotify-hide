#ifndef INOH_INO_HIDE_H
#define INOH_INO_HIDE_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "inoh/file.h"
#include "inoh/print.h"

struct ino_hide 
{
  const char * target_fname;
  int target_fd;
  int inotify_fd;
  char * buf_events;
  size_t buf_events_len;

  // child worker
  pid_t worker_pid;

  struct file_attr fattr;
};

bool ih_init(struct ino_hide *, const char *);
bool ih_init_inotify(struct ino_hide *);
bool ih_register_file(struct ino_hide *, const char *);
bool ih_create_buf_events(struct ino_hide *);
bool ih_hide_file(struct ino_hide *);
bool ih_worker_is_alive(struct ino_hide *);
bool ih_worker_restart_delay(struct ino_hide *);
bool ih_worker_delayed_delete(struct ino_hide *);
bool ih_worker_block_signals(void);
bool ih_worker_set_sigset(sigset_t *);
bool ih_worker_delay(void);
bool ih_open_target_file(struct ino_hide *);
bool ih_delete_file(struct ino_hide *);
bool ih_restore_file(struct ino_hide *);
int ih_block_until_need_to_hide(struct ino_hide *);
void ih_cleanup(struct ino_hide *);

#endif
