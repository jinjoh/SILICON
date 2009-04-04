#ifndef __SCALING_MANAGER_H__
#define __SCALING_MANAGER_H__

#include "graphics.h"

typedef struct scaling_manager {
  unsigned int zoom_in_factor;
  unsigned int zoom_out_factor;

  int num_layers;
  char * project_dir;
  image_t ** bg_images;
  image_t ** zoom_out_images;
  image_t ** zoom_in_images;
} scaling_manager_t;

scaling_manager_t * scalmgr_create(int num_layers, image_t ** bg_images,
				   const char * const project_dir);
ret_t scalmgr_destroy(scaling_manager_t * sm);

ret_t scalmgr_set_scalings(scaling_manager_t * sm, unsigned int zoom_out_factor, 
			   unsigned int zoom_in_factor);

#endif
