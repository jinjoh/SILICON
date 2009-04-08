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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>


#include "scaling_manager.h"

/**
 * The scaling manager maintains a set of precalculated scaled images. The
 * drawback is, that it requires in more disc space. If all images are mapped
 * into the address space via mmap(), it also reduces the address space. This
 * constraints the amount of image data, that can be loaded.
 *
 * In contrast to the drawbacks, the precalculated scaled images improve the
 * rendering quality and the speed of rendering. If you store images on a
 * mechanical disc, seeking to noncached blocks increases io load. The precalulated
 * scaled images can also be used by template matching to increase performance.
 *
 */

/**
 * Create a new scaling manager object.
 */
scaling_manager_t * scalmgr_create(int num_layers, image_t ** bg_images, 
				   const char * const project_dir) {
  scaling_manager_t * ptr = NULL;
  assert(num_layers > 0);
  assert(bg_images != NULL);
	 
  if(num_layers == 0) return NULL;
  if(bg_images == NULL) return NULL;
  if(bg_images[0] == NULL) return NULL;

  if((ptr = (scaling_manager_t *)malloc(sizeof(scaling_manager_t))) == NULL) {
    return NULL;
  }
  memset(ptr, 0, sizeof(scaling_manager_t));

  ptr->num_layers = num_layers;
  ptr->bg_images = bg_images;
  ptr->project_dir = strdup(project_dir);

  return ptr;
}

/**
 * The function destroyes a scling manager object. It does not destroy the 
 * original background images. But it destroys images, that were handled by the
 * scaling manager.
 */
ret_t scalmgr_destroy(scaling_manager_t * sm) {
  assert(sm != NULL);
  if(sm == NULL) return RET_INV_PTR;

  if(sm->project_dir != NULL) free(sm->project_dir);

  memset(sm, 0, sizeof(scaling_manager_t));
  free(sm);

  return RET_OK;
}

unsigned int get_nearest_power_of_two(unsigned int value) {
  unsigned int i = 1;

  if (value == 0) return 1;
  for (;;) {
    if (value == 1) return i;
    else if (value == 3) return i*4;
    value >>= 1; 
    i *= 2;
  }
}


/**
 * Get a pointer to an image, that is scaled up or scaled down by a factor near to 
 * a given scaling factor.
 */
image_t * scalmgr_get_image(scaling_manager_t * sm, unsigned int layer, double scaling, 
			    double * scaling_found) {
  assert(sm != NULL);
  assert(scaling_found != NULL);
  assert(layer < sm->num_layers);
  if(sm == NULL || scaling_found == NULL || layer >= sm->num_layers) return NULL;
  
  unsigned int factor;

  if(scaling > 1 && sm->zoom_out_images != NULL){
    image_list_t * ptr = sm->zoom_out_images;

    factor = get_nearest_power_of_two(lrint(scaling));
    if(factor > sm->zoom_out_factor) factor = sm->zoom_out_factor;

    while(ptr != NULL) {
      if(ptr->layer == layer && ptr->zoom == factor) {
	debug(TM, "return prescaled image");

	if(scaling < 1) *scaling_found = 1.0/factor;
	else *scaling_found = factor;
	return ptr->image;
      }
      ptr = ptr->next;
    }
  }
  else {
    debug(TM, "return normal image");
    assert(sm->bg_images[layer] != NULL);
    *scaling_found = 1;
    return sm->bg_images[layer];
  }
  
  assert( 1 == 0);
  return NULL;
}

image_list_t * scalmgr_create_list(unsigned int layer, unsigned int zoom, image_t * image) {
  image_list_t * ptr = NULL;
  if((ptr = (image_list_t *)malloc(sizeof(image_list_t))) == NULL) return NULL;

  memset(ptr, 0, sizeof(image_list_t));

  ptr->layer = layer;
  ptr->zoom = zoom;
  ptr->image = image;
  return ptr;
}

ret_t scalmgr_destroy_image_list(image_list_t * list) {
  assert(list != NULL);
  if(list == NULL) return RET_INV_PTR;
  ret_t ret;
  image_list_t * ptr = list, * ptr_next = NULL;
  while(ptr != NULL) {

    if(RET_IS_NOT_OK(ret = gr_image_destroy(ptr->image))) return ret;
    ptr_next = ptr->next;
    memset(ptr, 0, sizeof(image_list_t));
    free(ptr);
    ptr = ptr->next;
  }
  return RET_OK;
}

ret_t scalmgr_destroy_scalings(scaling_manager_t * sm) {
  ret_t ret;
  assert(sm != NULL);
  if(sm == NULL) return RET_INV_PTR;

  
  if(sm->zoom_out_factor >= 2) {
    if(sm->zoom_out_images != NULL && 
       RET_IS_NOT_OK(ret = scalmgr_destroy_image_list(sm->zoom_out_images))) return ret;
    sm->zoom_out_images = NULL;
  }

  return RET_OK;
}

