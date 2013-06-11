#ifndef INOH_FILE_H
#define INOH_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#include "inoh/print.h"

bool is_writable_file(const char *);
bool is_regular_file(const char *);
size_t get_max_name_len(void);

#endif
