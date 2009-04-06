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

#ifndef __ALIGNMENT_MARKER_H__
#define __ALIGNMENT_MARKER_H__

#include "globals.h"

#define MARKERS_PER_LAYER 4

/* data types */

enum MARKER_TYPE {
  MARKER_TYPE_UNDEF = 0,
  MARKER_TYPE_M1_UP = 1,
  MARKER_TYPE_M1_DOWN = 2,
  MARKER_TYPE_M2_UP = 3,
  MARKER_TYPE_M2_DOWN = 4
};

struct alignment_marker {
  unsigned int x, y, layer;
  MARKER_TYPE marker_type;
};

typedef struct alignment_marker alignment_marker_t;

struct alignment_marker_set {
  alignment_marker_t ** markers;
  int max_markers;
  int num_layers;
};

typedef struct alignment_marker_set alignment_marker_set_t;

/* functions */

alignment_marker_set_t * amset_create(unsigned int num_layers);
ret_t amset_destroy(alignment_marker_set_t * set);
alignment_marker_t * amset_get_marker(const alignment_marker_set_t * const set, unsigned int layer, 
				      MARKER_TYPE marker_type);
ret_t amset_add_marker(alignment_marker_set_t * set, unsigned int layer, 
		       MARKER_TYPE marker_type,
		       unsigned int x, unsigned int y);

ret_t amset_replace_marker(alignment_marker_set_t * set, unsigned int layer, 
			   MARKER_TYPE marker_type,
			   unsigned int x, unsigned int y);


int amset_complete(const alignment_marker_set_t * const set);

void amset_print(const alignment_marker_set_t * const set);

ret_t amset_calc_transformation(const alignment_marker_set_t * const set,
				double * const scaling_x, double * const scaling_y, 
				int * const shift_x, int * const shift_y);


ret_t amset_apply_transformation_to_markers(alignment_marker_set * const set,
					  double * const scaling_x, double * const scaling_y, 
					  int * const shift_x, int * const shift_y);

char * amset_marker_type_to_str(MARKER_TYPE m_type);
MARKER_TYPE amset_mtype_str_to_mtype(const char * const marker_type_str);

#endif
