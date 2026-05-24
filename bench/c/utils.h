#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

// reads stdin and progressivelly allocates memory by malloc/reallocing
// works both for files (.o < file) and piping to stdin (printf ".." | .o)
static inline bool read_alloc_entire_stdin(char **out_buf, size_t *out_len) {
  size_t cap = 4096;
  size_t len = 0;
  char  *buf = (char *)malloc(cap);
  if (!buf) {
    perror("malloc");
    goto error;
  }

  for (;;) {
    size_t space = cap - len;
    if (space < 1024) {
      size_t new_cap = cap * 2;
      char  *tmp     = (char *)realloc(buf, new_cap);
      if (!tmp) {
        perror("realloc");
        goto error;
      }
      buf = tmp;
      cap = new_cap;
      space = cap - len;
    }

    size_t n = fread(buf + len, 1, space, stdin);
    len += n;
    if (n < space) {
      if (feof(stdin)) break;
      if (ferror(stdin)) {
        perror("fread");
        goto error;
      }
    }
  }

  char *tmp = (char *)realloc(buf, len + 1);
  if (!tmp) {
    perror("realloc");
    goto error;
  }
  buf       = tmp;
  buf[len]  = '\0'; // null terminator
  *out_buf  = buf;
  *out_len  = len;
  return true;

error:
  if(buf)
    free(buf);
  return false;
}

#endif /* UTILS_H_INCLUDED */