image_list_t * scalmgr_get_list_elem(image_list_t * img_list, 
				     unsigned int zoom_factor, unsigned int layer) {
  image_list_t * ptr = img_list;

  while(ptr != NULL) {
    if(ptr->zoom == zoom_factor && ptr->layer == layer) return ptr;
    ptr = ptr->next;
  }
  return NULL;
}


image_t * scalmgr_create_scaled_image(scaling_manager_t * sm,
				      image_t * master_img, unsigned int width, unsigned int height,
				      unsigned int layer, unsigned int zoom_i) {
  image_t * img = NULL;
  char filename[PATH_MAX];
  char fq_filename[PATH_MAX];
  struct stat stat_buf;

  debug(TM, "\tcreate image");
  if((img = gr_create_image(width, height, IMAGE_TYPE_RGBA)) == NULL) {
    return NULL;
  }
    
  snprintf(filename, sizeof(filename), "scaled_layer_%02d.%d.dat", layer, zoom_i);
  snprintf(fq_filename, sizeof(fq_filename), "%s/%s", sm->project_dir, filename);
  int file_exists = stat(fq_filename, &stat_buf);
    
  // map file
  debug(TM, "\tmap image from file %s", filename);
  if(!RET_IS_OK(gr_map_file(img, sm->project_dir, filename))) {
    debug(TM, "mapping failed: %s", filename);
    return NULL;
  }
    
  // scale
  if(file_exists == -1) { // does not exists
    debug(TM, "\tfile does not exists - scaling image");
      
    if(RET_IS_NOT_OK(gr_scale_image(master_img, img))) {
      debug(TM, "scaling failed: %s", filename);
      return NULL;
    }
  }
  
  return img;
}

ret_t scalmgr_load_scalings_for_layer(scaling_manager_t * sm, unsigned int max_factor, 
				      unsigned int layer) {

  unsigned int zoom_i;
  ret_t ret;
  image_list_t * list = NULL;

  debug(TM, "load scaled images for layer %d - maxzoom = %d", layer, max_factor);
  assert(sm != NULL);
  if(sm == NULL) return RET_INV_PTR;

  if(max_factor < 2) return RET_OK;
  
  unsigned int width = sm->bg_images[0]->width;
  unsigned int height = sm->bg_images[0]->height;

#ifdef MAP_FILES_ON_DEMAND
  if(RET_IS_NOT_OK(ret = gr_reactivate_mapping(sm->bg_images[layer]))) {
    debug(TM, "gr_reactivate_mapping() failed");
    return ret;
  }
#endif

  image_t * img_last = sm->bg_images[layer];

  for(zoom_i = 2; zoom_i <= max_factor; zoom_i*= 2) {
    image_t * img = NULL;

    debug(TM, "\tzoom = %d", zoom_i);
    
    width /= 2;
    height /= 2;
    
    img = scalmgr_create_scaled_image(sm, img_last, width, height, layer, zoom_i);
    assert(img != NULL);
    if(img == NULL) {
      scalmgr_destroy_scalings(sm);
      return RET_ERR;
    }

#ifdef MAP_FILES_ON_DEMAND
    if(RET_IS_NOT_OK(ret = gr_deactivate_mapping(img_last))) {
      debug(TM, "deactivate mapping");
      scalmgr_destroy_scalings(sm);
      return ret;
    }
#endif

    img_last = img;

    image_list_t * list_elem = scalmgr_create_list(layer, zoom_i, img);
    assert(list_elem != NULL);
    if(list_elem == NULL) {
      scalmgr_destroy_scalings(sm);
      return RET_ERR;
    }
    
    // add to list
    if(list != NULL) list_elem->next = list;
    list = list_elem;
  }

#ifdef MAP_FILES_ON_DEMAND
  if(RET_IS_NOT_OK(ret = gr_deactivate_mapping(img_last))) {
    debug(TM, "deactivate mapping");
    scalmgr_destroy_scalings(sm);
    return ret;
  }
  if(RET_IS_NOT_OK(ret = gr_deactivate_mapping(sm->bg_images[layer]))) return ret;
#endif
  
  sm->zoom_out_images = list;

  return RET_OK;
  
}

ret_t scalmgr_load_scalings(scaling_manager_t * sm) {
  ret_t ret;
  unsigned int layer;
  assert(sm != NULL);
  if(sm == NULL) return RET_INV_PTR;

  debug(TM, "Load scaled images.");

  for(layer = 0; layer < sm->num_layers; layer++) {

    if(RET_IS_NOT_OK(ret = scalmgr_load_scalings_for_layer(sm, sm->zoom_out_factor, layer)))
      return ret;

  }

  debug(TM, "Loading images done.");
  return RET_OK;
}
			    

