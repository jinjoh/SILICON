/**
   This piece of code serves as an example on how to write a
   plugin for degate.

   The code performs template matching by shifting a template
   over a background image. For each step the normalized
   cross correlation is calculated.

   http://en.wikipedia.org/wiki/Cross-correlation#Normalized_cross-correlation

   If you want to write a plugin, you should read:
   - logic_model.h
   - plugins.h
   - graphics.h and memory_map.h

   Please do not interact with the quadtree directly. There will be changes.
   If you miss features, please contact me.

   Martin Schobert <martin@mailbeschleuniger.de>
 */
#include <iostream>
#include <gtkmm.h>
#include <assert.h>
#include <fcntl.h>
#include <time.h>
#include "globals.h"
#include "plugins.h"
#include "gui/GateSelectWin.h"
#include "gui/TemplateMatchingParamsWin.h"

#define TM "template.cc"

Gtk::Dialog* pDialog = 0;

int stats_real_gamma_calcs = 0;

typedef struct {
  double threshold;
  unsigned int max_step_size_search;
  unsigned int max_step_size_xcorr;
  lmodel_gate_template_t * tmpl_ptr;
} template_matching_params_t;

ret_t init_template(plugin_params_t * pparams) {
  debug(TM, "init(%p) called", pparams);
  if(!pparams) return RET_INV_PTR;

  pparams->data_ptr = malloc(sizeof(template_matching_params_t));
  if(!pparams->data_ptr) return RET_MALLOC_FAILED;

  memset(pparams->data_ptr, 0, sizeof(template_matching_params_t));
  return RET_OK;
}

ret_t shutdown_template(plugin_params_t * pparams) {
  debug(TM, "shutdown(%p) called", pparams);
  if(!pparams) return RET_INV_PTR;

  if(pparams->data_ptr) free(pparams->data_ptr);
  pparams->data_ptr = NULL;
  return RET_OK;
}

/* some function prototypes */
ret_t template_matching(plugin_params_t * foo);
ret_t raise_dialog(Gtk::Window *parent, plugin_params_t * foo);
ret_t imgalgo_run_template_matching(image_t * master,
				    image_t * _template,
				    memory_map_t * summation_table_single,
				    memory_map_t * summation_table_squared,
				    
				    unsigned int min_x, unsigned int min_y,
				    unsigned int max_x, unsigned int max_y,
				    const char * const project_dir,
				    logic_model_t * const lmodel, int layer, 
				    lmodel_gate_template_t * tmpl_ptr,
				    LM_TEMPLATE_ORIENTATION orientation,
				    template_matching_params_t * matching_params);

ret_t imgalgo_calc_xcorr(image_t * master, 
			 memory_map_t * zero_mean_template, 
			 memory_map_t * summation_table_single,
			 memory_map_t * summation_table_squared,
			 double sum_over_zero_mean_template,
			 memory_map_t * result_map);

double imgalgo_calc_single_xcorr(image_t * master, 
				 memory_map_t * zero_mean_template, 
				 memory_map_t * summation_table_single,
				 memory_map_t * summation_table_squared,
				 double sum_over_zero_mean_template,
				 unsigned int x, unsigned int y, unsigned int step_size);

double calc_mean_for_img_area(image_t * img, unsigned int min_x, unsigned int min_y, 
			      unsigned int width, unsigned int height);



/** 
    This structure defines, which functions this plugin provides. 
*/
plugin_func_descr_t plugin_func_descriptions[] = {
  { "Template matching",     // name will be displayed in menu
    &template_matching, // a function that perfoms the calculation
    (plugin_raise_dialog_func_t) &raise_dialog, // a gui dialog to call before
    NULL,         // a gui dialog to call after calculation
    &init_template,
    &shutdown_template},
  { NULL, NULL, NULL, NULL}
};


inline double mm_get_double(memory_map_t * table, unsigned int x, unsigned int y) {
  return *(double *)mm_get_ptr(table, x, y);
}

inline void mm_set_double(memory_map_t * table, unsigned int x, unsigned int y, double v) {
  *(double *)mm_get_ptr(table, x, y) = v;
}


