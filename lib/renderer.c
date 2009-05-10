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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <ft2build.h>
#include <ctype.h>
#include <assert.h>

#include "graphics.h"
#include "renderer.h"
#include "globals.h"
#include "img_algorithms.h"
#include "logic_model.h"
#include "alignment_marker.h"
#include "quadtree.h"
//#include "font.h"

// #define FONTFILE "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
#define FONT_SIZE 9

#ifdef DEBUG
//#define RENDERER_MEASURE_TIME
#endif

typedef struct {
  renderer_t * const renderer;
  render_params_t * const render_params;
  image_t * dst_img;
  LM_OBJECT_TYPE object_type;
  unsigned int min_x;  // real
  unsigned int min_y;
  unsigned int max_x;
  unsigned int max_y;
} qtree_callback_params_t;

ret_t recalc_steps(renderer_t * renderer,
		   unsigned int map_min_x, unsigned int map_min_y, 
		   unsigned int map_max_x, unsigned int map_max_y,
		   double scaling_x, double scaling_y,
		   unsigned int dst_width, unsigned int dst_height) {

  if(renderer->last_screen_width != dst_width ||
     renderer->last_screen_height != dst_height ||
     renderer->last_map_width != map_max_x - map_min_x ||
     renderer->last_map_height != map_max_y - map_min_y ||
     renderer->last_rel_scaling_x != scaling_x ||
     renderer->last_rel_scaling_y != scaling_y ) {

    unsigned int i;

    renderer->last_screen_width =  dst_width;
    renderer->last_screen_height =  dst_height;
    renderer->last_map_width = map_max_x - map_min_x;
    renderer->last_map_height = map_max_y - map_min_y;
    renderer->last_rel_scaling_x = scaling_x;
    renderer->last_rel_scaling_y = scaling_y;
    
    if(renderer->x_steps) free(renderer->x_steps);
    if(renderer->y_steps) free(renderer->y_steps);

    if((renderer->x_steps = (unsigned int *) malloc(dst_width * sizeof(unsigned int))) == NULL)
      return RET_MALLOC_FAILED;

    if((renderer->y_steps = (unsigned int *) malloc(dst_height * sizeof(unsigned int))) == NULL) {
      free(renderer->x_steps);
      return RET_MALLOC_FAILED;
    }

    for(i = 0; i < dst_width; i++) renderer->x_steps[i] = (unsigned int)(i * scaling_x);
    for(i = 0; i < dst_height; i++) renderer->y_steps[i] = (unsigned int)(i * scaling_y);

  }

  return RET_OK;
}


renderer_t * renderer_create() {
  renderer_t * rend = (renderer_t *)malloc(sizeof(renderer_t));
  if(!rend) return NULL;
	
  memset(rend, 0, sizeof(renderer_t));

  char font_file[PATH_MAX];
  snprintf(font_file, PATH_MAX, "%s/FreeSans.ttf", getenv("DEGATE_HOME"));

  if(FT_Init_FreeType(&rend->library) ||
     FT_New_Face(rend->library, font_file, 0, &rend->face) ||
     FT_Set_Pixel_Sizes( rend->face, 0, FONT_SIZE)) {
    debug(TM, "Initializing font renderer failed");
    free(rend);
    return NULL;
  }
  rend->slot = rend->face->glyph;

  return rend;
}

void renderer_destroy(renderer_t * const renderer) {
  if(renderer) {
    int i;
    for(i = 0; i < renderer->num; i++) if(renderer->names[i]) free(renderer->names[i]);
    free(renderer);
  }
}


static inline uint32_t highlight_color(uint32_t col) {
  uint8_t r = MASK_R(col);
  uint8_t g = MASK_G(col);
  uint8_t b = MASK_B(col);
  uint8_t a = MASK_A(col);
  
  return MERGE_CHANNELS((((255-r)>>1) + r),
			(((255-g)>>1) + g),
			(((255-b)>>1) + b), (a < 128 ? 128 : a));
}

static inline uint32_t highlight_color_by_state(uint32_t col, int state) {
  if(state == SELECT_STATE_NOT) return col;
  else if(state == SELECT_STATE_DIRECT) return highlight_color(highlight_color(col));
  else if(state == SELECT_STATE_ADJ) return highlight_color(col);
  return col;
}

static inline uint32_t alpha_blend(uint32_t bg_col, uint32_t new_col) {
  uint8_t alpha = MASK_A(new_col);
  uint8_t r = ((0xff-alpha) * MASK_R(bg_col) + alpha * MASK_R(new_col)) >> 8;
  uint8_t g = ((0xff-alpha) * MASK_G(bg_col) + alpha * MASK_G(new_col)) >> 8;
  uint8_t b = ((0xff-alpha) * MASK_B(bg_col) + alpha * MASK_B(new_col)) >> 8;

  return MERGE_CHANNELS(r, g, b, 0xff);
}

void renderer_add_layer(renderer_t * const renderer, render_func_t function_ptr, 
			void * data_ptr, int enabled, const char * const name) {
  if(renderer) {
    renderer->funcs[renderer->num] = function_ptr;
    renderer->rendering_enabled[renderer->num] = enabled;
    renderer->data_ptr[renderer->num] = data_ptr;
    renderer->names[renderer->num] = strdup(name);
    renderer->num++;
  }
}

