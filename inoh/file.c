#include "inoh/file.h"

bool is_writable_file(const char * file)
{
  if( file == NULL )
    return false;

  return access(file, W_OK) == 0;
}

bool is_regular_file(const struct file_attr * fattr)
{
  if( fattr == NULL )
    return false;

  return S_ISREG(fattr->st_mode);
}

bool copy_fd(int fd_src, int fd_target)
{
  if( fd_src < 0 || fd_target < 0 )
    return false;

  char buffer[8192] = {0};
  ssize_t byte_count = 0;
  while( true )
  {
    byte_count = read(fd_src, &buffer, sizeof(buffer));

    if( byte_count == 0 )
      return true;

    if( byte_count == -1 )
    {
      print_error("Failed reading file (%s)", strerror(errno));
      return false;
    }

    if( write(fd_target, &buffer, byte_count) != byte_count )
    {
      print_error("Failed writing file (%s)", strerror(errno));
      return false;
    }
  }

  return false;
}

bool rewind_fd(int fd)
{
  if( lseek(fd, 0, SEEK_SET) != 0 )
  {
    print_error("Failed seeking to file offset 0 (%s)", strerror(errno));
    return false;
  }

  return true;
}

bool get_file_attributes(struct file_attr * fattr, const char * fname)
{
  if( fattr == NULL )
    return false;

  struct stat buf_stat;

  if( stat(fname, &buf_stat) != 0 )
  {
    print_error("stat %s failed (%s)", fname, strerror(errno));
    return false;
  }

  fattr->st_mode = buf_stat.st_mode;
  fattr->st_uid = buf_stat.st_uid;
  fattr->st_gid = buf_stat.st_gid;

  return true;
}

bool set_file_ownership(int fd, uid_t uid, gid_t gid)
{
  if( fchown(fd, uid, gid) == -1 )
  {
    print_error("Failed setting file ownership %d:%d (%s)", 
      uid,
      gid,
      strerror(errno)
    );
    return false;
  }

  return true;
}

size_t get_max_name_len(void)
{
  // struct inotify_event may contain (optionally) the name
  // of the file that is subject to the event. To satisfy 
  // the arbitrary size of the filename, we lazily allocate 
  // enough space to hold any filename length possible on this 
  // filesystem (_PC_NAME_MAX).
  
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
    print_error("Maximum filename length is unknown, using %zd", name_max);
  }

  // truncate ssize_t to size_t (guaranteed to be lossless)
  return (size_t)name_max;
}