ret_t precalc_summation_tables(image_t * master_img, 
			       memory_map_t * summation_table_single, memory_map_t * summation_table_squared) {


  unsigned int x, y;

  for(y = 0; y < master_img->height; y++)
    for(x = 0; x < master_img->width; x++) {
      mm_set_double(summation_table_single, x, y, 0);
      mm_set_double(summation_table_squared, x, y, 0);
    }

  for(y = 0; y < master_img->height; y++)
    for(x = 0; x < master_img->width; x++) {

      double f = gr_get_greyscale_pixval(master_img, x, y);

      double s_l = x > 0 ? mm_get_double(summation_table_single, x - 1, y) : 0;
      double s_o = y > 0 ? mm_get_double(summation_table_single, x, y - 1) : 0;
      double s_lo = x > 0 && y > 0 ? mm_get_double(summation_table_single, x - 1, y - 1) : 0;


      double s2_l = x > 0 ? mm_get_double(summation_table_squared, x - 1, y) : 0;
      double s2_o = y > 0 ? mm_get_double(summation_table_squared, x, y - 1) : 0;
      double s2_lo = x > 0 && y > 0 ? mm_get_double(summation_table_squared, x - 1, y - 1) : 0;

      double f1 = f + s_l + s_o - s_lo;
      double f2 = f*f + s2_l + s2_o - s2_lo;

      mm_set_double(summation_table_single, x, y, f1);
      mm_set_double(summation_table_squared, x, y, f2);
    }
  return RET_OK;
}

