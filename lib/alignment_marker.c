#include "alignment_marker.h"
#include "globals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

alignment_marker_set_t * amset_create(unsigned int num_layers) {
  alignment_marker_set_t * set = NULL;

  if((set = (alignment_marker_set_t *)malloc(sizeof(alignment_marker_set_t))) == NULL) {
    return NULL;
  }

  memset(set, 0, sizeof(alignment_marker_set_t));

  set->num_layers = num_layers;
  set->max_markers = num_layers * MARKERS_PER_LAYER;

  // we alloc mem for four more markers (2*down on bottom layer and 2*up on top layer)
  if((set->markers = (alignment_marker_t **)malloc(set->max_markers * 
						   sizeof(alignment_marker_t))) == NULL) {
    free(set);
    return NULL;
  }
  
  memset(set->markers, 0, num_layers * MARKERS_PER_LAYER * sizeof(alignment_marker_t));

  return set;
}

ret_t amset_destroy(alignment_marker_set_t * set) {
  assert(set != NULL);
  assert(set->markers != NULL);
  if(set == NULL || set->markers == NULL) return RET_INV_PTR;

  int i;
  for(i = 0; i < set->max_markers; i++)
    if(set->markers[i] != NULL) {
      free(set->markers[i]);
      set->markers[i] = NULL;
    }

  free(set->markers);
  set->markers = NULL;
  memset(set, 0, sizeof(alignment_marker_set_t));
  free(set);
  
  return RET_OK;
}


/**
   Checks, if a marker is already stored in the set. 
   @returns pointer to marker, else NULL
 */   
alignment_marker_t * amset_get_marker(const alignment_marker_set_t * const set, unsigned int layer, 
				       MARKER_TYPE marker_type) {

  int i;
  assert(set != NULL);
  assert(set->markers != NULL);

  if(set == NULL || set->markers == NULL) return NULL;
  
  for(i = 0; i < set->max_markers; i++) {
    alignment_marker_t * marker = set->markers[i];
    if(marker != NULL && 
       marker->marker_type == marker_type && 
       marker->layer == layer) return set->markers[i];
  }

  return NULL;
}


ret_t amset_add_marker(alignment_marker_set_t * set, unsigned int layer, 
		     MARKER_TYPE marker_type,
		     unsigned int x, unsigned int y) {
  int i;
  if(!set || !set->markers) return RET_INV_PTR;

  if(amset_get_marker(set, layer, marker_type) == NULL) {

    // find a free slot
    for(i = 0; i < set->max_markers; i++) {
      if(set->markers[i] == NULL) {
	if((set->markers[i] = (alignment_marker_t *)malloc(sizeof(alignment_marker_t))) == NULL)
	  return RET_ERR;
	memset(set->markers[i], 0, sizeof(alignment_marker_t));
	set->markers[i]->marker_type = marker_type;
	set->markers[i]->layer = layer;
	set->markers[i]->x = x;
	set->markers[i]->y = y;
	return RET_OK;
      }
    }
  }
  
  return RET_ERR;
}

ret_t amset_replace_marker(alignment_marker_set_t * set, unsigned int layer, 
			 MARKER_TYPE marker_type,
			 unsigned int x, unsigned int y) {
  
  if(!set || !set->markers) return RET_INV_PTR;
  alignment_marker_t * ptr_marker = amset_get_marker(set, layer, marker_type);

  if(ptr_marker != NULL) {
    ptr_marker->x = x;
    ptr_marker->y = y;
    return RET_OK;
  }
  else return RET_ERR;
}

/**
 * Check if all markers are specified to calculate the alignment.
 * @returns Returns 0 if we can't calculate the alignment, else 1. If there is only one layer, 0 is returned.
 */

