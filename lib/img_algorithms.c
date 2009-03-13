// XXX assertions needed

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "globals.h"
#include "graphics.h"
#include "logic_model.h"
#include "img_algorithms.h"
#include "grid.h"

#define COL_UNDEF 255
#define COL_PIN_DETECTED 254
#define COL_TMP 253
#define COL_DONE 252

/**
 * convert a RGB value to a hue value
 */
uint8_t rgb_to_h(uint8_t red, uint8_t green, uint8_t blue) {

  int max = MAX(red, MAX(green, blue));
  int min = MIN(red, MIN(green, blue));
  double delta = max - min;
  double h = 0;

  if(max == min) h = 0;
  else if(max == red) h = 60 *  (green-blue)/delta;
  else if(max == green) h = 60 * (2+(blue-red)/delta);
  else if(max == blue) h = 60 * (4 + (red-green)/delta);
  if(h < 0) h+=360;

  h *= 255.0/360.0;
  return (uint8_t)rint(h);
}

/** 
 * Convert pixels of a RGBA image to greyscaled pixels.
 * Afterwards the model is still RGBA, but with R == G == B
 */
ret_t imgalgo_to_grayscale(image_t * img) {
  unsigned int x, y;

  if(img->image_type != IMAGE_TYPE_RGBA) return RET_ERR;

  for(y = 0; y < img->height; y++) {
    for(x = 0; x < img->width; x++) {
      uint8_t gs = gr_get_greyscale_pixval(img, x, y);
      gr_set_greyscale_pixval(img, x, y, gs);
    }
  }
  return RET_OK;
}

/**
 * Transform an image to a hue-distance image.
 *
 * @param img
 * @param color an RGB value as reference
 */
ret_t imgalgo_to_color_similarity(image_t * img, uint32_t color) {
  unsigned int dst_x, dst_y;

  if(img->image_type != IMAGE_TYPE_RGBA) return RET_ERR;

  uint8_t dst_hue = rgb_to_h(MASK_R(color), MASK_G(color), MASK_B(color));
  printf("dst_hue = %d\n", dst_hue);

  for(dst_y = 0; dst_y < img->height; dst_y++) {
    for(dst_x = 0; dst_x < img->width; dst_x++) {			
      uint32_t * pix = (uint32_t *)mm_get_ptr(img->map, dst_x, dst_y);
      uint8_t h = rgb_to_h(MASK_R(*pix), MASK_G(*pix), MASK_B(*pix));
      uint8_t pix_val = dst_hue > h ? dst_hue - h : h - dst_hue;
      *pix = MERGE_CHANNELS(pix_val, pix_val, pix_val, 0xff);
    }
  }
  return RET_OK;
}

/** 
 * Separate a grayscale image given as RGB plane by a threshold value. Each pixel is replaced with
 * (0xff, 0xff, 0xff, 0xff) or (0, 0, 0, 0) depending on the red-channel from input data.
 */
ret_t imgalgo_separate_by_threshold(image_t * img, unsigned int threshold) {
  unsigned int x, y;
  for(y = 0; y < img->height; y++) {
    for(x = 0; x < img->width; x++) {			

      uint8_t pix_val = gr_get_greyscale_pixval(img, x, y);
      gr_set_greyscale_pixval(img, x, y, pix_val > (uint8_t)threshold ? 0xff : 0);
    }
  }
  return RET_OK;
}


// flood_trace() is a modified and bug fixed implementation of the seed fill algorithm by Paul Heckbert.
// The original was published in "Graphics Gems", Academic Press, 1990.

#define GetPixelFF(x, y) (MASK_R(*(uint32_t *)mm_get_ptr(img->map, x, y)))
#define SetPixelFF(x, y, col) { *(uint32_t *)mm_get_ptr(img->map, x, y) = MERGE_CHANNELS(col, col, col, 0xff);}

typedef struct { unsigned int x1, x2, y, dy; } LINESEGMENT;

#define MAXDEPTH 1000000

