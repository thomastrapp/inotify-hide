#include "inoh/ino-hide-worker.h"

bool ih_worker_is_alive(const struct ino_hide * ih)
{
  if( ih == NULL )
    return false;
 
  if( ih->worker_pid < 0  )
  {
    return false;
  }
  
  int status = 0;
  pid_t childpid = waitpid(ih->worker_pid, &status, WNOHANG);

  // If we don't have any children, errno is set to ECHILD,
  // which is no error in this context
  if( childpid == -1 && errno != ECHILD )
  {
    print_error("Failed querying child status (%s)", strerror(errno));
    return false;
  }

  if( childpid == 0 )
  {
    return true;
  }
  else if( childpid != -1 )
  {
    if( WIFEXITED(status) )
    {
      print_info("Child exited with status %d", WEXITSTATUS(status));
    }
    else if( WIFSIGNALED(status) )
    {
      print_info("Child was terminated by signal %d", WTERMSIG(status));
    }
  }

  return false;
}

bool ih_worker_restart_delay(const struct ino_hide * ih)
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

    if( !ih_worker_block_signals() )
      _exit(EXIT_FAILURE);

    if( !ih_open_file(ih) )
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

bool ih_worker_block_signals(void)
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

  if( sigaddset(set, SIGINT) == -1 )
  {
    print_error("Failed adding SIGINT to sigset_t (%s)", strerror(errno));
    return false;
  }

  return true;
}

bool ih_worker_delay(void)
{
  sigset_t set;
  if( !ih_worker_set_sigset(&set) )
  {
    print_error("Fatal: Failed creating sigset");
    _exit(EXIT_FAILURE);
  }

  struct timespec timeout = {4L, 0L};

  int signal = sigtimedwait(&set, /* siginfo_t * info */ NULL, &timeout);

  print_info("Worker received signal %d", signal);

  // If parent sent SIGUSR2, continue timeout
  if( signal == SIGUSR2 )
    return true;

  if( signal == -1 && errno != EAGAIN )
    print_error("sigtimedwait failed (%s)", strerror(errno));

  return false;
}

