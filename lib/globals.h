/*                                                                              
                                                                                
This file is part of the IC reverse engineering tool degate.                    
                                                                                
Copyright 2008, 2009 by Martin Schobert                                         
                                                                                
Degate is free software: you can redistribute it and/or modify                  
it under the terms of the GNU General Public License as published by            
the Free Software Foundation, either version 3 of the License, or               
any later version.                                                              
                                                                                
Degate is distributed in the hope that it will be useful,                       
but WITHOUT ANY WARRANTY; without even the implied warranty of                  
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   
GNU General Public License for more details.                                    
                                                                                
You should have received a copy of the GNU General Public License               
along with degate. If not, see <http://www.gnu.org/licenses/>.                  
                                                                                
*/

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

#define TM __FILE__,__LINE__


#ifdef DEBUG
void debug(const char * const module, int line, const char * const format, ...);
#else
#define debug(module, line, format, ...) ;
#endif

#define degate_mime_type "application/degate"

#if __SIZEOF_POINTER__ == 4
#define ARCH_32
#define MAP_FILES_ON_DEMAND
#elif __SIZEOF_POINTER__ == 8
#define ARCH_64
#else
#error "Unknown architecture"
#endif

typedef uint32_t color_t;

#endif
