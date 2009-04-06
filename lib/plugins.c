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

#include "plugins.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


plugin_func_table_t * plugins_init(const char * const plugin_path) {
  
  DIR * dir;
  struct dirent * dir_ent;
  plugin_func_table_t * table = NULL;

  debug(TM, "loading plugins from %s", plugin_path);
  
  if((dir = opendir(plugin_path)) == NULL) {
    debug(TM, "plugins_init() invalid path %s", plugin_path);
    return NULL;
  }


  while((dir_ent = readdir(dir)) != NULL) {

    if(strlen(dir_ent->d_name) > 3 // there is already a length field but it's name depends on platform
       && !strcmp(dir_ent->d_name + strlen(dir_ent->d_name) - 3, ".so")) {

      char plugin_file[PATH_MAX];
      
      snprintf(plugin_file, PATH_MAX, "%s/%s", plugin_path, dir_ent->d_name);
      debug(TM, "loading plugin %s", plugin_file);

      if(!RET_IS_OK(plugin_load(plugin_file, &table)))
	return NULL;
    }
    
  }

  if(closedir(dir) != 0) {
    debug(TM, "plugins_init() can't close dir");
    return NULL;
  }
  
  return table;
}


ret_t plugin_load(const char * const plugin_file, plugin_func_table_t ** func_table) {
    void * lib_handle;

    if(func_table == NULL) return RET_INV_PTR;

    if((lib_handle = dlopen(plugin_file, RTLD_LAZY)) == NULL) {
      debug(TM, "can't open plugin: %s", plugin_file, dlerror());
      return RET_ERR;
    }
    else debug(TM, "file %s loaded", plugin_file);
    
    /* loading symbols ... */
    plugin_func_descr_t *funcs_descr = (plugin_func_descr_t *)dlsym(lib_handle, "plugin_func_descriptions");
    if(!funcs_descr) {
      debug(TM, "can't load plugin's functions descriptions");
      if(dlclose(lib_handle) != 0) debug(TM, "can't close plugin");
      return RET_ERR;
    }
    
    
    int i = 0;
    while(funcs_descr[i].name != NULL) {
      // alloc mem for table node
      plugin_func_table_t * ptr;

      if((ptr = ( plugin_func_table_t *) malloc(sizeof(struct plugin_func_table))) == NULL) {
	debug(TM, "can't malloc() mem for a plugin_func_table_t element");
	 return RET_ERR;
      }

      debug(TM, "loading plugin '%s'", funcs_descr[i].name);

      ptr->item = &funcs_descr[i];
      ptr->lib_handle = lib_handle;
      ptr->next = NULL;

      // traverse to tail and insert record
      if(*func_table == NULL) {
	*func_table = ptr;
      }
      else {
	plugin_func_table_t * curr_ptr = *func_table;
	while(curr_ptr->next != NULL) curr_ptr = curr_ptr->next;
	curr_ptr->next = ptr;
      }

      i++;
    }

    return RET_OK;
}

const char * const plugin_get_func_description(plugin_func_table_t * func_table, int slot) {

  plugin_func_table_t * ptr = plugin_lookup_slot(func_table, slot);
  return ptr ? ptr->item->name : NULL;
}


ret_t plugin_calc_slot(plugin_func_table_t * func_table, int slot, 
		       PLUGIN_FUNC_TYPE func_type, plugin_params_t * func_params, void * window_ptr) {
  plugin_func_table_t * ptr = plugin_lookup_slot(func_table, slot);

  assert(ptr);
  assert(ptr->item);
  if(!ptr || !ptr->item) return RET_INV_PTR;

  switch(func_type) {
  case PLUGIN_FUNC_BEFORE_DIALOG:
    return ptr->item->before ? (*(ptr->item->before))(window_ptr, func_params) : RET_OK;
    break;
  case PLUGIN_FUNC_AFTER_DIALOG:
    return ptr->item->after ? (*(ptr->item->after))(window_ptr, func_params) : RET_OK;
    break;
  case PLUGIN_FUNC_CALC:
    return ptr->item->func ? (*(ptr->item->func))(func_params) : RET_OK;
    break;
  case PLUGIN_FUNC_INIT:
    return ptr->item->init ? (*(ptr->item->init))(func_params) : RET_OK;
    break;
  case PLUGIN_FUNC_SHUTDOWN:
    return ptr->item->shutdown ? (*(ptr->item->shutdown))(func_params) : RET_OK;
    break;
  default:
    return RET_ERR;
  }
  return RET_ERR;
}

plugin_func_table_t *  plugin_lookup_slot(plugin_func_table_t * func_table, int slot_pos) {

  plugin_func_table_t * ptr = func_table;
  int i = 0;
  while(ptr != NULL && i <= slot_pos) {
    
    if(i == slot_pos && ptr != NULL) {
      debug(TM, "slot %d found", i);
      return ptr;
    }
    else {
      ptr = ptr->next;
      i++;
    }
  }
  
  return NULL;
}