void renderer_remove_last_layer(renderer_t * const renderer) {
  if(renderer && renderer->num > 0) {
    renderer->num--;
    if(renderer->names[renderer->num] != NULL)
      free(renderer->names[renderer->num]);
  }
}

void renderer_toggle_render_func(renderer_t * const renderer, int slot_pos) {
  if(renderer && slot_pos < renderer->num) {
    renderer->rendering_enabled[slot_pos] = renderer->rendering_enabled[slot_pos] == 0 ? 1 : 0;
  }
}

int renderer_get_num_render_func(renderer_t * const renderer) {
  return renderer ? renderer->num : 0;
}

char * renderer_get_name_render_func(renderer_t * const renderer, int slot_pos) {
  if(renderer && slot_pos < renderer->num) {
    return renderer->names[slot_pos];
  }
  return NULL;
}

int renderer_render_func_enabled(renderer_t * const renderer, int slot_pos) {
  return (renderer && slot_pos < renderer->num) ? renderer->rendering_enabled[slot_pos] : 0;
}

void render_region(RENDERER_REGION_FUNC_PARAMS) {
  int i;
  if(!renderer) return;

#ifdef RENDERER_MEASURE_TIME
  clock_t start, finish;
  start = clock();
#endif

  if(renderer->rendering_enabled[0] == 0) {
    // if first render func is not enabled, memset the buffer
    gr_map_clear(dst_img);
  }
  for(i = 0; i < renderer->num; i++) {
    
    if(renderer->rendering_enabled[i]) {

      if(!RET_IS_OK((*(renderer->funcs[i]))(renderer, dst_img, layer, min_x, min_y, max_x, max_y,
					(render_params_t *)renderer->data_ptr[i])))
	debug(TM, "rendering failed: %s\n", renderer->names[i]);
    }
  }

#ifdef RENDERER_MEASURE_TIME
  finish = clock();
  debug(TM, "rendering time: %f ms", 1000*(double(finish - start)/CLOCKS_PER_SEC));
#endif
}

void renderer_initialize_params(render_params_t * rend) {
  // format: A B G R
  // low values for A = less transparency
  rend->gate_pin_color  = 0x7fb006b2; // pink
  rend->gate_area_color = 0xa0303030; // gray
  rend->grid_color      = 0x7fff00ff; // red

  rend->wire_color      = 0xffff1200; // blue

  rend->il_down_color     = 0xff0000ff; // red
  rend->il_up_color       = 0xff12ff00; // green

  rend->marker_color_m1_up =   0x7fa02020;
  rend->marker_color_m1_down = 0x7f20a020;
  rend->marker_color_m2_up =   0x7fa02020;
  rend->marker_color_m2_down = 0x7f20a020;
  rend->alignment_marker_size = 10;

  /*
  rend->threshold_col_separation = 110;
  rend->pin_diameter = 4;
  rend->match_horizontal_wires = TRUE;
  rend->match_vertical_wires = TRUE;	
  rend->distance_to_color = 0xff52a25c;
  */

  if(rend->grid != NULL) {
    rend->grid->offset_x = 0;
    rend->grid->offset_y = 0;
    rend->grid->dist_x = 1;
    rend->grid->dist_y = 1;
  }
}

/*
ret_t render_background_old(RENDERER_FUNC_PARAMS) {

  ret_t ret;
  unsigned int dst_x, dst_y;
  image_t * bg_img = data_ptr->bg_images[layer];
  if(!bg_img) return RET_INV_PTR;

  unsigned int bg_width = bg_img->width;
  unsigned int bg_height = bg_img->height;

  if(!RET_IS_OK(ret = recalc_steps(renderer, min_x, min_y, max_x, max_y,
				   dst_img->width, dst_img->height)))
    return ret;
		
  // clipped region
  unsigned int _dst_width = (unsigned int) MIN(dst_img->width, (bg_width - min_x)/renderer->scaling_x);
  unsigned int _dst_height = (unsigned int) MIN(dst_img->height, (bg_height - min_y)/renderer->scaling_y);

  gr_map_clear(dst_img);
  for(dst_y = 0; dst_y < _dst_height; dst_y++) {
    for(dst_x = 0; dst_x < _dst_width; dst_x++) {
      gr_copy_pixel_rgba(dst_img, dst_x, dst_y,
			 bg_img, renderer->x_steps[dst_x] + min_x, renderer->y_steps[dst_y] + min_y);
    }
  }

  return RET_OK;
}

*/