#define PUSH(XL, XR, Y, DY) \
    if( sp < stack+MAXDEPTH && Y+(DY) >= 0 && Y+(DY) <= img->height -1 ) \
    { sp->x1 = XL; sp->x2 = XR; sp->y = Y; sp->dy = DY; ++sp; }

#define POP(XL, XR, Y, DY) \
    { --sp; XL = sp->x1; XR = sp->x2; Y = sp->y+(DY = sp->dy); }

void imgalgo_flood_trace(image_t * img, //uint8_t *pixels, unsigned int img_width, unsigned int img_height,
			 unsigned int x, unsigned int y, 
			 uint8_t old_col, uint8_t new_col,
			 unsigned int *lowest_x, unsigned int * lowest_y, 
			 unsigned int * highest_x, unsigned int * highest_y) {
    
  unsigned int left, x1, x2, dy;

  LINESEGMENT stack[MAXDEPTH], *sp = stack;

  PUSH(x, x, y, 1);        // needed in some cases 
  PUSH(x, x, y+1, -1);    // seed segment (popped 1st)

  while( sp > stack ) {
    POP(x1, x2, y, dy);

    if(highest_y != NULL && y > *highest_y) *highest_y = y;
    if(lowest_y != NULL && y < *lowest_y) *lowest_y = y;

    // goto left
    for(x = x1; x > 0 && GetPixelFF(x, y) == old_col; --x) {
      SetPixelFF(x, y, new_col);
      //if(add_feature_func) (*add_feature_func)(lmodel, layer, x, y, obj);
      if(lowest_x != NULL && x < *lowest_x) *lowest_x = x;
      if(highest_x != NULL && x > *highest_x) *highest_x = x;
    }

    if(x >= x1 ) goto SKIP;

    left = x+1;
    if(left < x1 ) PUSH(left, x1-1, y, -dy);    // leak on left? 

    x = x1+1;

    do {
      if(lowest_y != NULL && y < *lowest_y) *lowest_y = y;
      if(highest_y != NULL && y > *highest_y) *highest_y = y;

      for( ; x <= img->width - 1 && GetPixelFF(x, y) == old_col; ++x) {
	SetPixelFF(x, y, new_col);
	//if(add_feature_func) (*add_feature_func)(lmodel, layer, x, y, obj);
	if(lowest_x != NULL && x < *lowest_x) *lowest_x = x;
	if(highest_x != NULL && x > *highest_x) *highest_x = x;
      }

      PUSH(left, x-1, y, dy);
      if( x > x2+1 )  PUSH(x2+1, x-1, y, -dy);    // leak on right?

    SKIP: 
      for( ++x; x <= x2 && GetPixelFF(x, y) != old_col; ++x );

      left = x;
    } while( x<=x2 );
  }

}
/*
ret_t imgalgo_trace_area(image_t * img,
			 unsigned int lowest_x, unsigned int lowest_y, 
			 unsigned int highest_x, unsigned int highest_y,
			 uint8_t col,
			 grid_t * grid,
			 logic_model_t * const lmodel, int layer, uint32_t obj, unsigned int object_id) {
  unsigned int _x, _y;

  if(!img || !lmodel) return RET_INV_PTR;

  for(_y = lowest_y; _y < highest_y; _y++)
    for(_x = lowest_x; _x < highest_x; _x++) {
      uint8_t pix_val = MASK_R(*(uint32_t *)mm_get_ptr(img->map, _x, _y));
      if(pix_val == col) {
	
	lmodel_add_feature(lmodel, layer, _x, _y, obj);
	lmodel_set_object_id(lmodel, layer, _x, _y, object_id);
	
      }
    }

  return RET_OK;
}

bool imgalgo_is_pin_at(image_t * img, unsigned int x, unsigned int y, 
		       unsigned int pin_diameter, uint8_t col,
		       unsigned int *lowest_x, unsigned int * lowest_y, 
		       unsigned int * highest_x, unsigned int * highest_y) {

  unsigned int _x = x, _y = y;

  uint8_t pix_val = MASK_R(*(uint32_t *)mm_get_ptr(img->map, x, y));

  if(pix_val == col) {
    *lowest_x = _x;
    *lowest_y = _y;
    *highest_x = _x;
    *highest_y = _y;
    
    imgalgo_flood_trace(img, _x, _y, COL_UNDEF, COL_TMP, lowest_x, lowest_y, highest_x, highest_y);


    if((*highest_x - *lowest_x <= pin_diameter) && (*highest_y - *lowest_y <= pin_diameter) &&
       !((*highest_x - *lowest_x == 1) && (*highest_y - *lowest_y == 1))) {
      
      if(*lowest_x > pin_diameter && *highest_x < img->width - pin_diameter - 1) {
	*lowest_x = _x - (pin_diameter >> 1);
	*highest_x = _x + (pin_diameter >> 1);
      }
      if(*lowest_y > pin_diameter && *highest_y < img->height - pin_diameter - 1) {
	*lowest_y = _y - (pin_diameter >> 1);
	*highest_y = _y + (pin_diameter >> 1);
      }
      imgalgo_flood_trace(img, _x, _y, COL_TMP, COL_DONE, NULL, NULL, NULL, NULL);
      return TRUE;
    }
    else {
      imgalgo_flood_trace(img, _x, _y, COL_TMP, COL_DONE, NULL, NULL, NULL, NULL);
      
    }
  }

  return FALSE;
}

#define POINT_IN_AREA(check_x, check_y, minx, miny, maxx, maxy)	\
  ((check_x > minx) && (check_x < maxx) && (check_y > miny) && (check_y < maxy))

ret_t imgalgo_match_wires(image_t * img, matching_params_t * m_params) {

  unsigned int dst_x, dst_y;
  
  if(!m_params || !img) return RET_INV_PTR;

  for(dst_y = m_params->min_y; 
      dst_y < MIN(img->height, m_params->max_y); 
      dst_y++) {

    for(dst_x = m_params->min_x; 
	dst_x < MIN(img->width, m_params->max_x);
	dst_x++) {

      uint32_t * pix = (uint32_t *)mm_get_ptr(img->map, dst_x, dst_y);
      uint8_t pix_val = MASK_R(*pix);

      unsigned int lowest_x = dst_x, lowest_y = dst_y, highest_x = dst_x, highest_y = dst_y;

      if(pix_val == COL_UNDEF) {

	imgalgo_flood_trace(img, dst_x, dst_y, COL_UNDEF, COL_TMP, &lowest_x, &lowest_y, &highest_x, &highest_y);

	// if it is to small (noise or pins), ignore it
	if((highest_x - lowest_x <= m_params->pin_diameter) && (highest_y - lowest_y <= m_params->pin_diameter) ) {
	  imgalgo_flood_trace(img, dst_x, dst_y, COL_TMP, 0, NULL, NULL, NULL, NULL);
	}
	
	// horizontal or "quadratic" object
	if(highest_x - lowest_x >= highest_y - lowest_y) { 
	  if(m_params->match_horizontal_objects == FALSE) {
	    imgalgo_flood_trace(img, dst_x, dst_y, COL_TMP, 0, NULL, NULL, NULL, NULL);
	  }
	  else {

	    if(m_params->lmodel)
	      imgalgo_trace_area(img, lowest_x, lowest_y, highest_x, highest_y,
				 COL_TMP, m_params->grid,
				 m_params->lmodel, m_params->layer, LM_WIRE_HORIZONTAL | LM_WIRE_VERTICAL, 
				 m_params->lmodel->object_id_counter++);
	    imgalgo_flood_trace(img, dst_x, dst_y, COL_TMP, COL_DONE, NULL, NULL, NULL, NULL);
	  }
	}	
	// vertical object
	if(highest_x - lowest_x < highest_y - lowest_y) { 
	  if(m_params->match_vertical_objects == FALSE) {
	    imgalgo_flood_trace(img, dst_x, dst_y, COL_TMP, 0, NULL, NULL, NULL, NULL);
	  }
	  else {
	    
	    if(m_params->lmodel)
	      imgalgo_trace_area(img, lowest_x, lowest_y, highest_x, highest_y,
				 COL_TMP, m_params->grid,
				 m_params->lmodel, m_params->layer, 
				 LM_WIRE_HORIZONTAL | LM_WIRE_VERTICAL, 
				 m_params->lmodel->object_id_counter++);
	    
	    imgalgo_flood_trace(img, dst_x, dst_y, COL_TMP, COL_DONE, NULL, NULL, NULL, NULL);
	  }
                        
	}
      } 
    }
  }
  return RET_OK;
}

ret_t imgalgo_match_pins(image_t * img, matching_params_t * m_params) {

  unsigned int dst_x, dst_y;
  
  if(!m_params || !img) return RET_INV_PTR;

  for(dst_y = m_params->min_y; 
      dst_y < MIN(img->height, m_params->max_y); 
      dst_y++) {

    for(dst_x = m_params->min_x; 
	dst_x < MIN(img->width, m_params->max_x);
	dst_x++) {

      //uint32_t * pix = (uint32_t *)mm_get_ptr(img, dst_x, dst_y);
      //uint8_t pix_val = MASK_R(*pix);

      // check pin
      unsigned int lowest_x = dst_x, lowest_y = dst_y, highest_x = dst_x, highest_y = dst_y;

      if(imgalgo_is_pin_at(img, dst_x, dst_y, 
			   m_params->pin_diameter, COL_UNDEF,
			   &lowest_x, &lowest_y, &highest_x, &highest_y)) {
	
	if(m_params->lmodel && 
	   !LM_BIT_SET(lmodel_get_object(m_params->lmodel, m_params->layer, dst_x, dst_y), 
		       LM_IL_DOWN| LM_IL_UP)) {

	  unsigned int x,y;
	  lmodel_find_nearest_wire_endings(m_params->lmodel, m_params->layer, dst_x, dst_y, m_params->pin_diameter,
					   m_params->lmodel->object_id_counter);
	  
	  for(y = lowest_y; y <= highest_y; y++)
	    for(x = lowest_x; x <= highest_x; x++) {
	      lmodel_add_feature(m_params->lmodel, m_params->layer, x, y, LM_IL_UP |  LM_WIRE_HORIZONTAL | LM_WIRE_VERTICAL);
	      lmodel_set_object_id(m_params->lmodel, m_params->layer, x, y, m_params->lmodel->object_id_counter);
	    }

	  m_params->lmodel->object_id_counter++;
	}
      } 
    }
  }
  return RET_OK;
}
*/


