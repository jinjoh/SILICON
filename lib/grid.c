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

#include "grid.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>


grid_t * grid_create() {
  grid_t * ptr = NULL;

  if((ptr = (grid_t *)malloc(sizeof(grid_t))) != NULL) {
    memset(ptr, 0, sizeof(grid_t));
  }

  return ptr;
}


ret_t grid_destroy(grid_t * const grid) {
  assert(grid != NULL);
  if(grid == NULL) return RET_INV_PTR;

  if(grid->uhg_offsets != NULL) free(grid->uhg_offsets);
  if(grid->uvg_offsets != NULL) free(grid->uvg_offsets);
  memset(grid, 0, sizeof(grid_t));
  free(grid);
  return RET_OK;
}

ret_t snap_to_regular_grid(const grid_t * const grid, 
		   unsigned int x_in, unsigned int y_in, unsigned int * x_out, unsigned int * y_out) {

  assert(grid != NULL);
  assert(x_out != NULL || y_out != NULL);
  if(grid == NULL) return RET_INV_PTR;
  if(x_out == NULL && y_out == NULL) return RET_INV_PTR;
	
  if(x_out != NULL) {
    if(x_in <= grid->offset_x) *x_out = grid->offset_x;	
    else {
      if(grid->dist_x == 0) return RET_ERR;

      unsigned int grid_coord_x_lo = (unsigned int)((x_in - grid->offset_x) / grid->dist_x);
      unsigned int grid_coord_x_hi = grid_coord_x_lo + 1;
    
      if( (grid_coord_x_hi * grid->dist_x + grid->offset_x - x_in) < (x_in - (grid_coord_x_lo * grid->dist_x + grid->offset_x)))       
	*x_out = (unsigned int)(grid_coord_x_hi * grid->dist_x + grid->offset_x);
      else  *x_out = (unsigned int)(grid_coord_x_lo * grid->dist_x + grid->offset_x);
    }
  }

  if(y_out != NULL) {

    if(y_in <= grid->offset_y) *y_out = grid->offset_y;	
    else {
      if(grid->dist_y == 0) return RET_ERR;

      unsigned int grid_coord_y_lo = (unsigned int)((y_in - grid->offset_y) / grid->dist_y);
      unsigned int grid_coord_y_hi = grid_coord_y_lo + 1;
      
      if( (grid_coord_y_hi * grid->dist_y + grid->offset_y - y_in) < (y_in - (grid_coord_y_lo * grid->dist_y + grid->offset_y)))
	*y_out = (unsigned int)(grid_coord_y_hi * grid->dist_y + grid->offset_y);
      else *y_out = (unsigned int)(grid_coord_y_lo * grid->dist_y + grid->offset_y);
    }
  }

  return RET_OK;
}


int grid_compare_uintegers(const void *a, const void *b) {
  if(*(unsigned int *)a < *(unsigned int *)b) return -1;
  else if(*(unsigned int *)a > *(unsigned int *)b) return 1;
  else return 0;
}



ret_t grid_set_uhg_offsets(grid_t * const grid, unsigned int * offsets, unsigned int num_entries) {
  assert(grid != NULL);
  if(grid == NULL) return RET_INV_PTR;

  qsort(offsets, num_entries, sizeof(unsigned int), grid_compare_uintegers);

  grid->num_uhg_entries = num_entries;
  if(grid->uhg_offsets != NULL) free(grid->uhg_offsets);
  grid->uhg_offsets = offsets;
  return RET_OK;
}

ret_t grid_set_uvg_offsets(grid_t * const grid, unsigned int * offsets, unsigned int num_entries) {
  assert(grid != NULL);
  if(grid == NULL) return RET_INV_PTR;

  qsort(offsets, num_entries, sizeof(unsigned int), grid_compare_uintegers);

  grid->num_uvg_entries = num_entries;
  if(grid->uvg_offsets != NULL) free(grid->uvg_offsets);
  grid->uvg_offsets = offsets;
  return RET_OK;
}