ret_t render_background(RENDERER_FUNC_PARAMS) {
  ret_t ret;

  double scaling_x = (max_x - min_x) / (double)dst_img->width;
  double scaling_y = (max_y - min_y) / (double)dst_img->height;
  image_t * bg_img = NULL;
  double bg_pre_scaling = 0;
  unsigned int dst_x, dst_y;

  //gr_map_clear(dst_img);
  //debug(TM, "scaling is %f", scaling_x);
  
  
  bg_img = scalmgr_get_image(data_ptr->scaling_manager, layer, scaling_x, &bg_pre_scaling);
  assert(bg_img != NULL);
  if(bg_img == NULL) return RET_ERR;

  //debug(TM, "scaling found %f\n\n", bg_pre_scaling);

  scaling_x /= bg_pre_scaling;
  scaling_y /= bg_pre_scaling;

  //debug(TM, "scaling related to already scaled image is %f\n\n", scaling_x);

  //unsigned int bg_width = bg_img->width;
  //unsigned int bg_height = bg_img->height;
  
  unsigned int bg_min_x = min_x / bg_pre_scaling;
  unsigned int bg_min_y = min_y / bg_pre_scaling;

  if(!RET_IS_OK(ret = recalc_steps(renderer, min_x, min_y, max_x, max_y,
				   scaling_x, scaling_y,
				   dst_img->width, dst_img->height)))
    return ret;

  unsigned int src_x, src_y;
  for(dst_y = 0; dst_y < dst_img->height; dst_y++) {
    src_y = bg_min_y + renderer->y_steps[dst_y];

    for(dst_x = 0; dst_x < dst_img->width; dst_x++) {
      src_x = bg_min_x + renderer->x_steps[dst_x];

      if(src_x < bg_img->width && src_y < bg_img->height)
	gr_copy_pixel_rgba(dst_img, dst_x, dst_y, bg_img, src_x, src_y);
      else
	gr_set_pixval(dst_img, dst_x, dst_y, 0);
    }
  }

 
  return RET_OK;
}




ret_t render_to_grayscale(RENDERER_FUNC_PARAMS) {
  return imgalgo_to_grayscale(dst_img);
}


/*
ret_t render_color_similarity(RENDERER_FUNC_PARAMS) {
  return imgalgo_to_color_similarity(dst_img, data_ptr->distance_to_color);
}
*/

// screen coords
ret_t draw_rectangle(image_t * dst_img, unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y, 
		     color_t fill_color, color_t frame_color, unsigned int frame_size) {
  unsigned int x, y,
    x_a = min_x + frame_size, 
    x_b = max_x > frame_size ? max_x - frame_size : 0,
    y_a = min_y + frame_size,
    y_b = max_y > frame_size ? max_y - frame_size : 0;

  if(max_x >= dst_img->width || max_y >= dst_img->height) {
    debug(TM, "rectangle is out of image area");
    return RET_ERR;
  }

  
  for(y = min_y; y < max_y; y++) {

      for(x = min_x; x < max_x; x++) {
	uint32_t pix = gr_get_pixval(dst_img, x, y);
	color_t col = 
	  (x < x_a || 
	   x >= x_b || 
	   y < y_a || 
	   y >= y_b) ? frame_color : fill_color;
	gr_set_pixval(dst_img, x, y, alpha_blend(pix, col));
      }
    }
  return RET_OK;
}

ret_t draw_circle(image_t * dst_img, unsigned int x, unsigned int y, unsigned int diameter, 
		  uint32_t color) {

  unsigned int _x, _y;
  unsigned int radius = diameter >> 1;
  unsigned int radius2 = radius << 1;

  unsigned int x_min = x > radius ? x - radius : 0;
  unsigned int x_max = x + radius < dst_img->width ? x + radius : dst_img->width;
  unsigned int y_min = y > radius ? y - radius : 0;
  unsigned int y_max = y + radius < dst_img->height ? y + radius : dst_img->height;

  for(_y = y_min; _y < y_max; _y++) {
    unsigned int dy2 = (_y > y ? _y - y : y - _y);
    dy2 = dy2 * dy2;

    for(_x = x_min; _x < x_max; _x++) {
      unsigned int dx = (_x > x ? _x - x : x - _x);

      if(dx * dx + dy2 <= radius2) {
	uint32_t pix = gr_get_pixval(dst_img, _x, _y);
	gr_set_pixval(dst_img, _x, _y, alpha_blend(pix, color));
      }
    }
  }
  return RET_OK;
}


