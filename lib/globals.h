#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <stdio.h>
#include <stdint.h>

#ifndef MIN
#define MIN(a, b) (a < b ? a : b)
#endif

#ifndef MAX
#define MAX(a, b) (a < b ? b : a)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define SIGNUM(x) ((x > 0) ? 1 : (x < 0) ? -1 : 0)

enum ret_t {
  RET_OK = 0,
  RET_ERR = 1,
  RET_INV_PTR = 2,
  RET_MALLOC_FAILED = 3,
  RET_INV_PATH = 4,
  RET_MATH_ERR = 5,
  RET_CANCEL = 6
};

#define RET_IS_OK(call_res) ((call_res) == RET_OK)
#define RET_IS_NOT_OK(call_res) ((call_res) != RET_OK)


#ifdef DEBUG
void debug(const char * const module, const char * const format, ...);
#else
#define debug(module, format, ...) ;
#endif


#endif