ret_t grid_alloc_mem(grid_t * const grid, unsigned int num_uhg_entries, unsigned int num_uvg_entries) {
  unsigned int 
    * ptr1 = NULL,
    * ptr2 = NULL;
  assert(grid != NULL);
  if(grid == NULL) return RET_INV_PTR;

  if((ptr1 = (unsigned int * ) malloc(num_uhg_entries * sizeof(unsigned int))) == NULL) {
    return RET_MALLOC_FAILED;
  }

  if((ptr2 = (unsigned int * ) malloc(num_uvg_entries * sizeof(unsigned int))) == NULL) {
    free(ptr1);
    return RET_MALLOC_FAILED;
  }

  memset(ptr1, 0, num_uhg_entries * sizeof(unsigned int));
  memset(ptr2, 0, num_uvg_entries * sizeof(unsigned int));

  assert(grid->uhg_offsets == NULL);
  assert(grid->uvg_offsets == NULL);

  grid->num_uhg_entries = num_uhg_entries;
  grid->num_uvg_entries = num_uvg_entries;
  grid->uhg_offsets = ptr1;
  grid->uvg_offsets = ptr2;

  return RET_OK;
}


/**
 * Get the first offset, that is larger than min_x
 */
ret_t grid_get_first_v_offset(const grid_t * const grid, unsigned int min_x, unsigned int width, unsigned int * x_out) {
  assert(grid != NULL);
  assert(x_out != NULL);
  if(grid == NULL || x_out == NULL) return RET_INV_PTR;;

  if(grid->grid_mode == USE_REGULAR_GRID) {
    
    if(RET_IS_NOT_OK(snap_to_regular_grid(grid, min_x, 0, x_out, NULL))) return RET_ERR;

    if(*x_out < min_x) *x_out += grid->dist_x;

    if(*x_out < width) return RET_OK;
    else return RET_ERR;
  }
  else if(grid->grid_mode == USE_UNREGULAR_GRID) {
    unsigned int i;
    for(i = 0; i < grid->num_uvg_entries; i++) {
      debug(TM, "\toffset = %d min_x=%d width = %d", grid->uvg_offsets[i], min_x, width);
      if(grid->uvg_offsets[i] >= min_x && grid->uvg_offsets[i] < width) {
	*x_out = grid->uvg_offsets[i];
	return RET_OK;
      }
    }
    debug(TM, "no first grid found");
    return RET_ERR;
  }
  return RET_ERR;  
}


/**
 * Get the first offset, that is larger than min_x
 */
ret_t grid_get_first_h_offset(const grid_t * const grid, unsigned int min_y, unsigned int height, unsigned int * y_out) {
  assert(grid != NULL);
  assert(y_out != NULL);
  if(grid == NULL || y_out == NULL) return RET_INV_PTR;;

  if(grid->grid_mode == USE_REGULAR_GRID) {
    
    if(RET_IS_NOT_OK(snap_to_regular_grid(grid, 0, min_y, NULL, y_out))) return RET_ERR;

    if(*y_out < min_y) *y_out += grid->dist_y;

    if(*y_out < height) return RET_OK;
    else return RET_ERR;
  }
  else if(grid->grid_mode == USE_UNREGULAR_GRID) {
    unsigned int i;
    for(i = 0; i < grid->num_uhg_entries; i++) {
      debug(TM, "\toffset = %d min_y=%d height = %d", grid->uhg_offsets[i], min_y, height);
      if(grid->uhg_offsets[i] >= min_y && grid->uhg_offsets[i] < height) {
	*y_out = grid->uhg_offsets[i];
	return RET_OK;
      }
    }
    debug(TM, "no first grid found");
    return RET_ERR;
  }
  
  return RET_ERR;
}

/**
 * Get the offset for the next vertical line, that is larger than the parameter offset.
 * @return Returns RET_OK. If there is no further offset, RET_ERR is returned.
 */
