#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/inotify.h>

void va_print_error(const char * format, va_list args)
{
  fprintf(stderr, "Error: ");
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
}

void print_error(const char * format, ...)
{
  va_list args;
  va_start(args, format);
  va_print_error(format, args);
  va_end(args);
}

bool is_writable_file(const char * file)
{
  return access(file, W_OK) == 0;
}

bool is_regular_file(const char * file)
{
  struct stat buf_stat;

  if( stat(file, &buf_stat) != 0 )
  {
    print_error("stat %s failed (%s)", file, strerror(errno));
    return false;
  }

  return S_ISREG(buf_stat.st_mode);
}

void             /* Display information from inotify_event structure */
displayInotifyEvent(struct inotify_event *i)
{
    printf("    wd =%2d; ", i->wd);
    if (i->cookie > 0)
        printf("cookie =%4d; ", i->cookie);

    printf("mask = ");
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

    if (i->len > 0)
        printf("        name = %s\n", i->name);
}

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

  int inotify_fd = inotify_init();
  if( inotify_fd == -1 )
  {
    print_error("inotify_init failed (%s)", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if( is_writable_file(argv[1]) && 
      is_regular_file(argv[1])     )
  {
    if( inotify_add_watch(inotify_fd, argv[1], IN_ALL_EVENTS) == -1 )
    {
      print_error("inotify_add_watch failed (%s)", strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    print_error("File %s is either not a regular file or not writable", argv[1]);
    exit(EXIT_FAILURE);
  }

  // struct inotify_event may contain (optionally) the name
  // of the file that is subject to the event. To satisfy 
  // the arbitrary size of the filename, we lazily allocate 
  // enough space to hold any filename length possible on this 
  // system (_PC_NAME_MAX).
  
  // pathconf won't set errno if there is no limit
  errno = 0;

  // truncate long to ssize_t
  ssize_t name_max = (ssize_t)pathconf(".", _PC_NAME_MAX);
  if( name_max < 1 )
  {
    if( errno )
    {
      print_error("pathconf failed (%s)", strerror(errno)); 
    }

    name_max = 511;
    print_error("Maximum filename length is unknown, using %ld", name_max);
  }

  // We only care about the first 10 events that happened  
  // since the previous call to read. Events may get lost.
  size_t buf_events_len = 10 * sizeof(struct inotify_event) + name_max + 1;

  char * buf_events = (char *)calloc(1, buf_events_len);
  if( buf_events == NULL )
  {
    print_error("Failed to allocate %zd bytes\n", buf_events_len);
    exit(EXIT_FAILURE);
  }

  ssize_t bytes_read = 0;
  char * p_buf_events = NULL;
  struct inotify_event * event = NULL;
  while( true )
  {
    bytes_read = read(inotify_fd, buf_events, buf_events_len);

    if( bytes_read < 1 )
    {
      print_error("read inotify_fd failed (%s)", strerror(errno));
      exit(EXIT_FAILURE);
    }

    p_buf_events = buf_events;
    while( p_buf_events < buf_events + bytes_read )
    {
      event = (struct inotify_event *) p_buf_events;
      displayInotifyEvent(event);

      p_buf_events += sizeof(struct inotify_event) + event->len;
    }
  }

  close(inotify_fd);
  free(buf_events);

  return EXIT_SUCCESS;
}

