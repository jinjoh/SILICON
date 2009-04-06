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

#ifndef __IMG_ALGORITHMS_H__
#define __IMG_ALGORITHMS_H__

#include "grid.h"
#include "logic_model.h"

struct matching_params {
  image_t * img;

  unsigned int min_x, min_y, max_x, max_y;
  logic_model_t * lmodel;
  int layer;
  char * project_dir;
  unsigned int width;
  
  unsigned int pin_diameter;
  int match_horizontal_objects;
  int match_vertical_objects;
  //OBJECT_TYPE match_object_type;

  unsigned int threshold_col_separation;
  grid_t * grid;
};

typedef struct matching_params matching_params_t;

ret_t imgalgo_to_grayscale(image_t * img);
ret_t imgalgo_to_color_similarity(image_t * img, uint32_t color);
ret_t imgalgo_separate_by_threshold(image_t * img, unsigned int threshold);
ret_t imgalgo_run_object_matching(matching_params_t * m_params);

ret_t imgalgo_match_wires(image_t * img, matching_params_t * m_params);
ret_t imgalgo_match_pin(image_t * img, matching_params_t * m_params);



#endif
