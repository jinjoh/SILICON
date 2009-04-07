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

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include "logic_model.h"
#include "graphics.h"
#include "renderer.h"
#include "alignment_marker.h"
#include "grid.h"
#include "scaling_manager.h"
#include "port_color_manager.h"

struct project {

  unsigned int width, height;
  int num_layers;
  char * project_name;
  char * project_description;

  char * project_dir;
  
  image_t ** bg_images;
  scaling_manager_t * scaling_manager;
  port_color_manager_t * port_color_manager;

  logic_model_t * lmodel;
  
  int current_layer;

  unsigned int pin_diameter;
  unsigned int wire_diameter;
  unsigned int lambda;

  grid_t grid;

  alignment_marker_set_t * alignment_marker_set;
  char * project_file_version;
};

typedef struct project project_t;

project_t * project_create(const char * project_dir, unsigned int width, unsigned int height, int num_layers);
ret_t project_destroy(project_t * project);
ret_t project_init_directory(const char * const directory, int enable_mkdir);
project_t * project_load(const char * const project_dir /*, render_params_t * const render_params*/);

ret_t project_map_background_memfiles(project_t * const project);

ret_t project_save(const project_t * const project);

ret_t project_cleanup(const char * const project_dir);

ret_t project_set_name(project_t * const project, const char * const name);
ret_t project_set_description(project_t * const project, const char * const descr);

#endif
