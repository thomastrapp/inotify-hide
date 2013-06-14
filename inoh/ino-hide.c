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
  ih->st_mode = -1;
  ih->st_uid = -1;
  ih->st_gid = -1;

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

  // The field to be hidden must be writable, since
  // we will delete it (and restore it later)
  if( is_writable_file(target_fname) && 
      is_regular_file(target_fname)     )
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
      free(fname_copy);
      return ih_save_file_permissions(ih);
    }
  }
  else
  {
    print_error("File %s is either not a regular file or not writable", target_fname);
  }

  return false;
}

bool ih_save_file_permissions(struct ino_hide * ih)
{
  struct stat buf_stat;

  if( stat(ih->target_fname, &buf_stat) != 0 )
  {
    print_error("stat %s failed (%s)", ih->target_fname, strerror(errno));
    return false;
  }

  ih->st_mode = buf_stat.st_mode;
  ih->st_uid = buf_stat.st_uid;
  ih->st_gid = buf_stat.st_gid;

  return true;
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

bool ih_loop_delete_restore_file_on_event(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  if( !ih_open_target_file(ih) )
    return false;

  while( true )
  {
    if( ih_block_until_open(ih) )
    {
      if( !ih_delete_file_and_wait(ih) )
      {
        return false;
      }

      if( !ih_restore_file(ih) )
      {
        return false;
      }
    }
  }

  return true;
}

bool ih_open_target_file(struct ino_hide * ih)
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
  print_info("Opened file %s", ih->target_fname);

  return true;
}

bool ih_delete_file_and_wait(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  if( unlink(ih->target_fname) == -1 )
  {
    print_error("Failed unlinking file (%s)", strerror(errno));
    return false;
  }

  print_info("File %s unlinked, sleeping", ih->target_fname);
  sleep(2);
  print_info("Woke up");

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

  print_info("Copying hiddenfile to temporary");
  if( !copy_fd(ih->target_fd, tmp_fd) )
  {
    return false;
  }

  if( close(ih->target_fd) == -1 )
  {
    print_error("Close hiddenfile failed (%s)", strerror(errno));
  }

  print_info("Recreating file");
  int new_fd = open(ih->target_fname, O_CREAT|O_RDWR|O_EXCL, ih->st_mode);
  if( new_fd == -1 )
  {
    print_error("Creating file %s failed (%s)", ih->target_fname, strerror(errno));
    return false;
  }

  ih_restore_file_ownership(new_fd, ih);

  if( lseek(tmp_fd, 0, SEEK_SET) != 0 )
  {
    print_error("Failed seeking to offset 0 on temporary (%s)", strerror(errno));
    return false;
  }

  if( !copy_fd(tmp_fd, new_fd) )
  {
    return false;
  }

  if( close(tmp_fd) == -1 )
  {
    print_error("Close tmpfile failed (%s)", strerror(errno));
  }

  if( lseek(new_fd, 0, SEEK_SET) != 0 )
  {
    print_error("Failed seeking to offset 0 on hiddenfile (%s)", strerror(errno));
    return false;
  }

  ih->target_fd = new_fd;

  return true;
}

bool ih_restore_file_ownership(int target_fd, struct ino_hide * ih)
{
  if( fchown(target_fd, ih->st_uid, ih->st_gid) == -1 )
  {
    print_error("Failed restoring file ownership (%s)", strerror(errno));
    return false;
  }

  return true;
}

bool ih_block_until_open(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  ssize_t bytes_read = 0;
  char * p_buf_events = NULL;
  struct inotify_event * event = NULL;
  bool need_to_hide = false;

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

    // event->len will only be 0 if the watched directory itself 
    // is subject to the event (filter out noise from the directory's 
    // children)
    need_to_hide |= ( event->mask & IN_ISDIR ) && event->len == 0;

    print_event(event);
    print_info("Need to hide: %d", need_to_hide);

    p_buf_events += sizeof(struct inotify_event) + event->len;
  }

  return need_to_hide;
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

