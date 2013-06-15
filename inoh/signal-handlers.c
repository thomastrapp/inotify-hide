#include "inoh/signal-handlers.h"

void sig_interrupt_handler(int signal)
{
  // Tell gcc we don't care that paramter "int signal" is
  // not used in this function 
  (void)signal;

  // Don't mess up global variable errno in this 
  // signal handler (try to prevent race condition)
  int errno_old = errno;

  wait(/* int * stat_loc */ NULL);

  errno = errno_old;
}

bool sig_register_interrupt_handler(void)
{
  if( signal(SIGTERM, sig_interrupt_handler) == SIG_ERR )
  {
    print_error("Failed registering interrupt handler");
    return false;
  }

  if( signal(SIGINT, sig_interrupt_handler) == SIG_ERR )
  {
    print_error("Failed registering interrupt handler");
    return false;
  }

  return true;
}

