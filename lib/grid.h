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

struct grid {
  unsigned int offset_x;
  unsigned int offset_y;
  double dist_x;
  double dist_y;

  int horizontal_lines_enabled;
  int vertical_lines_enabled;
};

typedef struct grid grid_t;

ret_t snap_to_grid(const grid_t * const grid, unsigned int x_in, unsigned int y_in, unsigned int * x_out, unsigned int * y_out);

#endif
