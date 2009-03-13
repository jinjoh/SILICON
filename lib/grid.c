#include "grid.h"

void snap_to_grid(grid_t * grid, unsigned int x_in, unsigned int y_in, unsigned int * x_out, unsigned int * y_out) {
	
  if(x_in <= grid->offset_x) *x_out = grid->offset_x;	
  else {
    unsigned int grid_coord_x_lo = (unsigned int)((x_in - grid->offset_x) / grid->dist_x);
    unsigned int grid_coord_x_hi = grid_coord_x_lo + 1;
    
    if( (grid_coord_x_hi * grid->dist_x + grid->offset_x - x_in) < (x_in - (grid_coord_x_lo * grid->dist_x + grid->offset_x)))       
      *x_out = (unsigned int)(grid_coord_x_hi * grid->dist_x + grid->offset_x);
    else  *x_out = (unsigned int)(grid_coord_x_lo * grid->dist_x + grid->offset_x);
  }
  
  if(y_in <= grid->offset_y) *y_out = grid->offset_y;	
  else {
    unsigned int grid_coord_y_lo = (unsigned int)((y_in - grid->offset_y) / grid->dist_y);
    unsigned int grid_coord_y_hi = grid_coord_y_lo + 1;
    
    if( (grid_coord_y_hi * grid->dist_y + grid->offset_y - y_in) < (y_in - (grid_coord_y_lo * grid->dist_y + grid->offset_y)))
      *y_out = (unsigned int)(grid_coord_y_hi * grid->dist_y + grid->offset_y);
    else *y_out = (unsigned int)(grid_coord_y_lo * grid->dist_y + grid->offset_y);
  }
}
