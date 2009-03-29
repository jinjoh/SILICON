#ifndef __GRID_H__
#define __GRID_H__

#include "globals.h"

struct grid {
  unsigned int offset_x;
  unsigned int offset_y;
  double dist_x;
  double dist_y;

  int horizontal_lines_enabled;
  int vertical_lines_enabled;
};

typedef struct grid grid_t;

ret_t snap_to_grid(grid_t * grid, unsigned int x_in, unsigned int y_in, unsigned int * x_out, unsigned int * y_out);

#endif
