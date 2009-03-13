#include "globals.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef DEBUG
void debug(const char * const module, const char * const format, ...) {

  va_list args;
  //  printf("[%s] ", module);

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  puts("");

}

#endif