ret_t draw_line(image_t * dst_img, 
		unsigned int min_x, unsigned int min_y,
		unsigned int max_x, unsigned int max_y,
		unsigned int wire_diameter, uint32_t color) {

  // bresenham-copy'n'paste from wikipedia

  int x, y, t, dx, dy, incx, incy, pdx, pdy, ddx, ddy, es, el, err, i;
  //unsigned int visible_x = 0, visible_y = 0;
  uint32_t pix;
  /* Entfernung in beiden Dimensionen berechnen */
  dx = max_x - min_x;
  dy = max_y - min_y;
  
  /* Vorzeichen des Inkrements bestimmen */
  incx = SIGNUM(dx);
  incy = SIGNUM(dy);
  if(dx<0) dx = -dx;
  if(dy<0) dy = -dy;
  
  /* feststellen, welche Entfernung größer ist */
  if (dx>dy) {
    /* x ist schnelle Richtung */
    pdx=incx; pdy=0;    /* pd. ist Parallelschritt */
    ddx=incx; ddy=incy; /* dd. ist Diagonalschritt */
    es =dy;   el =dx;   /* Fehlerschritte schnell, langsam */
  } 
  else {
    /* y ist schnelle Richtung */
    pdx=0;    pdy=incy; /* pd. ist Parallelschritt */
    ddx=incx; ddy=incy; /* dd. ist Diagonalschritt */
    es =dx;   el =dy;   /* Fehlerschritte schnell, langsam */
  }
  
  /* Initialisierungen vor Schleifenbeginn */
  x = min_x;
  y = min_y;
  err = el/2;
  
  if(x>0 && y>0 && x < (int)dst_img->width && y < (int)dst_img->height) {
    pix = gr_get_pixval(dst_img, x, y);
    gr_set_pixval(dst_img, x, y, alpha_blend(pix, color));
  }

  /* Pixel berechnen */
  for(t=0; t<el; ++t) {/* t zaehlt die Pixel, el ist auch Anzahl */
    
    /* Aktualisierung Fehlerterm */
    err -= es; 
    if(err<0) {
      /* Fehlerterm wieder positiv (>=0) machen */
      err += el;
      /* Schritt in langsame Richtung, Diagonalschritt */
      x += ddx;
      y += ddy;
    } 
    else {
      /* Schritt in schnelle Richtung, Parallelschritt */
      x += pdx;
      y += pdy;
    }
    
    if (dx>dy) {
      // x schnell
      for(i = y - (int)(wire_diameter >> 1); i < y + (int)(wire_diameter >> 1); i++)
	if(x>0 && i>0 && x < (int)dst_img->width && i < (int)dst_img->height) {
	  pix = gr_get_pixval(dst_img, x, i);
	  gr_set_pixval(dst_img, x, i, alpha_blend(pix, color));
	}
    }
    else {
      for(i = x - (int)(wire_diameter >> 1); i < x + (int)(wire_diameter >> 1); i++)
	if(i>0 && y>0 && i < (int)dst_img->width && y < (int)dst_img->height) {
	  pix = gr_get_pixval(dst_img, i, y);
	  gr_set_pixval(dst_img, i, y, alpha_blend(pix, color));
	}
    }
  }

  return RET_OK;
}

			  
ret_t draw_string(renderer_t * renderer, render_params_t * render_params, image_t * dst_img,
		  const char * const text, unsigned int x, unsigned int y) {

  unsigned int i, _x, _y, x_offs = 0;

  for(i = 0; i < strlen(text); i++) {

    int glyph_index = FT_Get_Char_Index(renderer->face, islower(text[i]) ? toupper(text[i]) : text[i]);
    if(FT_Load_Glyph(renderer->face, glyph_index, FT_LOAD_DEFAULT)) return RET_ERR;
    if(FT_Render_Glyph(renderer->face->glyph, FT_RENDER_MODE_NORMAL)) return RET_ERR;

    for (_x=0; _x < (unsigned)renderer->slot->bitmap.width; _x++) {
      for (_y=0; _y < (unsigned)renderer->slot->bitmap.rows; _y++) {

	unsigned int __x = x + _x + x_offs;
	unsigned int __y = y + _y;

	int byte_index = (_x + _y* renderer->slot->bitmap.pitch);

	if(__x < dst_img->width && __y < dst_img->height) {
	  unsigned char f = (renderer->slot->bitmap.buffer[byte_index]);
	  if(f) {
	    uint32_t col = MERGE_CHANNELS(255, 255, 255, f);
	    uint32_t pix = gr_get_pixval(dst_img, __x, __y);
	    gr_set_pixval(dst_img, __x, __y, alpha_blend(pix, col));
	  }

	}
      }
    }
    x_offs += renderer->slot->bitmap.width + 2;
  }
  return RET_OK;
}

			
ret_t render_gate(renderer_t * renderer, render_params_t * render_params, image_t * dst_img, lmodel_gate_t * gate,
		  unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y) {


  if(gate->max_x < min_x || gate->min_x > max_x ||
     gate->max_y < min_y || gate->min_y > max_y) return RET_OK;

  ret_t ret;
  double scaling_x = (max_x - min_x) / (double)dst_img->width;
  double scaling_y = (max_y - min_y) / (double)dst_img->height;

  unsigned int screen_min_x = gate->min_x > min_x ? (unsigned int)((gate->min_x - min_x) / scaling_x) : 0;
  unsigned int screen_min_y = gate->min_y > min_y ? (unsigned int)((gate->min_y - min_y) / scaling_y) : 0;
  unsigned int screen_max_x = gate->max_x < max_x ? (unsigned int)((gate->max_x - min_x) / scaling_x) : dst_img->width - 1;
  unsigned int screen_max_y = gate->max_y < max_y ? (unsigned int)((gate->max_y - min_y) / scaling_y) : dst_img->height - 1;

  // render filled rectangle
  color_t fill_col = gate->gate_template != NULL ? gate->gate_template->fill_color : 0;
  color_t frame_col = gate->gate_template != NULL ? gate->gate_template->frame_color : 0;
  if(fill_col == 0) fill_col = render_params->gate_area_color;
  if(frame_col == 0) frame_col = fill_col;

  draw_rectangle(dst_img, screen_min_x, screen_min_y, screen_max_x, screen_max_y, 
		 highlight_color_by_state(fill_col, gate->is_selected), 
		 highlight_color_by_state(frame_col, gate->is_selected),
		 MIN(MAX(lrint(2.5 / scaling_x), 1), 3)
		 );

  if(gate->name && screen_max_x - screen_min_x > strlen(gate->name) * FONT_SIZE) {
    draw_string(renderer, render_params, dst_img,
		gate->name, screen_min_x + 5, screen_min_y + 2*5 + FONT_SIZE);
  }

  // render names for type and instance
  if(gate->gate_template && gate->gate_template->short_name &&
     (screen_max_x - screen_min_x > strlen(gate->gate_template->short_name) * FONT_SIZE)) {

    draw_string(renderer, render_params, dst_img,
		gate->gate_template->short_name, screen_min_x + 5, screen_min_y + 5 );
      
    unsigned int width  = gate->gate_template->master_image_max_x - gate->gate_template->master_image_min_x;
    unsigned int height = gate->gate_template->master_image_max_y - gate->gate_template->master_image_min_y;

    lmodel_gate_port_t * ports = gate->ports;

    while(ports != NULL) {
      lmodel_gate_template_port_t * tmpl_port = ports->tmpl_port;

      if(tmpl_port && tmpl_port->relative_x_coord != 0 && tmpl_port->relative_y_coord != 0) {
	unsigned int 
	  x = tmpl_port->relative_x_coord, 
	  y = tmpl_port->relative_y_coord;
	
	switch(gate->template_orientation) {
	case LM_TEMPLATE_ORIENTATION_UNDEFINED:
	  break;
	case LM_TEMPLATE_ORIENTATION_NORMAL:
	  break;
	case LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN:
	  y = height - y;
	  break;
	case LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT:
	  x = width - x;
	  break;
	case LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH:
	  x = width - x;
	  y = height - y;
	  break;
	}
	
	x += gate->min_x;
	y += gate->min_y;
	
	if(gate->template_orientation != LM_TEMPLATE_ORIENTATION_UNDEFINED && 
	   x >= min_x && x < max_x && y >= min_y && y < max_y) {
	  x -= min_x;
	  y -= min_y;
	  x = (double)x / scaling_x;
	  y = (double)y / scaling_y;
	  
	  unsigned int port_size = (double)tmpl_port->diameter / scaling_x;

	  color_t port_color = tmpl_port->color == 0 ? render_params->il_down_color : tmpl_port->color;
	  port_color = highlight_color_by_state(port_color, ports->is_selected);

	  if(RET_IS_NOT_OK(ret = draw_circle(dst_img, 
					     x, 
					     y, 
					     ports->is_selected ? (port_size << 2) : port_size + 1,
					     port_color)))
	    return ret;
	  
	  if(tmpl_port->port_name && x + strlen(tmpl_port->port_name) * FONT_SIZE < screen_max_x) {
	    draw_string(renderer, render_params, dst_img, tmpl_port->port_name, x + 5, y + 5 );
	  }
	}
      }
      ports = ports->next;
    }
  }

  return RET_OK;
}