/* This function is called back from the main application within
   a thread. 
*/
ret_t template_matching(plugin_params_t * pparams) {
  ret_t ret;
  image_t * master_img_gs;

  LM_TEMPLATE_ORIENTATION orientation;
  template_matching_params_t * match_params = (template_matching_params_t *) pparams->data_ptr;

  assert(pparams);
  if(!pparams) return RET_INV_PTR;

  unsigned int layer = pparams->project->current_layer;
  /* this is a pointer to the background image */
  image_t * master_img = pparams->project->bg_images[layer];

  debug(TM, "matching on layer = %d", layer);

  /************************************************************************************
   *
   * Prepare the master image. This is the backgound image.
   *
   ************************************************************************************/
  // we get get a lot of performance gain, if we use a grayscaled image
  master_img_gs = gr_create_image( pparams->max_x - pparams->min_x, 
				   pparams->max_y - pparams->min_y, 
				   IMAGE_TYPE_GS);
     
  if(RET_IS_NOT_OK(ret = gr_map_temp_file(master_img_gs, 
					  pparams->project->project_dir))) return ret;
  
  // implicit conversion to gs
  if(RET_IS_NOT_OK(ret = gr_copy_image(master_img_gs, master_img, 
				       pparams->min_x, pparams->min_y,
				       pparams->max_x, pparams->max_y))) return ret;

  /************************************************************************************
   *
   * Summation tables
   *
   ************************************************************************************/

  memory_map_t * summation_table_single = mm_create(pparams->max_x - pparams->min_x,
						    pparams->max_y - pparams->min_y, 
						    sizeof(double));
  if(!summation_table_single) return RET_ERR;
  
  if(RET_IS_NOT_OK(ret = mm_map_temp_file(summation_table_single, pparams->project->project_dir))) {
    mm_destroy(summation_table_single);
    return ret;
  }

  memory_map_t * summation_table_squared = mm_create(pparams->max_x - pparams->min_x,
						     pparams->max_y - pparams->min_y, 
						     sizeof(double));
  if(!summation_table_squared) return RET_ERR;
  
  if(RET_IS_NOT_OK(ret = mm_map_temp_file(summation_table_squared, pparams->project->project_dir))) {
    mm_destroy(summation_table_squared);
    return ret;
  }

  precalc_summation_tables(master_img_gs, summation_table_single, summation_table_squared);


  /************************************************************************************
   *
   * create a temp image from the template 
   *
   ************************************************************************************/
  lmodel_gate_template_t * gate_template = match_params->tmpl_ptr;
  debug(TM, "selected template = %s", gate_template->short_name);

  image_t * _template = 
    gr_extract_image_as_gs(master_img,
			   gate_template->master_image_min_x,
			   gate_template->master_image_min_y,
			   gate_template->master_image_max_x - gate_template->master_image_min_x,
			   gate_template->master_image_max_y - gate_template->master_image_min_y);
					 
  if(!_template) return RET_ERR;


  clock_t start, finish;
  start = clock();

  debug(TM, "Template matching: normal");
  orientation = LM_TEMPLATE_ORIENTATION_NORMAL;
  ret = imgalgo_run_template_matching(master_img_gs, 
				      _template,
				      summation_table_single,
				      summation_table_squared,
				      
				      pparams->min_x, pparams->min_y,
				      pparams->max_x - _template->width,
				      pparams->max_y - _template->height,
				      pparams->project->project_dir,
				      pparams->project->lmodel, 
				      pparams->project->current_layer,
				      gate_template, orientation, 
				      match_params);
  
  debug(TM, "Template matching: flipped up down");
  orientation = LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN;
  gr_flip_up_down(_template);
  ret = imgalgo_run_template_matching(master_img_gs,
				      _template,
				      summation_table_single,
				      summation_table_squared,
				      
				      pparams->min_x, pparams->min_y,
				      pparams->max_x - _template->width,
				      pparams->max_y - _template->height,
				      pparams->project->project_dir,
				      pparams->project->lmodel, 
				      pparams->project->current_layer,
				      gate_template, orientation, 
				      match_params);

  debug(TM, "Template matching: flipped both");
  orientation = LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH;
  gr_flip_left_right(_template);
  ret = imgalgo_run_template_matching(master_img_gs, 			
				      _template,
				      summation_table_single,
				      summation_table_squared,
				      
				      pparams->min_x, pparams->min_y,
				      pparams->max_x - _template->width,
				      pparams->max_y - _template->height,
				      pparams->project->project_dir,
				      pparams->project->lmodel, 
				      pparams->project->current_layer,
				      gate_template, orientation, 
				      match_params);

  debug(TM, "Template matching: flipped left right");
  orientation = LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT;
  gr_flip_up_down(_template);
  ret = imgalgo_run_template_matching(master_img_gs, 			
				      _template,
				      summation_table_single,
				      summation_table_squared,
				      
				      pparams->min_x, pparams->min_y,
				      pparams->max_x - _template->width,
				      pparams->max_y - _template->height,
				      pparams->project->project_dir,
				      pparams->project->lmodel, 
				      pparams->project->current_layer,
				      gate_template, orientation, 
				      match_params);


  // stats
  finish = clock();
  double total_time_ms = 1000*(double(finish - start)/CLOCKS_PER_SEC);
  double number_of_gamma_calcs = 4 * 
    (pparams->max_y - pparams->min_y - _template->height) *
    (pparams->max_x - pparams->min_x - _template->width);

  debug(TM, "-------------------- [ matching stats ] --------------------");
  debug(TM, "region x: %d .. %d  region y %d .. %d -> %d x %d px", pparams->min_x , pparams->max_x, pparams->min_y, pparams->max_y,
	pparams->max_x - pparams->min_x, pparams->max_y - pparams->min_y);
  debug(TM, "xcorr time total: %f ms", total_time_ms);
  debug(TM, "xcorr nummer of (theoretical) gamma calculations: %.0f", number_of_gamma_calcs);
  debug(TM, "xcorr nummer of real gamma calculations: %d", stats_real_gamma_calcs);
  debug(TM, "xcorr time per (theoretical) gamma: %f ms", total_time_ms / number_of_gamma_calcs);
  debug(TM, "xcorr time per real gamma: %f ms", total_time_ms / stats_real_gamma_calcs);
  debug(TM, "------------------------------------------------------------");

  // free temp image
  if(RET_IS_NOT_OK(ret = gr_image_destroy(_template)))
    debug(TM, "gr_image_destroy() failed");
  if(RET_IS_NOT_OK(ret = gr_image_destroy(master_img_gs)))
    debug(TM, "gr_image_destroy() failed");

  mm_destroy(summation_table_single);
  mm_destroy(summation_table_squared);

  return RET_OK;
}