ret_t grid_get_next_v_offset(const grid_t * const grid, unsigned int offset, unsigned int width, unsigned int * x_out) {
  assert(grid != NULL);
  assert(x_out != NULL);
  if(grid == NULL || x_out == NULL) return RET_INV_PTR;;

  if(grid->grid_mode == USE_REGULAR_GRID) { 
    
    if(offset + grid->dist_x < width) {
      unsigned int new_x = offset + grid->dist_x;

      if(RET_IS_OK(snap_to_regular_grid(grid, new_x, 0, x_out, NULL))) {
	if(*x_out < width) return RET_OK;
	else return RET_ERR;
      }
      
    }
    return RET_ERR;
  }
  else if(grid->grid_mode == USE_UNREGULAR_GRID) {
    unsigned int i;
    for(i = 0; i < grid->num_uvg_entries; i++) {
      debug(TM, "\toffset = %d width = %d", grid->uvg_offsets[i], width);
      if(grid->uvg_offsets[i] > offset && grid->uvg_offsets[i] < width) {
	*x_out = grid->uvg_offsets[i];
	return RET_OK;
      }
    }
    return RET_ERR;
  }

  return RET_ERR;
}

/**
 * Get the offset for the next horizontal line, that is larger than the parameter offset.
 * @return Returns RET_OK. If there is no further offset, RET_ERR is returned.
 */

ret_t grid_get_next_h_offset(const grid_t * const grid, unsigned int offset, unsigned int height, unsigned int * y_out) {
  assert(grid != NULL);
  assert(y_out != NULL);
  if(grid == NULL || y_out == NULL) return RET_INV_PTR;;

  if(grid->grid_mode == USE_REGULAR_GRID) { 
    
    if(offset + grid->dist_y < height) {
      unsigned int new_y = offset + grid->dist_y;

      if(RET_IS_OK(snap_to_regular_grid(grid, 0, new_y, NULL, y_out))) {
	if(*y_out < height) return RET_OK;
	else return RET_ERR;
      }
      
    }
    return RET_ERR;
  }
  else if(grid->grid_mode == USE_UNREGULAR_GRID) {
    unsigned int i;
    for(i = 0; i < grid->num_uhg_entries; i++) {
      debug(TM, "\toffset = %d height = %d", grid->uhg_offsets[i], height);
      if(grid->uhg_offsets[i] > offset && grid->uhg_offsets[i] < height) {
	*y_out = grid->uhg_offsets[i];
	return RET_OK;
      }
    }
    return RET_ERR;
  }

  return RET_ERR;
}


/**
 * Add a horizontal grid line to the set of unregular grid lines.
 */
ret_t grid_add_horizontal_grid_line(grid_t * const grid, unsigned int offset) {
  assert(grid != NULL);
  if(grid == NULL) return RET_INV_PTR;

  unsigned int * ptr = (unsigned int *)realloc(grid->uhg_offsets, 
					       sizeof(unsigned int) * (grid->num_uhg_entries + 1));
  assert(ptr != NULL);
  if(ptr == NULL) return RET_MALLOC_FAILED;
  ptr[grid->num_uhg_entries] = offset;

  qsort(ptr, grid->num_uhg_entries + 1, sizeof(unsigned int), grid_compare_uintegers);

  grid->uhg_offsets = ptr;
  grid->num_uhg_entries++;

  return RET_OK;
}

/**
 * Add a vertical grid line to the set of unregular grid lines.
 */

ret_t grid_add_vertical_grid_line(grid_t * const grid, unsigned int offset) {
  assert(grid != NULL);
  if(grid == NULL) return RET_INV_PTR;

  unsigned int * ptr = (unsigned int *)realloc(grid->uvg_offsets, 
					       sizeof(unsigned int) * (grid->num_uvg_entries + 1));
  assert(ptr != NULL);
  if(ptr == NULL) return RET_MALLOC_FAILED;
  ptr[grid->num_uvg_entries] = offset;

  qsort(ptr, grid->num_uvg_entries + 1, sizeof(unsigned int), grid_compare_uintegers);

  grid->uvg_offsets = ptr;
  grid->num_uvg_entries++;

  return RET_OK;

}