bool liang_barsky_clip_test( double nDenom, double nNumerator, double * io_rTE, double * io_rTL ) {
  double t;
  if(nDenom > 0) {
    t = nNumerator / nDenom;
    if(t > *io_rTL) return false;
    else if(t > *io_rTE) *io_rTE = t;
  } 
  else if(nDenom < 0) {
    t = nNumerator / nDenom;
    if(t < *io_rTE) return false;
    else *io_rTL = t;
  }
  else if(nNumerator > 0) return false;

  return true;
}

bool liang_barsky_clipping( double from_x, 
			    double from_y,
			    double to_x,
			    double to_y,
			    double clip_to_min_x, double clip_to_min_y, double clip_to_max_x, double clip_to_max_y,
			    double * clipped_from_x, 
			    double * clipped_from_y,
			    double * clipped_to_x,
			    double * clipped_to_y) {

  double nDX = to_x - from_x;
  double nDY = to_y - from_y;

  if(nDX == 0 && nDY == 0) {
    //return rClipRect.isInside( io_rStart );
    return false;
  }
  else {
    double nTE = 0.0;
    double nTL = 1.0;
    if(liang_barsky_clip_test(nDX, clip_to_min_x - from_x, &nTE, &nTL)) {// inside wrt. left edge
      if(liang_barsky_clip_test(-nDX, from_x - clip_to_max_x, &nTE, &nTL)) { // inside wrt. right edge
	if(liang_barsky_clip_test(nDY, clip_to_min_y - from_y, &nTE, &nTL)) {// inside wrt. bottom edge
	  if(liang_barsky_clip_test(-nDY, from_y - clip_to_max_y, &nTE, &nTL)) {// inside wrt. top edge
		   
	    // compute actual intersection points,
	    // if nTL has changed
	    if( nTL < 1.0 ) {
	      *clipped_to_x =  from_x + nTL * nDX;
	      *clipped_to_y = from_y + nTL * nDY;
	    }

	    // compute actual intersection points,
	    // if nTE has changed
	    if( nTE > 0.0 ) {
	      *clipped_from_x = from_x + nTE * nDX;
	      *clipped_from_y = from_y + nTE * nDY;
	    }

	    // line is (at least partially) visible
	    return true;
	  }
	}
      }
    }
  }

 return false;
}


