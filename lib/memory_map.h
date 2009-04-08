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

#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__

#include <stdint.h>
#include "globals.h"

enum MAP_STORAGE_TYPE {
  MAP_STORAGE_TYPE_UNDEF = 0,
  MAP_STORAGE_TYPE_FILE = 1,
  MAP_STORAGE_TYPE_MEM = 2,
};

struct memory_map {
  
  unsigned int width, height;
  unsigned int bytes_per_elem;
  MAP_STORAGE_TYPE storage_type;

  uint8_t * mem;
  char * filename;
  int fd;
  size_t filesize;	
  int is_temp_file;
};

typedef struct memory_map memory_map_t;

memory_map_t * mm_create(unsigned int width, unsigned int height, unsigned int bytes_per_elem);
ret_t mm_alloc_memory(memory_map_t * map);
ret_t mm_destroy(memory_map_t * map);
ret_t mm_destroy_and_unlink(memory_map_t * map);
ret_t mm_clear(memory_map_t * map);
ret_t mm_clear_area(memory_map_t * map, unsigned int min_x, unsigned int min_y, 
		    unsigned int width, unsigned int height);


ret_t mm_map_temp_file(memory_map_t * img, const char * const project_dir);
ret_t mm_map_file(memory_map_t * img, const char * const project_dir, const char * const filename);
ret_t mm_map_file_by_fd(memory_map_t * img, const char * const project_dir, int fd, const char * const filename);


ret_t mm_deactivate_mapping(memory_map_t * map);
ret_t mm_reactivate_mapping(memory_map_t * map);

ret_t mm_clone_map_data(memory_map_t * const dst, memory_map_t * const src);

// get/set pixels

void * mm_get_ptr(memory_map_t * map, unsigned int x, unsigned int y);


// misc
ret_t mm_scale_and_shift_in_place(memory_map_t *map, 
				  double scaling_x, double scaling_y, 
				  unsigned int shift_x, unsigned int shift_y);
#endif
