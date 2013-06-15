#ifndef INOH_FILE_H
#define INOH_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "inoh/print.h"

bool is_writable_file(const char *);
bool is_regular_file(const char *);
bool copy_fd(int, int);
bool rewind_fd(int);
bool set_file_ownership(int, uid_t, gid_t);
size_t get_max_name_len(void);

#endif
