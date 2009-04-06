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

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

#include "globals.h"
#include "graphics.h"
#include "logic_model.h"
#include "grid.h"
#include "project.h"

typedef struct plugin_params {

  project_t * project;
  unsigned int min_x, min_y, max_x, max_y;

  void * data_ptr;
} plugin_params_t;

/* function types */
enum PLUGIN_FUNC_TYPE {
  PLUGIN_FUNC_BEFORE_DIALOG = 0,
  PLUGIN_FUNC_AFTER_DIALOG = 1,
  PLUGIN_FUNC_CALC = 2,
  PLUGIN_FUNC_INIT = 3,
  PLUGIN_FUNC_SHUTDOWN = 4
};

/* function types defined as function pointers */
typedef ret_t (*plugin_func_t)(plugin_params_t *);
typedef ret_t (*plugin_raise_dialog_func_t)(void * window_ptr, plugin_params_t *);

/* sth. a plugin can offer */
typedef struct {
  const char * const name;
  
  plugin_func_t func;
  plugin_raise_dialog_func_t before;
  plugin_raise_dialog_func_t after;
  plugin_func_t init;
  plugin_func_t shutdown;
  // LATER: we may need a callback pointer for a preview render function
} plugin_func_descr_t;


/* for the plugin maintanance:
   a linked list of functions a plugin offers */
struct plugin_func_table {
  plugin_func_descr_t * item;
  void * lib_handle;
  struct plugin_func_table * next;
};

typedef struct plugin_func_table plugin_func_table_t;


plugin_func_table_t * plugins_init(const char * const plugin_path);
ret_t plugin_load(const char * const plugin_file, plugin_func_table_t ** func_table);

const char * const plugin_get_func_description(plugin_func_table_t * func_table, int slot);
ret_t plugin_calc_slot(plugin_func_table_t * func_table, int slot, 
		       PLUGIN_FUNC_TYPE func_type, plugin_params_t * func_params, void * window_ptr);
plugin_func_table_t *  plugin_lookup_slot(plugin_func_table_t * func_table, int i);

#endif
