#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inoh/print.h"
#include "inoh/signal_handlers.h"
#include "inoh/ino-hide.h"

// Todo fancy:
// - options: --verbose (print info, default be silent)
int main(int argc, char ** argv)
{
  if( argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 )
  {
    print_error("Usage: %s file", argv[0]);
    return EXIT_FAILURE;
  }

  if( !sig_register_interrupt_handler() )
    return EXIT_FAILURE;

  struct ino_hide ih;

  if( !ih_init(&ih, argv[1]) )
  {
    ih_cleanup(&ih);
    return EXIT_FAILURE;
  }

  ih_hide_file(&ih);

  if( ih_worker_is_alive(&ih) )
  {
    kill(ih.worker_pid, SIGUSR1);
    wait(/* int * stat_loc */ NULL);
  }

  ih_cleanup(&ih);

  return EXIT_FAILURE;
}