ret_t raise_dialog(Gtk::Window * parent, plugin_params_t * pparams) {
  lmodel_gate_template_t * tmpl = NULL;
  ret_t ret;
  assert(pparams);
  if(!pparams) return RET_INV_PTR;
  unsigned int layer = pparams->project->current_layer;

  /* pparams->(min|max)_(x|y) is the region, that is selected. If nothing
     is selected, these variables are zero.
     If nothing is selected, we set it to the complete background image.
   */

  if(pparams->max_x == 0 && pparams->max_y == 0) {
    Gtk::MessageDialog dialog(*parent, 
			      "You did not select an area for the template matching. "
			      "Do you really wan't to run template matching on the whole "
			      "image? It will take a lot of time.",
			      true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
    dialog.set_title("Warning");      
    if(dialog.run() == Gtk::RESPONSE_NO) {
      return RET_CANCEL;
    }
  }

  if(pparams->max_x == 0) {
    pparams->min_x = 0;
    pparams->max_x = pparams->project->bg_images[layer]->width -1;
  }
  if(pparams->max_y == 0) {
    pparams->min_y = 0;
    pparams->max_y = pparams->project->bg_images[layer]->height -1;
  }

  /* This will show a dialog window wit a list of available gate types. */
  GateSelectWin gsWin(parent, pparams->project->lmodel);
  tmpl = gsWin.run();
  if(tmpl) {
    /* Check, if there is a graphical representation for the gate template. */
    if(tmpl->master_image_min_x < tmpl->master_image_max_x &&
       tmpl->master_image_min_y < tmpl->master_image_max_y) {

      if(pparams->max_x - pparams->min_x < 
	 tmpl->master_image_max_x - tmpl->master_image_min_x ||
	 pparams->max_y - pparams->min_y < 
	 tmpl->master_image_max_y - tmpl->master_image_min_y) {

	Gtk::MessageDialog dialog(*parent, 
				  "The template to match is larger than the "
				  "aera, where the template should be searched.", 
				  true, Gtk::MESSAGE_ERROR);
	dialog.set_title("Error");
	dialog.run();
	return RET_ERR;
      }

      /* The user selected a type of gate. There is a data pointer field
	 in structure plugin_params_t, that the plugin can use to store
	 data. We use it to store the pointer to the template.

	 If you allocate data, you have to free it by yourself.
      */
      template_matching_params_t * matching_params = 
	(template_matching_params_t *) pparams->data_ptr;

      matching_params->tmpl_ptr = tmpl;

      TemplateMatchingParamsWin paramsWin(parent, 0.6, 
					  MAX(1, pparams->project->lambda), 
					  MAX(1, (pparams->project->lambda >> 1)));
      
      if(RET_IS_NOT_OK(ret = paramsWin.run(&(matching_params->threshold),
					   &(matching_params->max_step_size_search),
					   &(matching_params->max_step_size_xcorr) ))) return ret;

      /* If the user entered invalid step size, nothing will happen. Because of unsigned int
	 it is not possible to supply negative values. */

      return RET_OK;
    }
    else {
      Gtk::MessageDialog dialog(*parent, 
				"There is no master image for the gate type you selected.", 
				true, Gtk::MESSAGE_ERROR);
      dialog.set_title("Error");
      dialog.run();

      return RET_ERR;
    }

  }

  return RET_CANCEL;
}


ret_t clear_area_in_map(memory_map_t * temp, 
			unsigned int start_x, unsigned int start_y, 
			unsigned int radius) {

  unsigned int x, y;

  for(y = start_y > radius ? start_y - radius : 0; 
      y < MIN(start_y + radius, temp->height); y++) {
    for(x = start_x > radius ? start_x - radius : 0; 
	x < MIN(start_x + radius, temp->width); x++) {
      mm_set_double(temp, x, y, -1);
    }
  }
  return RET_OK;
}

#define CALC_AND_CHECK_DIRECTION(_x, _y, v) { \
    double curr_val =  mm_get_double(temp, _x, _y); \
    if(curr_val <= 0) { \
      curr_val = imgalgo_calc_single_xcorr(master, zero_mean_template, \
					     summation_table_single, summation_table_squared, \
					     sum_over_zero_mean_template, _x, _y, 1); \
      mm_set_double(temp, _x, _y, curr_val); \
    } \
    if(curr_max_val < curr_val) { \
      max_corr_x2 = _x; \
      max_corr_y2 = _y; \
      curr_max_val = curr_val; \
    }}


