#include "grid.h"
#include <assert.h>

ret_t snap_to_grid(const grid_t * const grid, 
		   unsigned int x_in, unsigned int y_in, unsigned int * x_out, unsigned int * y_out) {

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


