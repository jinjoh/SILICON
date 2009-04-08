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

#include "port_color_manager.h"
#include "graphics.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

/**
 * Create a new port color manager.
 */
port_color_manager_t * pcm_create() {
  port_color_manager_t * ptr = NULL;
  if((ptr = (port_color_manager_t *) malloc(sizeof(port_color_manager_t))) == NULL) return NULL;
  memset(ptr, 0, sizeof(port_color_manager_t));

  return ptr;
}

/**
 * Destroy a port color mananger.
 */
ret_t pcm_destroy(port_color_manager_t * pcm) {
  ret_t ret;

  assert(pcm != NULL);
  if(pcm == NULL) return RET_INV_PTR;

  if(pcm->port_color_list != NULL) 
    if(RET_IS_NOT_OK(ret = pcm_destroy_color_list(pcm))) return ret;
  memset(pcm, 0, sizeof(port_color_manager_t));
  free(pcm);
  return RET_OK;
}

/**
 * Destroy a port color list.
 */
ret_t pcm_destroy_color_list(port_color_manager_t * pcm) {
  assert(pcm != NULL);

  if(pcm == NULL) return RET_INV_PTR;
  if(pcm->port_color_list == NULL) return RET_OK; // nothing to do

  port_color_list_t * ptr = pcm->port_color_list, * ptr_next = NULL;
  while(ptr != NULL) {

    if(ptr->port_name != NULL) free(ptr->port_name);

    ptr_next = ptr->next;
    free(ptr);
    ptr = ptr->next;
  }

  pcm->port_color_list = NULL;
  return RET_OK;
}

/**
 * Add a color to the port color manager.
 * @return Returns RET_OK, if entry was added, RET_INV_PTR, if a param was a null pointer.
 *         RET_MALLOC_FAILED is returned, if malloc() failed.
 */
ret_t pcm_add_color_as_rgba(port_color_manager_t * pcm,  const char * const port_name,
			    uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
  port_color_list_t * ptr = NULL;
  assert(pcm != NULL);
  assert(port_name != NULL);
  if(pcm == NULL || port_name == NULL) return RET_INV_PTR;

  // checkm if there is already an entry for port_name
  //port_color_list_t * ptr = pcm_get_list_element_for_port(pcm, port_name);
  //if(ptr != NULL) return RET_ERR;

  if((ptr = (port_color_list_t *)malloc(sizeof(port_color_list_t))) == NULL) return RET_MALLOC_FAILED;

  ptr->color = MERGE_CHANNELS(red, green, blue, alpha);
  ptr->port_name = strdup(port_name);
  ptr->next = pcm->port_color_list;

  pcm->port_color_list = ptr;

  return RET_OK;
}


/**
 * Set a color to the port color manager.
 * @return Returns RET_OK, if entry was changed, RET_INV_PTR, if a param was a null pointer and RET_ERR,
 *         if the entry doesn't exists.
 */
ret_t pcm_set_color_as_rgba(port_color_manager_t * pcm,  const char * const port_name,
			    uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
  assert(pcm != NULL);
  assert(port_name != NULL);
  if(pcm == NULL || port_name == NULL) return RET_INV_PTR;

  // check if there is already an entry for port_name
  port_color_list_t * ptr = pcm_get_list_element_for_port(pcm, port_name);
  if(ptr == NULL) return RET_ERR;

  ptr->color = MERGE_CHANNELS(red, green, blue, alpha);
  return RET_OK;
}

/**
 * Look up a port name and return the list item.
 * @return Returns the list item. Return NULL if it was not found or for invalid params.
 */
port_color_list_t * pcm_get_list_element_for_port(const port_color_manager_t * const pcm,  
						  const char * const port_name) {

  assert(pcm != NULL);
  assert(port_name != NULL);
  if(pcm == NULL || port_name == NULL) return NULL;

  port_color_list_t * ptr = pcm->port_color_list;

  while(ptr != NULL) {
    if(!strcmp(ptr->port_name, port_name)) {
      return ptr;
    }
    else ptr = ptr->next;
  }

  return NULL;
}


/**
 * @return Returns RET_ERR, if the port was not found.
 */
ret_t pcm_remove_entry(port_color_manager_t * pcm,  const char * const port_name) {
  assert(pcm != NULL);
  assert(port_name != NULL);
  if(pcm == NULL || port_name == NULL) return RET_INV_PTR;

  port_color_list_t 
    * ptr = pcm->port_color_list, 
    * ptr_next = NULL;

  if(ptr != NULL && !strcmp(ptr->port_name, port_name)) {
    pcm->port_color_list = ptr->next;
    if(ptr->port_name) free(ptr->port_name);
    free(ptr);
    return RET_OK;
  }

  while(ptr != NULL && ptr->next != NULL) {

    if(ptr->next->port_name != NULL && !strcmp(ptr->next->port_name, port_name)) {
      free(ptr->next->port_name);
      ptr_next = ptr->next;
      ptr->next = ptr->next->next;
      free(ptr_next);
      return RET_OK;
    }
    else ptr = ptr->next;
  }

  return RET_ERR;

}

/**
 * Get a color for a port.
 */

color_t pcm_get_color_for_port(const port_color_manager_t * const pcm,  const char * const port_name) {
  assert(pcm != NULL);
  assert(port_name != NULL);
  if(pcm == 0 || port_name == NULL) return 0;

  port_color_list_t * ptr = pcm_get_list_element_for_port(pcm, port_name);
  if(ptr == NULL) return 0;
  else return ptr->color;
 
}