int amset_complete(const alignment_marker_set_t * const set) {

  int i;
  if(!set || !set->markers) return 0;
  if(set->num_layers == 1) return 0;

  for(i = 1; i < set->num_layers - 1; i++)
    if( !amset_get_marker(set, i, MARKER_TYPE_M1_UP) ||
	!amset_get_marker(set, i, MARKER_TYPE_M2_UP) ||
	!amset_get_marker(set, i, MARKER_TYPE_M1_DOWN) ||
	!amset_get_marker(set, i, MARKER_TYPE_M1_DOWN)) return 0;
  
  if( !amset_get_marker(set, 0, MARKER_TYPE_M1_UP) ||
      !amset_get_marker(set, 0, MARKER_TYPE_M2_UP) ||
      !amset_get_marker(set, set->num_layers - 1, MARKER_TYPE_M1_DOWN) ||
      !amset_get_marker(set, set->num_layers - 1, MARKER_TYPE_M2_DOWN)) return 0;

  return 1;
}

void amset_print(const alignment_marker_set_t * const set) {

  int i;
  if(!set || !set->markers) return;

  for(i = 0; i < set->max_markers; i++) {
    alignment_marker_t * marker = set->markers[i];
    printf("%03d", i);
    if(marker == NULL) puts(" null");
    else {
      char *marker_type_str = amset_marker_type_to_str(marker->marker_type);
      printf(" marker-type=%22s layer=%02d x=%-6d y=%-6d\n", 
	     marker_type_str, marker->layer, marker->x, marker->y);
      free(marker_type_str);
    }
  }
  
}

/**
 * Get string representation of the marker type. Function allocates memory for the string, so you have to free it.
 * @returns Pointer to char array with the string representation of the marker type.
 */

char * amset_marker_type_to_str(MARKER_TYPE m_type) {
  switch(m_type) {
  case MARKER_TYPE_M1_UP: return strdup("MARKER_TYPE_M1_UP");
  case MARKER_TYPE_M2_UP: return strdup("MARKER_TYPE_M2_UP");
  case MARKER_TYPE_M1_DOWN: return strdup("MARKER_TYPE_M1_DOWN");
  case MARKER_TYPE_M2_DOWN: return strdup("MARKER_TYPE_M2_DOWN");
  default:
    return strdup("MARKER_TYPE_UNDEF");
  }
}

/**
 * Converts the string representation of a marker type to a marker type.
 * @returns marker type. If you pass a NULL pointer or we can't convert the string, MARKER_TYPE_UNDEF is returned.
 */
MARKER_TYPE amset_mtype_str_to_mtype(const char * const marker_type_str) {
  if(!marker_type_str) return MARKER_TYPE_UNDEF;
  if(!strcmp(marker_type_str, "MARKER_TYPE_M1_UP")) return MARKER_TYPE_M1_UP;
  else if(!strcmp(marker_type_str, "MARKER_TYPE_M2_UP")) return MARKER_TYPE_M2_UP;
  else if(!strcmp(marker_type_str, "MARKER_TYPE_M1_DOWN")) return MARKER_TYPE_M1_DOWN;
  else if(!strcmp(marker_type_str, "MARKER_TYPE_M2_DOWN")) return MARKER_TYPE_M2_DOWN;
  else return MARKER_TYPE_UNDEF;
}

ret_t amset_apply_transformation_to_markers(alignment_marker_set * const set,
					  double * const scaling_x, double * const scaling_y, 
					  int * const shift_x, int * const shift_y) {
  
  int i;

  if(set == NULL|| set->markers == NULL) return RET_INV_PTR;

  for(i = 0; i < set->max_markers; i++) {
    if(set->markers[i] != NULL) {
      int layer = set->markers[i]->layer;
      set->markers[i]->x = lrint(scaling_x[layer] * set->markers[i]->x) + shift_x[layer];
      set->markers[i]->y = lrint(scaling_y[layer] * set->markers[i]->y) + shift_y[layer];
    }
  }

  return RET_OK;
}

