#ifndef __PROJECT_H__
#define __PROJECT_H__

#include "logic_model.h"
#include "graphics.h"
#include "renderer.h"
#include "alignment_marker.h"
#include "grid.h"
#include "scaling_manager.h"

struct project {

  unsigned int width, height;
  int num_layers;
  
  char * project_dir;
  
  image_t ** bg_images;
  scaling_manager_t * scaling_manager;

  logic_model_t * lmodel;
  
  //  gate_template_set_t * gset; // set of templates

  int current_layer;

  unsigned int pin_diameter;
  unsigned int wire_diameter;
  unsigned int lambda;

  grid_t grid;

  alignment_marker_set_t * alignment_marker_set;
};

typedef struct project project_t;

project_t * project_create(const char * project_dir, unsigned int width, unsigned int height, int num_layers);
ret_t project_destroy(project_t * project);
ret_t project_init_directory(const char * const directory, int enable_mkdir);
project_t * project_load(const char * const project_dir /*, render_params_t * const render_params*/);

ret_t project_map_background_memfiles(project_t * const project);

ret_t project_save(const project_t * const project);

ret_t project_cleanup(const char * const project_dir);

#endif
