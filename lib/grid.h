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

#ifndef __GRID_H__
#define __GRID_H__

#include "globals.h"

enum GRID_MODE {
  UNDEFINED_GRID_MODE = 0,
  USE_REGULAR_GRID = 1,
  USE_UNREGULAR_GRID = 2 };

struct grid {
  GRID_MODE grid_mode;

  // regular grid
  unsigned int offset_x;
  unsigned int offset_y;
  double dist_x;
  double dist_y;

  int horizontal_lines_enabled;
  int vertical_lines_enabled;

  // unregular horizontal grid
  unsigned int uhg_enabled;
  unsigned int num_uhg_entries;
  unsigned int * uhg_offsets;

  // unregular vertical grid
  unsigned int uvg_enabled;
  unsigned int num_uvg_entries;
  unsigned int * uvg_offsets;

};

typedef struct grid grid_t;


grid_t * grid_create();

ret_t grid_destroy(grid_t * const grid);

ret_t snap_to_regular_grid(const grid_t * const grid, 
			   unsigned int x_in, unsigned int y_in, 
			   unsigned int * x_out, unsigned int * y_out);

ret_t grid_set_uhg_offsets(grid_t * const grid, unsigned int * offsets, unsigned int num_entries);
ret_t grid_set_uvg_offsets(grid_t * const grid, unsigned int * offsets, unsigned int num_entries);


ret_t grid_alloc_mem(grid_t * const grid, unsigned int num_uhg_entries, unsigned int num_uvg_entries);

ret_t grid_get_first_v_offset(const grid_t * const grid, unsigned int min_x, unsigned int width, unsigned int * x_out);
ret_t grid_get_next_v_offset(const grid_t * const grid, unsigned int offset, unsigned int width, unsigned int * x_out);

ret_t grid_get_first_h_offset(const grid_t * const grid, unsigned int min_y, unsigned int height, unsigned int * y_out);
ret_t grid_get_next_h_offset(const grid_t * const grid, unsigned int offset, unsigned int height, unsigned int * y_out);

ret_t grid_add_horizontal_grid_line(grid_t * const grid, unsigned int offset);
ret_t grid_add_vertical_grid_line(grid_t * const grid, unsigned int offset);

#endif
