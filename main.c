#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inoh/print.h"
#include "inoh/ino-hide.h"

// Todo: 
// 1. add watches for dirname(file): dir IN_OPEN
// 2. open file, keep handle
// 3. watch is triggered
// 4. delete file (inode will still be there)
// 5. Wait some time 
// 6. restore file (copy fd to filename)
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

  if( !ih_loop_events(&ih) )
  {
    exit(EXIT_FAILURE);
  }

  ih_cleanup(&ih);

  return EXIT_SUCCESS;
}