ret_t amset_calc_transformation(const alignment_marker_set_t * const set,
			      double * const scaling_x, double * const scaling_y, 
			      int * const shift_x, int * const shift_y) {

  int i,j;

  if(!set || !(set->markers)) {
    puts("amset_calc_transformation(): invalid params");
    return RET_ERR;
  }

  // preset scalings to 1 and shifts to 0
  for(i = 0; i < set->num_layers; i++) {
    scaling_x[i] = 1;
    scaling_y[i] = 1;
    shift_x[i] = 0;
    shift_y[i] = 0;
  }

  if(set->num_layers == 1) {
    puts("amset_calc_transformation(): only one layer");
    return RET_OK;
  }


  printf("calculating scalings\n");

  for(i = 0; i < set->num_layers - 1; i++) {

    // lower layer
    alignment_marker_t * m1_up = amset_get_marker(set, i, MARKER_TYPE_M1_UP);
    alignment_marker_t * m2_up = amset_get_marker(set, i, MARKER_TYPE_M2_UP);
    // upper layer
    alignment_marker_t * m1_down = amset_get_marker(set, i + 1, MARKER_TYPE_M1_DOWN);
    alignment_marker_t * m2_down = amset_get_marker(set, i + 1, MARKER_TYPE_M2_DOWN);

    if(!m1_up || !m2_up || !m1_down || !m2_down) {
      puts("amset_calc_transformation(): invalid marker pointer");
      return RET_ERR;
    }

    // lower layer
    double dist_x_lower = fabs(scaling_x[i] * ((double)(m1_up->x) - (double)(m2_up->x)) );
    double dist_y_lower = fabs(scaling_y[i] * ((double)(m1_up->y) - (double)(m2_up->y)) );

    // upper layer
    double dist_x_upper = fabs( scaling_x[i+1] * ((double)(m1_down->x) - (double)(m2_down->x)) );
    double dist_y_upper = fabs( scaling_y[i+1] * ((double)(m1_down->y) - (double)(m2_down->y)) );

    if(dist_x_lower == 0 || dist_x_upper == 0 ||
       dist_y_lower == 0 || dist_y_upper == 0) {
      puts("amset_calc_transformation(): ");
      return RET_ERR;
    }
       
    printf("\t lower layer %02d: distance between m1 and m2: x=%f  y=%f\n", i, dist_x_lower, dist_y_lower);
    printf("\t upper layer %02d: distance between m1 and m2: x=%f  y=%f\n", i, dist_x_upper, dist_y_upper);

    if(dist_x_lower > dist_x_upper) {
      /* lower layer is larger, this means, that any lower layer is larger (in respect to
	 calulated scaling values) */
      scaling_x[i+1] *= dist_x_lower / dist_x_upper;
      printf("\t x: lower layer is larger -> scaling of upper layer=%f\n", scaling_x[i+1]);
    }
    else {
      // upper layer is larger
      double scale = dist_x_upper / dist_x_lower;
      for(j = 0; j <= i; j++)
	scaling_x[j] *= scale;
      printf("\t x: upper layer is larger -> scaling of lower layer=%f\n", scale);
    }

    if(dist_y_lower > dist_y_upper) {
      /* lower layer is larger, this means, that any lower layer is larger (in respect to
	 calulated scaling values) */
      scaling_y[i+1] *= dist_y_lower / dist_y_upper;
      printf("\t y: lower layer is larger -> scaling of upper layer=%f\n", scaling_y[i+1]);
    }
    else {
      double scale = dist_y_upper / dist_y_lower;
      // upper layer is larger
      for(j = 0; j <= i; j++)
	scaling_y[j] *= scale;

      printf("\t y: upper layer is larger -> scaling of lower layer=%f\n", scaling_y[i]);
    }
    assert(scaling_x[i] >= 1);
    assert(scaling_y[i] >= 1);
    assert(scaling_x[i+1] >= 1);
    assert(scaling_y[i+1] >= 1);
  }

  puts("calculating shifts");
  for(i = 0; i < set->num_layers - 1; i++) {
    alignment_marker_t m_up;
    alignment_marker_t m_down;

    // lower layer
    alignment_marker_t * m1_up = amset_get_marker(set, i, MARKER_TYPE_M1_UP);
    alignment_marker_t * m2_up = amset_get_marker(set, i, MARKER_TYPE_M2_UP);
    // upper layer
    alignment_marker_t * m1_down = amset_get_marker(set, i + 1, MARKER_TYPE_M1_DOWN);
    alignment_marker_t * m2_down = amset_get_marker(set, i + 1, MARKER_TYPE_M2_DOWN);

    if(!m1_up || !m2_up || !m1_down || !m2_down) {
      puts("amset_calc_transformation(): invalid marker pointer");
      return RET_ERR;
    }


    // lower layer
    m_up.x = m1_up->x < m2_up->x ? m1_up->x : m2_up->x;
    m_up.y = m1_up->y < m2_up->y ? m1_up->y : m2_up->y;
    //m_up.x += shift_x[i];
    //m_up.y += shift_y[i];
    // upper layer
    m_down.x = m1_down->x < m2_down->x ? m1_down->x : m2_down->x;
    m_down.y = m1_down->y < m2_down->y ? m1_down->y : m2_down->y;
    //m_down.x += shift_x[i+1];
    //m_down.y += shift_y[i+1];

    double delta_x = 
      scaling_x[i] * ((double)(m_up.x)) + (double)shift_x[i] -
      (scaling_x[i+1] * ((double)m_down.x) + (double)shift_x[i+1]);

    double delta_y = 
      scaling_y[i] * ((double)(m_up.y)) + (double)shift_y[i] - 
      (scaling_y[i+1] * ((double)m_down.y) + (double)shift_y[i+1]);

    printf("\tlayer %02d -> %02d x=%f x=%f\n", i, i+1,
	   scaling_x[i] * ((double)(m_up.x)),
	   scaling_x[i+1] * ((double)m_down.x));
    printf("\tlayer %02d -> %02d y=%f y=%f\n", i, i+1,
	   scaling_y[i] * ((double)(m_up.y)),
	   scaling_y[i+1] * ((double)m_down.y));

    printf("\tlayer %02d to %02d: distance between m1/m1 delta_x = %f, delta_y = %f\n",
	   i, i+1, delta_x, delta_y);

    if(delta_x > 0) {
      // x-component: lower layer is more right than upper layer
      puts("\t x-component: lower layer is more right than upper layer");
      shift_x[i+1] += lrint(delta_x);
    }
    else {
      // x-component: upper layer is more right than lower layer
      puts("\t x-component: upper layer is more right than lower layer");
      for(j = 0; j <= i; j++)
	shift_x[j] += lrint(fabs(delta_x));
    }

    if(delta_y > 0) {
      // y-component: lower layer is more bottom than upper layer
      puts("\t y-component: lower layer is more bottom than upper layer");
      shift_y[i+1] += lrint(delta_y);
    }
    else {
      // y-component: upper layer is more bottom than lower layer
      puts("\t y-component: upper layer is more bottom than lower layer");
      for(j = 0; j <= i; j++)
	shift_y[j] += lrint(fabs(delta_y));
    }


  }

  // boundig boxes berechnen
  /*
    000 marker-type=1 layer=00 x=59     y=64    
    001 marker-type=2 layer=01 x=324    y=463   

    dx = 324 - 59
    dy = 463 - 64
    003 marker-type=4 layer=01 x=1309   y=4779  
    002 marker-type=3 layer=00 x=1069   y=4377  


    ueber alle layer von i = 0 bis i <  num_layer -1 iterieren:
      i: M1_UP
      i + 1: M1_DOWN

      dx, dy berechnen:
        dx = m1_up.x - m1_down.x
	
      if dx > 0:
        // auf dem untern layer ist weiter rechts
	dx[layer = 0] = 0 // den unteren layer nicht verschieben
	dx[layer = 1] = dx // den oberen layer nach rechts verschieben
      else:
        // der obere layer ist weiter rechts
	dx[layer = 0] = -dx
	dx[layer = 1] = 0
      end

      // exakt so fuer dy

      
      
  */

  return RET_OK;
}
