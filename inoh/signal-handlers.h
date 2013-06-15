#ifndef INOH_SIGNAL_HANDLERS_H
#define INOH_SIGNAL_HANDLERS_H

#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>

#include "inoh/print.h"

void sig_interrupt_handler(int);
bool sig_register_interrupt_handler(void);

#endif

