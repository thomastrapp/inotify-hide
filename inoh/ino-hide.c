#include "inoh/ino-hide.h"

bool ih_init(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  ih->target_fname = NULL;
  ih->target_fd = -1;
  ih->inotify_fd = -1;
  ih->buf_events = NULL;
  ih->buf_events_len = 0;

  int inotify_fd = inotify_init();

  if( inotify_fd == -1 )
  {
    print_error("inotify_init failed (%s)", strerror(errno));
    return false;
  }

  ih->inotify_fd = inotify_fd;

  return true;
}

bool ih_set_file(struct ino_hide * ih, const char * target_fname)
{
  if( ih == NULL || target_fname == NULL )
    return false;

  if( is_writable_file(target_fname) && 
      is_regular_file(target_fname)     )
  {
    if( inotify_add_watch(ih->inotify_fd, target_fname, IN_ALL_EVENTS) == -1 )
    {
      print_error("inotify_add_watch failed (%s)", strerror(errno));
    }
    else
    {
      ih->target_fname = target_fname;
      return true;
    }
  }
  else
  {
    print_error("File %s is either not a regular file or not writable", target_fname);
  }

  return false;
}

bool ih_create_buf_events(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  // struct inotify_event may contain (optionally) the name
  // of the file that is subject to the event. To satisfy 
  // the arbitrary size of the filename, we wastefully allocate
  // enough space to hold any filename length possible on this 
  // system (_PC_NAME_MAX).
  size_t name_max = get_max_name_len();
  
  // We only care about the first 10 events that happened  
  // since the previous call to read. Events may get lost.
  int max_events_per_read = 10;

  size_t buf_events_len = 
      max_events_per_read 
    * sizeof(struct inotify_event) 
    + name_max 
    + 1
  ;

  char * buf_events = (char *)calloc(1, buf_events_len);
  if( buf_events == NULL )
  {
    print_error("Failed to allocate %zd bytes\n", buf_events_len);
    return false;
  }

  ih->buf_events = buf_events;
  ih->buf_events_len = buf_events_len;

  return true;
}

bool ih_loop_events(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  ssize_t bytes_read = 0;
  char * p_buf_events = NULL;
  struct inotify_event * event = NULL;
  while( true )
  {
    bytes_read = read(ih->inotify_fd, ih->buf_events, ih->buf_events_len);

    if( bytes_read < 1 )
    {
      print_error("read inotify_fd failed (%s)", strerror(errno));
      return false;
    }

    p_buf_events = ih->buf_events;
    while( p_buf_events < ih->buf_events + bytes_read )
    {
      event = (struct inotify_event *) p_buf_events;
      ih_print_event(event);

      p_buf_events += sizeof(struct inotify_event) + event->len;
    }
  }

  return true;
}

void ih_print_event(struct inotify_event * i)
{
  if( i == NULL )
    return;

  printf("Event ");
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
}

void ih_cleanup(struct ino_hide * ih)
{
  if( ih == NULL )
    return;

  if( ih->inotify_fd != -1 )
  {
    close(ih->inotify_fd);
    ih->inotify_fd = -1;
  }

  if( ih->target_fd != -1 )
  {
    close(ih->target_fd);
    ih->target_fd = -1;
  }

  if( ih->buf_events != NULL )
  {
    free(ih->buf_events);
    ih->buf_events = NULL;
  }
}