ret_t render_wire(renderer_t * renderer, render_params_t * render_params, image_t * dst_img, lmodel_wire_t * wire,
		  unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y) {

  double clipped_from_x = (double)wire->from_x - (double)min_x;
  double clipped_from_y = (double)wire->from_y - (double)min_y;
  double clipped_to_x = (double)wire->to_x -  (double)min_x;
  double clipped_to_y = (double)wire->to_y - (double)min_y;


  if(liang_barsky_clipping(clipped_from_x, 
			   clipped_from_y,
			   clipped_to_x,
			   clipped_to_y,
			   0, 0, max_x - min_x, max_y - min_y,
			   &clipped_from_x, 
			   &clipped_from_y,
			   &clipped_to_x,
			   &clipped_to_y)) {

    double scaling_x = (max_x - min_x) / (double)dst_img->width;
    double scaling_y = (max_y - min_y) / (double)dst_img->height;

    double dia = (double)wire->diameter / scaling_x;
    if(dia > 1) {
      clipped_from_x /= scaling_x;
      clipped_from_y /= scaling_y;
      clipped_to_x /= scaling_x;
      clipped_to_y /= scaling_y;

      draw_line(dst_img, 
		clipped_from_x,
		clipped_from_y,
		clipped_to_x,
		clipped_to_y,
		dia,  highlight_color_by_state(render_params->wire_color, wire->is_selected));
     
      if(wire->is_selected && wire->name && 
	 wire->from_x > min_x && wire->from_x < max_x &&
	 wire->from_y > min_y && wire->from_y < max_y ) {

	draw_string(renderer, render_params, dst_img,
		    wire->name, clipped_from_x + 5, clipped_from_y + 5 );
      }
    }

  }

  return RET_OK;
}

ret_t render_via(renderer_t * renderer, render_params_t * render_params, image_t * dst_img, lmodel_via_t * via,
		 unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y) {

  if(via->x > min_x && via->x < max_x &&
     via->y > min_y && via->y < max_y) {

    double scaling_x = (max_x - min_x) / (double)dst_img->width;
    double diameter_on_screen = (double)(via->diameter) / scaling_x;

    if(diameter_on_screen > 1) {
      double scaling_y = (max_y - min_y) / (double)dst_img->height;
      unsigned int screen_x = via->x > min_x ? (unsigned int)((via->x - min_x) / scaling_x) : 0;
      unsigned int screen_y = via->y > min_y ? (unsigned int)((via->y - min_y) / scaling_y) : 0;
      
      unsigned int via_size = (double)via->diameter / scaling_x;

      if(via->is_selected) via_size <<= 2;
      
  
      // render filled circle
      uint32_t col = via->direction == LM_VIA_UP ? render_params->il_up_color : render_params->il_down_color;
      col = highlight_color_by_state(col, via->is_selected);

      draw_circle(dst_img, screen_x, screen_y, via_size + 1, col);
    }
  }

  return RET_OK;
}

ret_t cb_render_object(quadtree_t * qtree, qtree_callback_params_t * data_ptr) {

  quadtree_object_t * object = qtree->objects;;
  ret_t ret;

  while(object != NULL) {
    switch(object->object_type) {
    case LM_TYPE_GATE:
      if(data_ptr->object_type == LM_TYPE_GATE) { // are we interested in rendering this?
	lmodel_gate_t * gate = (lmodel_gate_t *) (object->object);
	if(RET_IS_NOT_OK(ret = render_gate(data_ptr->renderer, data_ptr->render_params, data_ptr->dst_img, gate,
					   data_ptr->min_x, data_ptr->min_y, data_ptr->max_x, data_ptr->max_y))) return ret;
      }
      break;
    case LM_TYPE_WIRE:
      if(data_ptr->object_type == LM_TYPE_WIRE) { // are we interested in rendering this?
	lmodel_wire_t * wire = (lmodel_wire_t *) (object->object);
	if(RET_IS_NOT_OK(ret = render_wire(data_ptr->renderer, data_ptr->render_params, data_ptr->dst_img, wire,
					   data_ptr->min_x, data_ptr->min_y, data_ptr->max_x, data_ptr->max_y))) return ret;
      }
      break;
    case LM_TYPE_VIA:
      if(data_ptr->object_type == LM_TYPE_VIA) { // are we interested in rendering this?
	lmodel_via_t * via = (lmodel_via_t *) (object->object);
	if(RET_IS_NOT_OK(ret = render_via(data_ptr->renderer, data_ptr->render_params, data_ptr->dst_img, via,
					  data_ptr->min_x, data_ptr->min_y, data_ptr->max_x, data_ptr->max_y))) return ret;
      }
      break;
    default:
      debug(TM, "unknown object type");
      return RET_ERR;
    }

    object = object->next;
  }
  return RET_OK;
}


