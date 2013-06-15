#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inoh/print.h"
#include "inoh/ino-hide.h"

// Todo: 
// - save and restore file permissions (stat)
// - print_info
// - restore file atexit if in limbo
// - register ctrl c to call atexit
// - restore file in child process? send signal if new event 
//   arrived while in sleep time, to restart sleep time.
//   Send signal to restore immediately (and join) => shutdown.
//   See sigtimedwait!
// Todo fancy:
// - options: --verbose (print info, default be silent)
// - nonblocking read of notify events (O_NONBLOCK, fork or pthreads)?
int main(int argc, char ** argv)
{
  if( argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 )
  {
    print_error("Usage: %s file", argv[0]);
    exit(EXIT_FAILURE);
  }

  struct ino_hide ih;

  if( !ih_init(&ih) )
  {
    exit(EXIT_FAILURE);
  }

  if( !ih_set_file(&ih, argv[1]) )
  {
    exit(EXIT_FAILURE);
  }

  if( !ih_create_buf_events(&ih) )
  {
    exit(EXIT_FAILURE);
  }

  if( !ih_loop_delete_restore_file_on_event(&ih) )
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

