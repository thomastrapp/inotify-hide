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
  ih->perm.st_mode = -1;
  ih->perm.st_uid = -1;
  ih->perm.st_gid = -1;

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

  struct permission perm = {0};
  if( !get_file_permissions(&perm, target_fname) )
    return false;

  // The file to be hidden must be writable, since
  // we will delete it (and restore it later)
  if(    is_writable_file(target_fname) 
      && is_regular_file(&perm)         )
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
      ih->perm = perm;
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

bool ih_hide_file(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  while( true )
  {
    if( ih_block_until_need_to_hide(ih) )
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

  return true;
}

bool ih_worker_is_alive(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;
 
  if( ih->worker_pid < 0  )
  {
    return false;
  }
  
  int status = 0;
  pid_t childpid = waitpid(ih->worker_pid, &status, WNOHANG);
  if( childpid == -1 )
  {
    print_error("Failed querying child status (%s)", strerror(errno));
    return false;
  }

  if( childpid == 0 )
  {
    return true;
  }

  if( WIFEXITED(status) )
  {
    print_info("Child exited with status %d", WEXITSTATUS(status));
  }
  else if( WIFSIGNALED(status) )
  {
    print_info("Child was terminated by signal %d", WTERMSIG(status));
  }

  return false;
}

bool ih_worker_restart_delay(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  if( kill(ih->worker_pid, SIGUSR2) == -1 )
  {
    print_error("Failed sending signal SIGUSR2 to worker process (%s)", strerror(errno));
    return false;
  }

  return true;
}

bool ih_worker_delayed_delete(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  pid_t worker_pid = fork();
  if( worker_pid == -1 )
  {
    print_error("Failed forking worker (%s)", strerror(errno));
    return false;
  }

  if( worker_pid == 0 )
  {
    // in child
    print_info("Forked worker");

    if( !ih_worker_block_sigusr() )
      _exit(EXIT_FAILURE);

    if( !ih_open_target_file(ih) )
      _exit(EXIT_FAILURE);

    if( !ih_delete_file(ih) )
      _exit(EXIT_FAILURE);

    while( ih_worker_delay() )
      ;

    if( !ih_restore_file(ih) )
      _exit(EXIT_FAILURE);

    print_info("Worker exiting succesfully");
    _exit(EXIT_SUCCESS);
  }
  else
  {
    ih->worker_pid = worker_pid;
  }

  return true;
}

bool ih_worker_block_sigusr()
{
  sigset_t set;

  if( !ih_worker_set_sigset(&set) )
    return false;

  if( sigprocmask(SIG_BLOCK, &set, NULL) == -1 )
  {
    print_error("Failed blocking sigusr signals in worker");
    return false;
  }

  return true;
}

bool ih_worker_set_sigset(sigset_t * set)
{
  if( set == NULL )
    return false;

  if( sigemptyset(set) == -1 )
  {
    print_error("Failed creating empty sigset_t (%s)", strerror(errno));
    return false;
  }

  if( sigaddset(set, SIGUSR1) == -1 )
  {
    print_error("Failed adding SIGUSR1 to sigset_t (%s)", strerror(errno));
    return false;
  }

  if( sigaddset(set, SIGUSR2) == -1 )
  {
    print_error("Failed adding SIGUSR2 to sigset_t (%s)", strerror(errno));
    return false;
  }

  return true;
}

bool ih_worker_delay()
{
  sigset_t set;
  if( !ih_worker_set_sigset(&set) )
  {
    print_error("Fatal: Failed creating sigset");
    _exit(EXIT_FAILURE);
  }

  struct timespec timeout = {4L, 0L};

  int signal = sigtimedwait(&set, /* siginfo_t * info */ NULL, &timeout);

  // If parent sent SIGUSR2, continue timeout
  if( signal == SIGUSR2 )
    return true;

  if( signal == -1 && errno != EAGAIN )
    print_error("sigtimedwait failed (%s)", strerror(errno));

  return false;
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

bool ih_delete_file(struct ino_hide * ih)
{
  if( ih == NULL )
    return false;

  if( unlink(ih->target_fname) == -1 )
  {
    print_error("Failed unlinking file (%s)", strerror(errno));
    return false;
  }

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
  int new_fd = open(ih->target_fname, O_CREAT|O_RDWR|O_EXCL, ih->perm.st_mode);
  if( new_fd == -1 )
  {
    print_error("Creating file %s failed (%s)", ih->target_fname, strerror(errno));
    return false;
  }

  set_file_ownership(new_fd, ih->perm.st_uid, ih->perm.st_gid);

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

bool ih_block_until_need_to_hide(struct ino_hide * ih)
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
    print_error("Reading inotify_fd failed (%s)", strerror(errno));
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

    p_buf_events += sizeof(struct inotify_event) + event->len;
  }

  print_info("Need to hide: %d", need_to_hide);

  return need_to_hide;
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

