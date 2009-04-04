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

#define TM "scaling_manager.c"

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


/**
 * Get a pointer to an image, that is scaled up or scaled down by a factor near to 
 * a given scaling factor.
 */
image_t * scalmgr_get_image(image_t * orig_img, double scaling) {
  assert(orig_img != NULL);
  if(orig_img == NULL) return NULL;
  
  return NULL;
}

ret_t scalmgr_destroy_scalings(scaling_manager_t * sm) {
  unsigned int i;
  ret_t ret;
  assert(sm != NULL);
  if(sm == NULL) return RET_INV_PTR;
  if(sm->zoom_out_factor >= 2) {
    for(i = 0; i < sm->zoom_out_factor - 2; i++) 
      if(RET_IS_NOT_OK(ret = gr_image_destroy(sm->zoom_out_images[i]))) return ret;
    free(sm->zoom_out_images);
    sm->zoom_out_images = NULL;
    sm->zoom_in_factor = 1;
  }

  if(sm->zoom_in_factor >= 2) {
    for(i = 0; i < sm->zoom_in_factor - 2; i++)
      if(RET_IS_NOT_OK(ret = gr_image_destroy(sm->zoom_in_images[i]))) return ret;
    free(sm->zoom_in_images);
    sm->zoom_in_images = NULL;
    sm->zoom_out_factor = 1;
  }

  return RET_OK;
}

/** 
 * Define the scalings.
 */
ret_t scalmgr_set_scalings(scaling_manager_t * sm, unsigned int zoom_out_factor, 
			  unsigned int zoom_in_factor) {
  unsigned int i;
  ret_t ret;
  struct stat stat_buf;

  assert(sm != NULL);
  assert(sm->bg_images[0] != NULL);
  assert(zoom_in_factor >= 1 && zoom_out_factor >= 1);

  if(sm == NULL || sm->bg_images[0] == NULL) return RET_INV_PTR;
  if(zoom_out_factor == 0 || zoom_in_factor == 0) return RET_ERR;

  // destroy current scaled images
  if(RET_IS_NOT_OK(ret = scalmgr_destroy_scalings(sm))) return ret;

  // create new scaling
  if(zoom_out_factor >= 2) {
    unsigned int width = sm->bg_images[0]->width;
    unsigned int height = sm->bg_images[0]->height;

    if((sm->zoom_out_images = (image_t **) malloc((zoom_out_factor-2) * sizeof(image_t *))) 
       == NULL) {
      return RET_MALLOC_FAILED;
    }

    for(i = 0; i < zoom_out_factor - 2; i++) {
      width /= 2;
      height /= 2;

      if((sm->zoom_out_images[i] = gr_create_image(width, height, IMAGE_TYPE_RGBA)) == NULL) {
	scalmgr_destroy_scalings(sm);
	return RET_ERR;
      }

      // map to file
      int layer;
      for(layer = 0; layer < sm->num_layers; layer++) {

	char filename[PATH_MAX];
	snprintf(filename, sizeof(filename), "scaled_layer_%02d.zout.%d.dat", layer, i + 2);
	
	int file_exists = stat(filename, &stat_buf);

	if(!RET_IS_OK(ret = gr_map_file(sm->zoom_out_images[layer], 
					sm->project_dir, filename))) {
	  debug(TM, "mapping failed: %s", filename);
	  scalmgr_destroy_scalings(sm);
	  return ret;
	}
	
	// scale
	if(file_exists != 0) { // does not exists
	  debug(TM, "scaling image");
	  if(RET_IS_NOT_OK(ret = gr_scale_image(sm->bg_images[layer], 
						sm->zoom_out_images[layer]))) {
	    debug(TM, "scaling failed: %s", filename);
	    scalmgr_destroy_scalings(sm);
	    return ret;
	  }
	}
	
      }
      
    }
  }
  
  sm->zoom_in_factor = zoom_in_factor;
  sm->zoom_out_factor = zoom_out_factor;

  return RET_OK;

}
