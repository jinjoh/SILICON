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

#ifndef __PORT_COLOR_MANAGER_H__
#define __PORT_COLOR_MANAGER_H__

#include "globals.h"

typedef struct port_color_list port_color_list_t;
typedef struct port_color_manager port_color_manager_t;

struct port_color_list {
  color_t color;
  char * port_name;

  port_color_list_t * next;
};

struct port_color_manager {
  unsigned int num_entries;
  port_color_list_t * port_color_list;
};

port_color_manager_t * pcm_create();
ret_t pcm_destroy(port_color_manager_t * pcm);
ret_t pcm_destroy_color_list(port_color_manager_t * pcm);


ret_t pcm_remove_entry(port_color_manager_t * pcm,  const char * const port_name);

ret_t pcm_add_color_as_rgba(port_color_manager_t * pcm,  const char * const port_name,
			    uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

port_color_list_t * pcm_get_list_element_for_port(const port_color_manager_t * const pcm,  
						  const char * const port_name);

ret_t pcm_set_color_as_rgba(port_color_manager_t * pcm,  const char * const port_name,
			    uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

color_t pcm_get_color_for_port(const port_color_manager_t * const pcm,  const char * const port_name);

#endif
