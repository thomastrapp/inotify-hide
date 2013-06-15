#ifndef INOH_INO_HIDE_WORKER_H
#define INOH_INO_HIDE_WORKER_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "inoh/file.h"
#include "inoh/print.h"
#include "inoh/ino-hide.h"

bool ih_worker_is_alive(const struct ino_hide *);
bool ih_worker_restart_delay(const struct ino_hide *);
bool ih_worker_delayed_delete(struct ino_hide *);
bool ih_worker_block_signals(void);
bool ih_worker_set_sigset(sigset_t *);
bool ih_worker_delay(void);

#endif

