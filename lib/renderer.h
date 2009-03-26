#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <ft2build.h>
#include FT_FREETYPE_H

#include "globals.h"
#include "logic_model.h"
#include "graphics.h"
#include "grid.h"
#include "alignment_marker.h"

typedef struct renderer renderer_t;

// rendering params
struct render_params {
  image_t ** bg_images;
  logic_model_t * lmodel;
  grid_t * grid;
  alignment_marker_set_t * alignment_marker_set;

  // color switches
  uint32_t gate_pin_color;
  uint32_t gate_area_color;
  uint32_t wire_color;
  uint32_t il_up_color;
  uint32_t il_down_color;
  uint32_t grid_color;
  
  uint32_t marker_color_m1_up;
  uint32_t marker_color_m1_down;
  uint32_t marker_color_m2_up;
  uint32_t marker_color_m2_down;
  unsigned int alignment_marker_size;

  // for image algorithms
  /*  unsigned int threshold_col_separation;
  unsigned int pin_diameter;
  int match_horizontal_wires;
  int match_vertical_wires;

  uint32_t distance_to_color; */
};

typedef struct render_params render_params_t;


#define RENDERER_REGION_FUNC_PARAMS \
  renderer_t * const renderer,	    \
  image_t * dst_img,		    \
  unsigned int layer, \
  unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y

#define RENDERER_FUNC_PARAMS \
  RENDERER_REGION_FUNC_PARAMS, render_params_t * data_ptr

typedef int (*render_func_t)(renderer_t * const renderer,
			     image_t *, 
			     unsigned int, // layer
			     unsigned int, // min_x
			     unsigned int, // max_y
			     unsigned int, // max_x
			     unsigned int, // max_y
			     render_params_t *);

#define MAX_RENDERER_LAYER 30

struct renderer {
  render_func_t funcs[MAX_RENDERER_LAYER];
  void * data_ptr[MAX_RENDERER_LAYER];
  int rendering_enabled[MAX_RENDERER_LAYER];
  char * names[MAX_RENDERER_LAYER];

  FT_Library library;
  FT_Face face;
  FT_GlyphSlot slot;

  // x/y-positions are only stored to check, if we have to recalculate the step-arrays
  unsigned int last_screen_width, last_screen_height;
  unsigned int last_map_width, last_map_height;
  unsigned int * x_steps, * y_steps;
  double scaling_x;
  double scaling_y;

  int num; // number of rendering layers
};

renderer_t * renderer_create();
void renderer_destroy(renderer_t * const renderer);
void renderer_add_layer(renderer_t * const renderer, render_func_t function_ptr, void * data_ptr,
			int enabled, const char * const name);
void renderer_remove_last_layer(renderer_t * const renderer);

void renderer_toggle_render_func(renderer_t * const renderer, int slot_pos);
int renderer_get_num_render_func(renderer_t * const renderer);
char * renderer_get_name_render_func(renderer_t * const renderer, int slot_pos);
int renderer_render_func_enabled(renderer_t * const renderer, int slot_pos);

void renderer_initialize_params(render_params_t * rend);

void render_region(RENDERER_REGION_FUNC_PARAMS);


ret_t render_background(RENDERER_FUNC_PARAMS);
ret_t render_to_grayscale(RENDERER_FUNC_PARAMS);
ret_t render_color_similarity(RENDERER_FUNC_PARAMS);
ret_t render_gates(RENDERER_FUNC_PARAMS);
ret_t render_wires(RENDERER_FUNC_PARAMS);
ret_t render_vias(RENDERER_FUNC_PARAMS);
ret_t render_grid(RENDERER_FUNC_PARAMS);
ret_t render_alignment_markers(RENDERER_FUNC_PARAMS);

ret_t renderer_write_image(image_t * img, const char * const filename);

// helper

void vline(image_t * img, unsigned int x, unsigned int y, uint32_t col);
void hline(image_t * img, unsigned int x, unsigned int y, uint32_t col);

#endif
