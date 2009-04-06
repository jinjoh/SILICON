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

#ifndef __SCALING_MANAGER_H__
#define __SCALING_MANAGER_H__

#include "graphics.h"
#include <math.h>

typedef struct image_list image_list_t;

struct image_list {
  unsigned int layer;
  unsigned int zoom;
  image_t * image;
  image_list_t * next;
};

typedef struct scaling_manager {
  unsigned int zoom_in_factor;
  unsigned int zoom_out_factor;

  unsigned int num_layers;
  char * project_dir;
  image_t ** bg_images;
  image_list_t * zoom_out_images;
  image_list_t * zoom_in_images;
} scaling_manager_t;

scaling_manager_t * scalmgr_create(int num_layers, image_t ** bg_images,
				   const char * const project_dir);
ret_t scalmgr_destroy(scaling_manager_t * sm);

ret_t scalmgr_set_scalings(scaling_manager_t * sm, unsigned int zoom_out_factor, 
			   unsigned int zoom_in_factor);

image_t * scalmgr_get_image(scaling_manager_t * sm, unsigned int layer, double scaling, 
			    double * scaling_found);

ret_t scalmgr_map_files_for_layer(scaling_manager_t * sm, unsigned int layer);
ret_t scalmgr_unmap_files_for_layer(scaling_manager_t * sm, unsigned int layer);

unsigned int scalmgr_get_max_zoom_in_factor(scaling_manager_t * sm);
unsigned int scalmgr_get_max_zoom_out_factor(scaling_manager_t * sm);

ret_t scalmgr_recreate_scalings(scaling_manager_t * sm);

#endif