ret_t scalmgr_map_files_for_layer(scaling_manager_t * sm, unsigned int layer) {
  image_list_t * ptr = NULL;
  ret_t ret;
  ptr = sm->zoom_out_images;
  while(ptr != NULL) {
    if(ptr->layer == layer) {
#ifdef MAP_FILES_ON_DEMAND
      if(RET_IS_NOT_OK(ret = gr_reactivate_mapping(ptr->image))) return ret;
#endif
    }
    ptr = ptr->next;
  }

  return RET_OK;
}

ret_t scalmgr_unmap_files_for_layer(scaling_manager_t * sm, unsigned int layer) {
  image_list_t * ptr = NULL;
  ret_t ret;
  ptr = sm->zoom_out_images;
  while(ptr != NULL) {
    if(ptr->layer == layer) {
#ifdef MAP_FILES_ON_DEMAND
      if(RET_IS_NOT_OK(ret = gr_deactivate_mapping(ptr->image))) return ret;
#endif
    }
    ptr = ptr->next;
  }

  return RET_OK;
}


/**
 * Recreate scaled images.
 */
ret_t scalmgr_recreate_scalings(scaling_manager_t * sm) {
  ret_t ret;
  unsigned int layer;
  assert(sm != NULL);
  if(sm == NULL) return RET_INV_PTR;

  // destroy current scaled images
  //if(RET_IS_NOT_OK(ret = scalmgr_destroy_scalings(sm))) return ret;

  for(layer = 0; layer < sm->num_layers; layer++) {
    debug(TM, "recreate scalings for layer %d", layer);
    if(RET_IS_NOT_OK(ret = scalmgr_recreate_scalings_for_layer(sm, layer))) return ret;
  }

  return RET_OK;
}


/**
 * Recreate scaled images for a layer.
 */
ret_t scalmgr_recreate_scalings_for_layer(scaling_manager_t * sm, unsigned int layer) {

  unsigned int zoom_i;
  assert(sm != NULL);
  assert(sm->bg_images[0] != NULL);
  if(sm == NULL || sm->bg_images[0] == NULL) return RET_INV_PTR;


  image_t * last_img = sm->bg_images[layer];

  for(zoom_i = 2; zoom_i <= sm->zoom_out_factor; zoom_i*= 2) {

    debug(TM, "recreate scalings for layer %d, zoom-out %d", layer, zoom_i);
    image_list_t * ptr = scalmgr_get_list_elem(sm->zoom_out_images, zoom_i, layer);
    if(ptr != NULL) {
      assert(ptr->image != NULL);

      unsigned int w = ptr->image->width;
      unsigned int h = ptr->image->height;
      gr_destroy_and_unlink(ptr->image);

      ptr->image = scalmgr_create_scaled_image(sm, last_img, w, h, layer, zoom_i);
      if(ptr->image == NULL) {
	scalmgr_destroy_scalings(sm);
	return RET_ERR;
      }

      last_img = ptr->image;
    }
    else {

      image_t * img = scalmgr_create_scaled_image(sm, last_img, 
						  last_img->width / 2, last_img->height / 2, 
						  layer, zoom_i);

      assert(img != NULL);
      if(img == NULL) return RET_ERR;

      image_list_t * list_elem = scalmgr_create_list(layer, zoom_i, img);
      assert(list_elem != NULL);
      if(list_elem == NULL) {
	scalmgr_destroy_scalings(sm);
	return RET_ERR;
      }

      list_elem->next = sm->zoom_out_images;
      sm->zoom_out_images = list_elem;

      last_img = list_elem->image;
    }

  }


  return RET_OK;
}

/** 
 * Define the scalings.
 */
ret_t scalmgr_set_scalings(scaling_manager_t * sm, unsigned int zoom_out_factor) {

  assert(sm != NULL);
  assert(sm->bg_images[0] != NULL);
  assert(zoom_out_factor >= 1);

  if(sm == NULL || sm->bg_images[0] == NULL) return RET_INV_PTR;
  if(zoom_out_factor == 0) return RET_ERR;

  sm->zoom_out_factor = zoom_out_factor;

  //return scalmgr_recreate_scalings(sm);
  return RET_OK;
}

/**
 * Get the maximum zoom out factor.
 * @return 0 on error, else the scaling factor
*/
unsigned int scalmgr_get_max_zoom_out_factor(scaling_manager_t * sm) {
  assert(sm != NULL);
  if(sm != NULL) {
    return sm->zoom_out_factor == 0 ? 1 : sm->zoom_out_factor;
  }
  return 0;
}
