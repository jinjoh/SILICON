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

#ifndef __QUADTREE_H__
#define __QUADTREE_H__

#include "globals.h"

#define MAX_POINTS 200
#define QTREE_MIN_SIZE 1

typedef struct quadtree quadtree_t;
typedef struct quadtree_object quadtree_object_t;

struct quadtree_object {
  unsigned int min_x, min_y, max_x, max_y;

  int object_type;
  void * object;
  quadtree_t * parent;
  
  struct quadtree_object * next;
};

struct quadtree {
  quadtree_t * ne, * se, * sw, * nw;
  quadtree_t * parent;

  quadtree_object_t * objects;
  int num;
  
  unsigned int min_x, max_x, min_y, max_y;
};




typedef ret_t (*quadtree_traverse_func_t)(quadtree_t *, void * data_ptr);
typedef int (*quadobject_traverse_func_t)(quadtree_object_t *, void * data_ptr);


quadtree_t * quadtree_create(unsigned int min_x, unsigned int min_y,
			     unsigned int max_x, unsigned int max_y);

void quadtree_destroy(quadtree_t * qtree);

quadtree_t * quadtree_traverse_downto_bbox(quadtree_t * qtree,
			       unsigned int min_x, unsigned int min_y,
			       unsigned int max_x, unsigned int max_y,
			       quadtree_traverse_func_t cb_func, void * cb_data_ptr);

ret_t quadtree_traverse_complete(quadtree_t * qtree,
				 quadtree_traverse_func_t cb_func, void * data_ptr);


ret_t quadtree_traverse_complete_within_region(quadtree_t * qtree,
					       unsigned int min_x, unsigned int min_y,
					       unsigned int max_x, unsigned int max_y,
					       quadtree_traverse_func_t cb_func, void * data_ptr);

quadtree_t * quadtree_insert(quadtree_t * qtree, quadtree_object_t * object);

void quadtree_print(quadtree_t * qtree, int level);

quadtree_object_t * quadtree_object_create(int object_type, void * obj_ptr,
					   unsigned int min_x, unsigned int min_y, 
					   unsigned int max_x, unsigned int max_y);

ret_t quadtree_object_destroy(quadtree_object_t * obj);

quadtree_object_t * quadtree_get_object_at(quadtree_t * qtree, unsigned int x, unsigned int y,
					   quadobject_traverse_func_t cb_func, void * data_ptr);

quadtree_object_t * quadtree_find_object(quadtree_t * qtree, 
					 quadobject_traverse_func_t cb_func, void * data_ptr);

ret_t quadtree_remove_object(quadtree_object_t * obj);

#endif
