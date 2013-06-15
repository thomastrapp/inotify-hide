#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inoh/print.h"
#include "inoh/ino-hide.h"

// Todo: 
// - restore file atexit if in limbo
// - register ctrl c to call atexit
// Todo fancy:
// - options: --verbose (print info, default be silent)
int main(int argc, char ** argv)
{
  if( argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 )
  {
    print_error("Usage: %s file", argv[0]);
    exit(EXIT_FAILURE);
  }

  struct ino_hide ih;

  if( !ih_init(&ih, argv[1]) )
  {
    exit(EXIT_FAILURE);
  }

  if( !ih_hide_file(&ih) )
  {
    exit(EXIT_FAILURE);
  }

  if( ih_worker_is_alive(&ih) )
  {
    kill(ih.worker_pid, SIGUSR1);
    wait(/* int * stat_loc */ NULL);
  }

  ih_cleanup(&ih);

  return EXIT_SUCCESS;
}