ret_t imgalgo_run_object_matching(matching_params_t * m_params) {
  if(!m_params) return RET_INV_PTR;
  ret_t ret;

  image_t * temp = gr_create_image(m_params->img->width, m_params->img->height, m_params->img->image_type);
  if(!temp) return RET_ERR;
	
  if(!RET_IS_OK(ret = gr_map_temp_file(temp, m_params->project_dir))) {
    gr_image_destroy(temp);
    return ret;
  }

  if(!RET_IS_OK(ret = gr_copy_image(temp, m_params->img,
			      0, 0, m_params->img->width, m_params->img->height))) {
    puts("Can't copy image");
    gr_image_destroy(temp);
    return ret;
  }

  if(!RET_IS_OK(ret = imgalgo_to_grayscale(temp))) {
    puts("Can't convert image to grayscale");
    gr_image_destroy(temp);
    return ret;
  }

  if(!RET_IS_OK(ret = imgalgo_separate_by_threshold(temp, m_params->threshold_col_separation))) {
    gr_image_destroy(temp);
    return ret;
  }

  /*
  if(m_params->match_object_type == LM_OBJECT_TYPE_METAL)
    if(!RET_IS_OK(ret = imgalgo_match_wires(temp, m_params))) {
      gr_image_destroy(temp);
      return ret;
    }

  if(m_params->match_object_type == LM_OBJECT_TYPE_PIN)
    if(!RET_IS_OK(ret = imgalgo_match_pins(temp, m_params))) {
      gr_image_destroy(temp);
      return ret;
    }
  */
  if(!RET_IS_OK(ret = gr_image_destroy(temp))) {
    return ret;
  }
  return RET_OK;
}