ret_t render_gates(RENDERER_FUNC_PARAMS) {
  int l = -1;

  switch(data_ptr->lmodel->layer_type[layer]) {
  case LM_LAYER_TYPE_LOGIC:
    l = layer;
    break;
  default:
    l = lmodel_get_layer_num_by_type(data_ptr->lmodel, LM_LAYER_TYPE_LOGIC);
  }

  if(l != -1) {
    qtree_callback_params_t params = {renderer, data_ptr, dst_img, LM_TYPE_GATE, min_x, min_y, max_x, max_y};
    quadtree_traverse_func_t cb_func = (quadtree_traverse_func_t) &cb_render_object;
    //quadtree_traverse_downto_bbox(data_ptr->lmodel->root[l], min_x, min_y,max_x, max_y, cb_func, &params);
    quadtree_traverse_complete_within_region(data_ptr->lmodel->root[l], min_x, min_y,max_x, max_y, cb_func, &params);
  }
  return RET_OK;
}


ret_t render_wires(RENDERER_FUNC_PARAMS) {

  qtree_callback_params_t params = {renderer, data_ptr, dst_img, LM_TYPE_WIRE, min_x, min_y, max_x, max_y};
  quadtree_traverse_func_t cb_func = (quadtree_traverse_func_t) &cb_render_object;
  quadtree_traverse_complete_within_region(data_ptr->lmodel->root[layer], min_x, min_y,max_x, max_y, cb_func, &params);

  return RET_OK;
}

ret_t render_vias(RENDERER_FUNC_PARAMS) {

  qtree_callback_params_t params = {renderer, data_ptr, dst_img, LM_TYPE_VIA, min_x, min_y, max_x, max_y};
  quadtree_traverse_func_t cb_func = (quadtree_traverse_func_t) &cb_render_object;
  quadtree_traverse_complete_within_region(data_ptr->lmodel->root[layer], min_x, min_y,max_x, max_y, cb_func, &params);

  return RET_OK;
}

void renderer_draw_marker(image_t * dst_img, 
			  unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y,
			  unsigned int screen_x, unsigned int screen_y,
			  uint32_t marker_color, unsigned int marker_size) {

  unsigned int y, x;
  unsigned int marker_radius = marker_size >> 1;

  for(y = screen_y > marker_radius ? screen_y - marker_radius : 0 ; 
      y < MIN(screen_y + marker_radius, dst_img->height); y++) {

      for(x = screen_x > marker_radius ? screen_x - marker_radius : 0 ; 
	  x < MIN(screen_x + marker_radius, dst_img->width); x++) {
	uint32_t pix = gr_get_pixval(dst_img, x, y);
	gr_set_pixval(dst_img, x, y, alpha_blend(pix, marker_color));
      }
    }
  
}

#define RENDER_ALIGNMENT_MARKER(m_type, m_var, m_col, alignment_marker_size) \
  alignment_marker * m_var = amset_get_marker(data_ptr->alignment_marker_set, layer, m_type); \
  if(m_var && \
     m_var->x > min_x && m_var->x < max_x && \
     m_var->y > min_y && m_var->y < max_y) \
    renderer_draw_marker(dst_img, min_x, min_y, max_x, max_y, \
			 (m_var->x - min_x) / scaling_x, (m_var->y - min_y) / scaling_y, \
			 m_col, alignment_marker_size);

ret_t render_alignment_markers(RENDERER_FUNC_PARAMS) {
  double scaling_x =  (max_x - min_x) / (double)dst_img->width;
  double scaling_y =  (max_y - min_y) / (double)dst_img->height;

  unsigned int m_size = data_ptr->alignment_marker_size;

  RENDER_ALIGNMENT_MARKER(MARKER_TYPE_M1_UP, m1_up, data_ptr->marker_color_m1_up, m_size);
  RENDER_ALIGNMENT_MARKER(MARKER_TYPE_M1_DOWN, m1_down, data_ptr->marker_color_m1_down, m_size);
  RENDER_ALIGNMENT_MARKER(MARKER_TYPE_M2_UP, m2_up, data_ptr->marker_color_m2_up, m_size);
  RENDER_ALIGNMENT_MARKER(MARKER_TYPE_M2_DOWN, m2_down, data_ptr->marker_color_m2_down, m_size);
  return RET_OK;
}


ret_t render_regular_grid(RENDERER_FUNC_PARAMS) {
  unsigned int dst_x = 0, dst_y = 0;
  double dbl_dst_x, dbl_dst_y;

  grid_t * grid = data_ptr->grid;
  if(grid == NULL) return RET_OK;

  double scaling_x =  (max_x - min_x) / (double)dst_img->width;
  double scaling_y =  (max_y - min_y) / (double)dst_img->height;

  if(grid->dist_x < 1 || grid->dist_y < 1) return RET_OK;

  unsigned int screen_offs_x;
  unsigned int screen_offs_y;

  if(grid->offset_y >= min_y)
    screen_offs_y = (unsigned int) ((grid->offset_y - min_y)/ scaling_y);
  else {
    int n = (int)((min_y - grid->offset_y) / grid->dist_y);
    screen_offs_y = (unsigned int) ((grid->dist_y - (min_y - grid->offset_y - n * grid->dist_y))/ scaling_y);
  }

  if(grid->offset_x >= min_x)
    screen_offs_x = (unsigned int) ((grid->offset_x - min_x)/ scaling_x);
  else {
    int n = (int)((min_x - grid->offset_x) / grid->dist_x);
    screen_offs_x = (unsigned int) ((grid->dist_x - (min_x - grid->offset_x - n * grid->dist_x))/ scaling_x);
  }

  if(grid->vertical_lines_enabled && scaling_x > 0 && grid->dist_x / scaling_x >= 2) {
    for(dbl_dst_x = screen_offs_x; dbl_dst_x < dst_img->width - 1; dbl_dst_x += (grid->dist_x / scaling_x)) {
      dst_x =  lrint(dbl_dst_x);
      vline(dst_img, dst_x, 0, data_ptr->grid_color);
    }
  }
  
  if(grid->horizontal_lines_enabled && scaling_y > 0 && grid->dist_y / scaling_y >= 2) {
    for(dbl_dst_y = screen_offs_y; dbl_dst_y < dst_img->height - 1; dbl_dst_y += (grid->dist_y / scaling_y)) {
      dst_y =  lrint(dbl_dst_y);
      hline(dst_img, 0, dst_y, data_ptr->grid_color);
    }
  }
  return RET_OK;
}