ret_t imgalgo_run_template_matching(image_t * master, 
				    image_t * _template,
				    memory_map_t * summation_table_single,
				    memory_map_t * summation_table_squared,

				    unsigned int min_x, unsigned int min_y,
				    unsigned int max_x, unsigned int max_y,
				    const char * const project_dir,
				    logic_model_t * const lmodel, 
				    int layer, lmodel_gate_template_t * tmpl_ptr,
				    LM_TEMPLATE_ORIENTATION orientation,
				    template_matching_params_t * matching_params) {

  unsigned int x, y;
  ret_t ret;
  double sum_over_zero_mean_template = 0;
  int placement_layer = lmodel_get_layer_num_by_type(lmodel, LM_LAYER_TYPE_LOGIC);
  debug(TM, "placement layer = %d", placement_layer);
  if(min_x >= max_x || min_y >= max_y) return RET_ERR;


  // prepare template
  memory_map_t * zero_mean_template = mm_create(_template->width, _template->height, sizeof(double));
  if(!zero_mean_template) return RET_ERR;
  if(RET_IS_NOT_OK(ret = mm_alloc_memory(zero_mean_template))) return ret;;

  double template_mean = calc_mean_for_img_area(_template, 0, 0, _template->width, _template->height);

  for(y = 0; y < _template->height; y++) 
    for(x = 0; x < _template->width; x++) {
      double tmp = (double)gr_get_greyscale_pixval(_template, x, y) - template_mean;
      mm_set_double( zero_mean_template, x, y, tmp);
      sum_over_zero_mean_template += tmp * tmp;
    }

  /* Create a data structure for the temp space, where we can store doubles. */
  memory_map_t * temp = mm_create(max_x - min_x, max_y - min_y, sizeof(double));
  if(!temp) return RET_ERR;
  
  /* Create a temp file in the project directory. The temp file is mapped into memory. */
  if(RET_IS_NOT_OK(ret = mm_map_temp_file(temp, project_dir))) {
    mm_destroy(temp);
    return ret;
  }
  for(y = 0; y < temp->height; y ++) for(x = 0; x < temp->width; x ++) mm_set_double(temp, x, y, 0);
    

  unsigned int step_size_search = matching_params->max_step_size_search;
  unsigned int step_size_xcorr  = matching_params->max_step_size_xcorr;
  double last_val = 0;

  unsigned int start = 0;

  for(y = 0; y < max_y - min_y; y += step_size_search) {
    for(x = start; x < max_x - min_x; x += step_size_search) {

      if(mm_get_double(temp, x, y) == -1) continue;
      
      double val = imgalgo_calc_single_xcorr(master, zero_mean_template, 
					     summation_table_single, summation_table_squared, 
					     sum_over_zero_mean_template, x, y, step_size_xcorr);

      mm_set_double(temp, x, y, val);

      // calculate step size
      if(val > 0) {
	if(fabs(last_val - val) > 0.01) {
	  step_size_search = MAX(1, rint((1.0 - (double)matching_params->max_step_size_search) * val + matching_params->max_step_size_search));
	  step_size_xcorr = MIN(MAX(1, (step_size_search>>1)), matching_params->max_step_size_xcorr);
	}
      }
      else {
	if(fabs(last_val - val) > 0.01) {
	  step_size_search = matching_params->max_step_size_search;
	  step_size_xcorr  = matching_params->max_step_size_xcorr;
	}
      }
      last_val = val;

      if(val >= matching_params->threshold) {
	
	unsigned int max_corr_x = x, max_corr_y = y;
	unsigned int max_corr_x2 = x, max_corr_y2 = y;
	double curr_max_val = val;

	debug(TM, "\t+++ area of higher correlation found at %d,%d -> corr = %f", max_corr_x, max_corr_y, val);

	do {
	  val = curr_max_val;

	  if(max_corr_x > 1 && max_corr_y > 1) CALC_AND_CHECK_DIRECTION(max_corr_x-1, max_corr_y-1, val);
	  if(max_corr_y > 1) CALC_AND_CHECK_DIRECTION(max_corr_x, max_corr_y-1, val);
	  if(max_corr_y > 1 && max_corr_x < temp->width) 
	    CALC_AND_CHECK_DIRECTION(max_corr_x+1, max_corr_y-1, val);
    
	  if(max_corr_x > 1) CALC_AND_CHECK_DIRECTION(max_corr_x - 1, max_corr_y, val);
	  if(max_corr_x < temp->width) CALC_AND_CHECK_DIRECTION(max_corr_x+1, max_corr_y, val);
    
	  if(max_corr_x > 1 && max_corr_y < temp->height) CALC_AND_CHECK_DIRECTION(max_corr_x - 1, max_corr_y + 1, val);
	  if(max_corr_y < temp->height) CALC_AND_CHECK_DIRECTION(max_corr_x, max_corr_y + 1, val);
	  if(max_corr_x < temp->width && max_corr_y < temp->height) 
	    CALC_AND_CHECK_DIRECTION(max_corr_x + 1, max_corr_y + 1, val);

	  max_corr_x = max_corr_x2;
	  max_corr_y = max_corr_y2;
	  debug(TM, "\tclimbed up to %d,%d -> corr = %f", max_corr_x, max_corr_y, curr_max_val);

	} while(curr_max_val > val);

	debug(TM, "\tfound a correlation hotspot at %d,%d with v = %f", max_corr_x, max_corr_y, curr_max_val);

	double dx = (double)max_corr_x - (double)x;
	double dy = (double)max_corr_y - (double)y;
	
	// clear areas of higher correlation to prevent picking it up next time
	unsigned int radius = 2 * sqrt(dx*dx + dy*dy) + matching_params->max_step_size_search;
	debug(TM, "\tclear area radius = %d", radius);
	if(RET_IS_NOT_OK(ret = clear_area_in_map(temp, max_corr_x, max_corr_y, radius))) return ret;	
	
	unsigned int w = (tmpl_ptr->master_image_max_x - tmpl_ptr->master_image_min_x);
	unsigned int h = (tmpl_ptr->master_image_max_y - tmpl_ptr->master_image_min_y);

	x += w;

	lmodel_gate_t * gate;
	if(RET_IS_NOT_OK(lmodel_get_gate_in_region(lmodel, placement_layer, 
						   min_x + max_corr_x, 
						   min_y + max_corr_y,
						   min_x + max_corr_x + w,
						   min_y + max_corr_y + h,
						   &gate))) return RET_ERR;

	if(!gate) {
	  // create gate
	  lmodel_gate_t * new_gate = lmodel_create_gate(lmodel,
							min_x + max_corr_x, 
							min_y + max_corr_y,
							min_x + max_corr_x + w,
							min_y + max_corr_y + h,
							tmpl_ptr,
							NULL, 0);
	  assert(new_gate);
	  if(!new_gate) return RET_ERR;
	  if(RET_IS_NOT_OK(ret = lmodel_set_gate_orientation(new_gate, orientation))) return ret;
	  if(RET_IS_NOT_OK(ret = lmodel_add_gate(lmodel, placement_layer, new_gate))) return ret;
	}
      }

    }
    start = start++ % matching_params->max_step_size_search;
  }
  /* unmap and remove temp image */
  if(RET_IS_NOT_OK(ret = mm_destroy(temp))) return ret;
  if(RET_IS_NOT_OK(ret = mm_destroy(zero_mean_template))) return ret;
  return RET_OK;
}

