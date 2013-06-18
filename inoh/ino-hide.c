#include "inoh/ino-hide.h"

bool ih_init(struct ino_hide * ih, const char * target_fname)
{
  if( ih == NULL )
    return false;

  ih->target_fname = NULL;
  ih->target_fd = -1;
  ih->inotify_fd = -1;
  ih->buf_events = NULL;
  ih->buf_events_len = 0;
  ih->worker_pid = -1;
  ih->fattr.st_mode = 0;
  ih->fattr.st_uid = 0;
  ih->fattr.st_gid = 0;

  return 
       ih_init_inotify(ih) 
    && ih_create_buf_events(ih)
    && ih_register_file(ih, target_fname);
}

bool ih_init_inotify(struct ino_hide * ih)
{
  int inotify_fd = inotify_init();

  if( inotify_fd == -1 )
  {
    print_error("inotify_init failed (%s)", strerror(errno));
    return false;
  }

  ih->inotify_fd = inotify_fd;

  return true;
}

bool ih_register_file(struct ino_hide * ih, const char * target_fname)
{
  if( ih == NULL || target_fname == NULL )
    return false;

  struct file_attr fattr;
  if( !get_file_attributes(&fattr, target_fname) )
    return false;

  // The file to be hidden must be writable, since
  // we will delete it (and restore it later)
  if(    is_writable_file(target_fname) 
      && is_regular_file(&fattr)        )
  {
    // dirname(char * path) may modify the contents of path,
    // so we create a copy here
    char * fname_copy = strdup(target_fname);
    char * dir = dirname(fname_copy);

    print_info("Adding watch for %s", dir);

    if( inotify_add_watch(ih->inotify_fd, dir, IN_OPEN|IN_EXCL_UNLINK) == -1 )
    {
      print_error("inotify_add_watch failed (%s)", strerror(errno));
      free(fname_copy);
      return false;
    }
    else
    {
      ih->target_fname = target_fname;
      ih->fattr = fattr;
      free(fname_copy);
      return true;
    }
  }
  else
  {
    print_error("File %s is neither a regular file nor writable", target_fname);
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
  
  // We only care about the first 2 events that happened
  // since the previous call to read. Events may get lost.
  const size_t max_events_per_read = 2;

  size_t buf_events_len =
      sizeof(struct inotify_event)
    + name_max
    + 1
  ;

  char * buf_events = (char *)calloc(max_events_per_read, buf_events_len);
  if( buf_events == NULL )
  {
    print_error("Failed to allocate %zd bytes\n", buf_events_len);
    return false;
  }

  ih->buf_events = buf_events;
  ih->buf_events_len = buf_events_len * max_events_per_read;

  return true;
}

bool ih_hide_file(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  int need_to_hide = -1;

  do
  {
    need_to_hide = ih_block_until_need_to_hide(ih);
    if( need_to_hide > 0 )
    {
      if( ih_worker_is_alive(ih) )
      {
        if( !ih_worker_restart_delay(ih) )
          return false;
      }
      else
      {
        if( !ih_worker_delayed_delete(ih) )
          return false;
      }
    }
  }
  while( need_to_hide != -1 );

  return false;
}

bool ih_open_file(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  int target_fd = open(ih->target_fname, O_RDONLY);

  if( target_fd == -1 )
  {
    print_error("Failed to open file %s (%s)", ih->target_fname, strerror(errno));
    return false;
  }

  ih->target_fd = target_fd;

  return true;
}

bool ih_delete_file(const struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  if( unlink(ih->target_fname) == -1 )
  {
    print_error("Failed unlinking file (%s)", strerror(errno));
    return false;
  }

  print_info("Unlinked %s from filesystem", ih->target_fname);

  return true;
}

bool ih_restore_file(struct ino_hide * ih)
{
  FILE * tmp = tmpfile();
  if( tmp == NULL )
  {
    print_error("Failed opening temporary file (%s)", strerror(errno));
    return false;
  }

  int tmp_fd = fileno(tmp);
  if( tmp_fd == -1 )
  {
    print_error("fileno failed (%s)", strerror(errno));
    return false;
  }

  if( !copy_fd(ih->target_fd, tmp_fd) )
    return false;

  if( close(ih->target_fd) == -1 )
    print_error("Close hiddenfile failed (%s)", strerror(errno));

  print_info("Recreating file");
  int new_fd = open(ih->target_fname, O_CREAT|O_RDWR|O_EXCL, ih->fattr.st_mode);
  if( new_fd == -1 )
  {
    print_error("Creating file %s failed (%s)", ih->target_fname, strerror(errno));
    return false;
  }

  set_file_ownership(new_fd, ih->fattr.st_uid, ih->fattr.st_gid);

  if( !rewind_fd(tmp_fd) )
    return false;
  if( !copy_fd(tmp_fd, new_fd) )
    return false;
  if( !rewind_fd(new_fd) )
    return false;

  if( close(tmp_fd) == -1 )
    print_error("Close tmpfile failed (%s)", strerror(errno));

  ih->target_fd = new_fd;

  return true;
}

int ih_block_until_need_to_hide(struct ino_hide * ih)
{
  if( ih == NULL )
    return -1;

  ssize_t bytes_read = 0;
  char * p_buf_events = NULL;
  struct inotify_event * event = NULL;
  bool need_to_hide = false;
  char my_buf[4096];

  ih->buf_events = &my_buf[0];
  bytes_read = read(ih->inotify_fd, ih->buf_events, ih->buf_events_len);

  if( bytes_read < 1 )
  {
    print_error("Reading inotify_fd failed (%s)", strerror(errno));
    return -1;
  }

  p_buf_events = ih->buf_events;
  while( p_buf_events < ih->buf_events + bytes_read )
  {
    // (struct inotify_event *)p_buf_events:
    // p_buf_events is type of char *
    // clang complains this cast changes alignment:
    // warning: cast from 'char *' to 'struct inotify_event *' increases
    //          required alignment from 1 to 4 [-Wcast-align]
    // (gcc gives no such warning with -Wcast-align)
    // But: p_buf_events is allocated with calloc, which is guaranteed to
    // give the widest possible alignment.
    event = (struct inotify_event *)p_buf_events;

    // event->len will only be 0 if the watched directory itself 
    // is subject to the event (filter out noise from the directory's 
    // children)
    need_to_hide |= ( event->mask & IN_ISDIR ) && event->len == 0;

    print_event(event);

    p_buf_events += sizeof(struct inotify_event) + event->len;
  }

  print_info("Need to hide: %d", need_to_hide);

  return (int)need_to_hide;
}

void ih_cleanup(struct ino_hide * ih)
{
  if( ih == NULL )
    return;

  if( ih->inotify_fd != -1 )
  {
    if( close(ih->inotify_fd) == -1 )
      print_error("Failed closing inotify_fd (%s)", strerror(errno));
    ih->inotify_fd = -1;
  }

  if( ih->buf_events != NULL )
  {
    free(ih->buf_events);
    ih->buf_events = NULL;
  }
}