ret_t render_unregular_grid(RENDERER_FUNC_PARAMS) {
  unsigned int i;
  grid_t * grid = data_ptr->grid;
  if(grid == NULL) return RET_OK;

  
  double scaling_x =  (max_x - min_x) / (double)dst_img->width;
  double scaling_y =  (max_y - min_y) / (double)dst_img->height;

  /* horizontal lines */
  if(grid->uhg_enabled == 1) {
    for(i = 0; i < grid->num_uhg_entries; i++) {
      unsigned int offset = grid->uhg_offsets[i];
      if(offset >= min_y && offset <= max_y) {
	unsigned int dst_y = (double)(offset - min_y) / scaling_y;
	hline(dst_img, 0, dst_y, data_ptr->grid_color);
      }
    }
  }

  /* vertical lines */
  if(grid->uvg_enabled == 1) {
    for(i = 0; i < grid->num_uvg_entries; i++) {
      unsigned int offset = grid->uvg_offsets[i];
      if(offset >= min_x && offset <= max_x) {
	unsigned int dst_x = (double)(offset - min_x) / scaling_x;
	vline(dst_img, dst_x, 0, data_ptr->grid_color);
      }
    }
  }

  return RET_OK;
}

ret_t render_grid(RENDERER_FUNC_PARAMS) {
  if(data_ptr->grid != NULL) {
    switch(data_ptr->grid->grid_mode) {
    case USE_REGULAR_GRID:
      return render_regular_grid(renderer, dst_img, layer, min_x, min_y, max_x, max_y, data_ptr); 
      break;
    case USE_UNREGULAR_GRID:
      return render_unregular_grid(renderer, dst_img, layer, min_x, min_y, max_x, max_y, data_ptr);
      break;
    default:
      return RET_ERR;
    }
  }
  else return RET_INV_PTR;
}

void vline(image_t * img, unsigned int x, unsigned int y, uint32_t col) {
  unsigned int _y;
  for(_y = y; _y < img->height; _y++) {
    uint32_t pix = gr_get_pixval(img, x, _y);
    gr_set_pixval(img, x, _y, alpha_blend(pix, col));
  }
}

void hline(image_t * img, unsigned int x, unsigned int y, uint32_t col) {
  unsigned int _x;
  for(_x = x; _x < img->width; _x++) {
    uint32_t pix = gr_get_pixval(img, _x, y);
    gr_set_pixval(img, _x, y, alpha_blend(pix, col));
  }	
}

/*
 */
ret_t renderer_write_image(image_t * img, const char * const filename) {

#define PIXELS_IN_BUFFER 1000

  int fd;
  uint8_t tmp[3*PIXELS_IN_BUFFER];
  char tmp2[100];
  unsigned int x, y, i = 1;
  uint8_t * ptr_dst = tmp;

  fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0600);
  if(fd == -1) return RET_ERR;

  snprintf(tmp2, sizeof(tmp2), "P6\n%d %d\n255\n", img->width, img->height);
  if(write(fd, tmp2, strlen(tmp2)) != (ssize_t)strlen(tmp2)) {
    close(fd);
    return RET_ERR;
  }

  ptr_dst = tmp;

  for(y = 0; y < img->height; y++)
    for(x = 0; x < img->width; x++, i++) {

      uint32_t pix = gr_get_pixval(img, x, y);

      *ptr_dst++ = MASK_R(pix);
      *ptr_dst++ = MASK_G(pix);
      *ptr_dst++ = MASK_B(pix);

      if(i % PIXELS_IN_BUFFER == 0) {
	if(write(fd, tmp, sizeof(tmp)) != sizeof(tmp)) {
	  close(fd);
	  return RET_ERR;
	}
	ptr_dst = tmp;
      }
    }

  if(img->width * img->height % PIXELS_IN_BUFFER > 0) {
    ssize_t s = 3*((img->width * img->height) % PIXELS_IN_BUFFER);
    if(write(fd, tmp, s) != s) { 
      close(fd);
      return RET_ERR;
    }
  }
  
  close(fd);
  return RET_OK;
}