double calc_mean_for_img_area(image_t * img, unsigned int min_x, unsigned int min_y, 
			      unsigned int width, unsigned int height) {
  double mean = 0;
  unsigned int x,y;
  assert(width > 0 && height > 0);
  assert(min_x + width <= img->width);
  assert(min_y + height <= img->height);

  for(y = min_y; y < min_y + height; y++)
    for(x = min_x; x < min_x + width; x++)
      mean += gr_get_greyscale_pixval(img, x, y);

  return mean / (width * height);
}


double imgalgo_calc_single_xcorr(image_t * master, 
				 memory_map_t * zero_mean_template, 
				 memory_map_t * summation_table_single,
				 memory_map_t * summation_table_squared,
				 double sum_over_zero_mean_template,
				 unsigned int local_x, unsigned int local_y, unsigned int step_size) {

  double template_size = zero_mean_template->width * zero_mean_template->height;

  unsigned int 
    x_plus_w = local_x + zero_mean_template->width -1,
    y_plus_h = local_y + zero_mean_template->height -1,
    lxm1 = local_x - 1,
    lym1 = local_y - 1;
  
  // calulate denominator
  double 
    f1 = mm_get_double(summation_table_single, x_plus_w, y_plus_h),
    f2 = mm_get_double(summation_table_squared, x_plus_w, y_plus_h);
  
  if(local_x > 0) {
    f1 -= mm_get_double(summation_table_single, lxm1, y_plus_h);
    f2 -= mm_get_double(summation_table_squared, lxm1, y_plus_h);
  }
  if(local_y > 0) {
    f1 -= mm_get_double(summation_table_single, x_plus_w, lym1);
    f2 -= mm_get_double(summation_table_squared, x_plus_w, lym1);
  }
  if(local_x > 0 && local_y > 0) {
    f1 += mm_get_double(summation_table_single, lxm1, lym1);
    f2 += mm_get_double(summation_table_squared, lxm1, lym1);
  }
  
  double denominator = sqrt((f2 - f1*f1/template_size) * sum_over_zero_mean_template);
  
  // calculate nummerator
  
  unsigned int _x, _y;
  unsigned int factor =  step_size * step_size;
  double nummerator1 = 0, nummerator2 = 0;

#define METHOD1

#ifdef METHOD1
  for(_y = 0; _y < zero_mean_template->height; _y += step_size) {
    for(_x = 0; _x < zero_mean_template->width; _x += step_size) {
      double f_xy = gr_get_greyscale_pixval(master, _x + local_x, _y + local_y);
      double t_xy = mm_get_double(zero_mean_template, _x, _y);
      nummerator1 += (f_xy * t_xy) * factor;
      
    }
  }
#endif

#ifdef METHOD2
  double * map = (double *)alloca(zero_mean_template->height * zero_mean_template->width* sizeof(double));

  for(_y = 0; _y < zero_mean_template->height; _y += step_size)
    for(_x = 0; _x < zero_mean_template->width; _x += step_size) {
      double f_xy = gr_get_greyscale_pixval(master, _x + local_x, _y + local_y);
      double t_xy = mm_get_double(zero_mean_template, _x, _y);
      map[_y * zero_mean_template->width + _x] = (f_xy * t_xy) * factor;
    }

  for(_y = 0; _y < zero_mean_template->height; _y += step_size)
    for(_x = 0; _x < zero_mean_template->width; _x += step_size)
      nummerator1 += map[_y * zero_mean_template->width + _x];
#endif

#ifdef METHOD3
  double * map = (double *)alloca(zero_mean_template->height * zero_mean_template->width* sizeof(double));
  double * i = map;

  v4df a,b,c;
  for(_y = 0; _y < zero_mean_template->height; _y += 2)
    for(_x = 0; _x < zero_mean_template->width; _x += 2) {
      
      a.f[0] = gr_get_greyscale_pixval(master, _x + local_x, _y + local_y);

      double f_xy = gr_get_greyscale_pixval(master, _x + local_x, _y + local_y);
      double t_xy = mm_get_double(zero_mean_template, _x, _y);
      (*i++) = (f_xy * t_xy) * factor;
    }

  for(i = map; i < map + zero_mean_template->height * zero_mean_template->width;)
    nummerator2 += (*i++);
#endif
  
  //  assert(fabs( nummerator1 - nummerator2) < 0.0001);

  stats_real_gamma_calcs++;
  return nummerator1/denominator;
}
