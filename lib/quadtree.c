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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "quadtree.h"
#include "globals.h"

#define PRINT_SPACE(level) {int i = level; while(i-- > 0) printf("    "); }

#define QUADTREE_IS_LEAVE(qtree) \
  (qtree->ne == NULL && qtree->se == NULL && \
   qtree->sw == NULL && qtree->nw == NULL)

quadtree_t * quadtree_create(unsigned int min_x, unsigned int min_y, 
			     unsigned int max_x, unsigned int max_y) {

  quadtree_t * ptr;

  //debug(TM, "\tcreate qt %d,%d -- %d,%d\n", min_x, min_y, max_x, max_y);
  assert(min_x < max_x);
  assert(min_y < max_y);

  if((ptr = (quadtree_t *) malloc(sizeof(struct quadtree))) == NULL) return NULL;
  memset(ptr, 0, sizeof(quadtree_t));

  //debug(TM, "\t-> done, got = %p\n", ptr);

  ptr->min_x = min_x;
  ptr->min_y = min_y;
  ptr->max_x = max_x;
  ptr->max_y = max_y;

  return ptr;
}


/**
 * This method destroys a quadtree and objects within it. It also
 * free()s referenced objects.
 */
void quadtree_destroy(quadtree_t *  qtree) {

  // free qtree object
  quadtree_object_t * ptr = qtree->objects;
  while(ptr != NULL) {
    quadtree_object_t * next = ptr->next;
    if(ptr->object) free(ptr->object);
    free(ptr);
    ptr = next;
  }

  // free children
  if(QUADTREE_IS_LEAVE(qtree)) {
    free(qtree);
  }
  else {
  
    if(qtree->ne != NULL) {
      quadtree_destroy(qtree->ne);
      qtree->ne = NULL;
    }
    
    if(qtree->se != NULL) {
      quadtree_destroy(qtree->se);
      qtree->se = NULL;
    }

    if(qtree->sw != NULL) {
      quadtree_destroy(qtree->sw);
      qtree->sw = NULL;
    }

    if(qtree->nw != NULL) {
      quadtree_destroy(qtree->nw);
      qtree->nw = NULL;
    }
  }
}


/** 
 * Traverse the quadtree down to a node, thats boundig box
 * is the smallest one greater than (x0,y0), (x1,y1). Call cb_func
 * for every quadtree node.
 * 
 */
quadtree_t * quadtree_traverse_downto_bbox(quadtree_t * qtree,
					   unsigned int min_x, unsigned int min_y,
					   unsigned int max_x, unsigned int max_y,
					   quadtree_traverse_func_t cb_func, void * data_ptr) {

  assert(qtree != NULL);

  if(cb_func) {
    if(RET_IS_NOT_OK((*(cb_func))(qtree, data_ptr))) {
      return NULL;
    }
  }

  if(QUADTREE_IS_LEAVE(qtree)) {
    /*assert(qtree->min_x <= min_x);
    assert(qtree->max_x > max_x);
    assert(qtree->min_y <= min_y);
    assert(qtree->max_y > max_y);*/
    return qtree;
  }

  if(max_x < qtree->nw->max_x) {

    if(max_y < qtree->nw->max_y) // NW
      return (qtree->nw == NULL) ? qtree : quadtree_traverse_downto_bbox(qtree->nw, min_x, min_y, max_x, max_y, cb_func, data_ptr);
    else if(min_y >= qtree->sw->min_y) // SW
      return (qtree->sw == NULL) ? qtree : quadtree_traverse_downto_bbox(qtree->sw, min_x, min_y, max_x, max_y, cb_func, data_ptr);
    
  }
  else if(min_x >= qtree->ne->min_x) {
    if(max_y < qtree->ne->max_y) // NE
      return (qtree->ne == NULL) ? qtree : quadtree_traverse_downto_bbox(qtree->ne, min_x, min_y, max_x, max_y, cb_func, data_ptr);
    else if(min_y >= qtree->se->min_y) // SE
      return (qtree->se == NULL) ? qtree : quadtree_traverse_downto_bbox(qtree->se, min_x, min_y, max_x, max_y, cb_func, data_ptr);
  }

  return qtree; 
}

quadtree_object_t * quadtree_get_object_at(quadtree_t * qtree, unsigned int x, unsigned int y,
					   quadobject_traverse_func_t cb_func, void * data_ptr) {
    assert(qtree != NULL);

    quadtree_object_t * optr = qtree->objects;
    while(optr) {
      if(x >= optr->min_x && x <= optr->max_x && y >= optr->min_y && y <= optr->max_y) {
	if((*(cb_func))(optr, data_ptr)) {
	  return optr;
	}
      }
      optr = optr->next;
    }
    if(QUADTREE_IS_LEAVE(qtree)) return NULL;

    if(x < qtree->nw->max_x) {
      if(y < qtree->nw->max_y) 
	return  quadtree_get_object_at(qtree->nw, x, y, cb_func, data_ptr);
      else return quadtree_get_object_at(qtree->sw, x, y, cb_func, data_ptr);
    }
    else {
      if(y < qtree->ne->max_y)
	return  quadtree_get_object_at(qtree->ne, x, y, cb_func, data_ptr);
      else return quadtree_get_object_at(qtree->se, x, y, cb_func, data_ptr);
    }
    return NULL;
}

quadtree_object_t * quadtree_find_object(quadtree_t * qtree,
					 quadobject_traverse_func_t cb_func, void * data_ptr) {
  assert(qtree != NULL);

  quadtree_object_t * optr = qtree->objects;
  while(optr) {
    if((*(cb_func))(optr, data_ptr)) return optr;
    optr = optr->next;
  }
  if(QUADTREE_IS_LEAVE(qtree)) return NULL;
  
  if(qtree->nw != NULL) {
    optr = quadtree_find_object(qtree->nw, cb_func, data_ptr);
    if(optr) return optr;
  }

  if(qtree->sw != NULL) {
    optr = quadtree_find_object(qtree->sw, cb_func, data_ptr);
    if(optr) return optr;
  }

  if(qtree->ne != NULL) {
    optr = quadtree_find_object(qtree->ne, cb_func, data_ptr);
    if(optr) return optr;
  }

  if(qtree->se != NULL) {
    optr = quadtree_find_object(qtree->se, cb_func, data_ptr);
    if(optr) return optr;
  }


  return NULL;
}

/**
 * Traverse a quadtree down to leaf node(s), going through all nodes that may have objects
 * within region.
 */
ret_t quadtree_traverse_complete_within_region(quadtree_t * qtree,
					       unsigned int min_x, unsigned int min_y,
					       unsigned int max_x, unsigned int max_y,
					       quadtree_traverse_func_t cb_func, void * data_ptr) {
 
  ret_t ret;
  quadtree_t * qnode;

  assert(qtree);
  assert(cb_func);
  if(!qtree || !cb_func) return RET_INV_PTR;

  qnode = quadtree_traverse_downto_bbox(qtree, min_x, min_y, max_x, max_y, cb_func, data_ptr);
  if(!qnode) return RET_ERR;

  if(qnode->nw) if(RET_IS_NOT_OK(ret = quadtree_traverse_complete(qnode->nw, cb_func, data_ptr))) return ret;
  if(qnode->ne) if(RET_IS_NOT_OK(ret = quadtree_traverse_complete(qnode->ne, cb_func, data_ptr))) return ret;
  if(qnode->sw) if(RET_IS_NOT_OK(ret = quadtree_traverse_complete(qnode->sw, cb_func, data_ptr))) return ret;
  if(qnode->se) if(RET_IS_NOT_OK(ret = quadtree_traverse_complete(qnode->se, cb_func, data_ptr))) return ret;

  return RET_OK;
}

ret_t quadtree_traverse_complete(quadtree_t * qtree,
				 quadtree_traverse_func_t cb_func, void * data_ptr) {

  ret_t ret;

  assert(qtree != NULL);
  assert(cb_func != NULL);

  if(RET_IS_NOT_OK(ret = (*(cb_func))(qtree, data_ptr))) {
    return ret;
  }

  if(QUADTREE_IS_LEAVE(qtree)) return RET_OK;

  if(qtree->nw != NULL) ret = quadtree_traverse_complete(qtree->nw, cb_func, data_ptr);
  if(RET_IS_NOT_OK(ret)) return ret;

  if(qtree->sw != NULL) ret = quadtree_traverse_complete(qtree->sw, cb_func, data_ptr);
  if(RET_IS_NOT_OK(ret)) return ret;

  if(qtree->ne != NULL) ret = quadtree_traverse_complete(qtree->ne, cb_func, data_ptr);
  if(RET_IS_NOT_OK(ret)) return ret;

  if(qtree->se != NULL) ret = quadtree_traverse_complete(qtree->se, cb_func, data_ptr);
  if(RET_IS_NOT_OK(ret)) return ret;

  return RET_OK; 
}



ret_t quadtree_split_node(quadtree_t * qtree) {

  assert(qtree != NULL);
  if(qtree == NULL) return RET_INV_PTR;

  /*  debug(TM, "split node %p: %d,%d -- %d,%d\n", 
      qtree, qtree->min_x, qtree->min_y, qtree->max_x, qtree->max_y);*/
  
  if(qtree->max_x - qtree->min_x <= QTREE_MIN_SIZE || 
     qtree->max_y - qtree->min_y <= QTREE_MIN_SIZE) {
    
    debug(TM, "Can't split quadtree node %p. Please increase MAX_POINTS.\n", qtree);
    assert(0 == 1);
    //quadtree_print(qtree, 0);
    //puts("\n\n");
    return RET_ERR;
  }

  //debug(TM, "\tnw: ");
  qtree->nw = quadtree_create( qtree->min_x, 
			       qtree->min_y,
			       (qtree->max_x - qtree->min_x) /2 + qtree->min_x, 
			       (qtree->max_y - qtree->min_y) /2 + qtree->min_y);
  assert(qtree->nw != NULL);
  qtree->nw->parent = qtree;

  //debug(TM, "\tsw: ");
  qtree->sw = quadtree_create( qtree->min_x, 
			       (qtree->max_y - qtree->min_y) /2 + qtree->min_y,
			       (qtree->max_x - qtree->min_x) /2 + qtree->min_x, 
			       qtree->max_y);
  assert(qtree->sw != NULL);
  qtree->sw->parent = qtree;

  //debug(TM, "\tne:");
  qtree->ne = quadtree_create( (qtree->max_x - qtree->min_x) /2 + qtree->min_x, 
			       qtree->min_y,
			       qtree->max_x, 
			       (qtree->max_y - qtree->min_y) /2 + qtree->min_y);
  assert(qtree->ne != NULL);
  qtree->ne->parent = qtree;

  //debug(TM, "\tse: ");
  qtree->se = quadtree_create( (qtree->max_x - qtree->min_x) /2 + qtree->min_x, 
			       (qtree->max_y - qtree->min_y) /2 + qtree->min_y,
			       qtree->max_x, 
			       qtree->max_y);
  assert(qtree->se != NULL);
  qtree->se->parent = qtree;

  return RET_OK;
}



ret_t quadtree_reinsert_objects(quadtree_t * qtree) {
  assert(qtree != NULL);
  if(qtree == NULL) return RET_INV_PTR;

  quadtree_object_t 
    * chain_ptr = qtree->objects, 
    * chain_ptr_next = NULL;

  qtree->objects = NULL;
  qtree->num = 0;

  while(chain_ptr != NULL) {
    chain_ptr_next = chain_ptr->next;

    chain_ptr->parent = NULL;
    chain_ptr->next = NULL;

    if(quadtree_insert(qtree, chain_ptr) == NULL) return RET_ERR;

    chain_ptr = chain_ptr_next;
  }
  return RET_OK;
}


quadtree_object_t * quadtree_object_create(int object_type, void * obj_ptr,
					   unsigned int min_x, unsigned int min_y, 
					   unsigned int max_x, unsigned int max_y
					   ) {

  quadtree_object_t * obj = (quadtree_object_t *) malloc(sizeof(quadtree_object_t));
  if(obj) {
    memset(obj, 0, sizeof(quadtree_object_t));
    obj->min_x = MIN(min_x, max_x);
    obj->min_y = MIN(min_y, max_y);
    obj->max_x = MAX(min_x, max_x);
    obj->max_y = MAX(min_y, max_y);
    obj->object_type = object_type;
    obj->object = obj_ptr;
  }
  return obj;
}

/** Destroys a quadtree object. It is not a deep destroy, so you have to free
 * the memory of the referrenced object by yourself.
 */
ret_t quadtree_object_destroy(quadtree_object_t * obj) {
  if(!obj) return RET_INV_PTR;

  free(obj);

  return RET_OK;
}

quadtree_t * quadtree_insert(quadtree_t * qtree, quadtree_object * object) {

  assert(qtree != NULL);
  assert(object != NULL);
  //debug(TM, "insert");

  quadtree_t * found = quadtree_traverse_downto_bbox(qtree, 
						     object->min_x, object->min_y,
						     object->max_x, object->max_y,
						     NULL, NULL);

  assert(found != NULL);
  assert(object->min_x >= found->min_x); assert(object->max_x < found->max_x);
  assert(object->min_y >= found->min_y); assert(object->max_y < found->max_y);
  if(found == NULL) return NULL;

  if(found->num == MAX_POINTS && QUADTREE_IS_LEAVE(found)) {
    
    //quadtree_print(found, 0);

    if(RET_IS_NOT_OK(quadtree_split_node(found))) {
      debug(TM, "quadtree_split_node() failed");
      return NULL;
    }
    if(RET_IS_NOT_OK(quadtree_reinsert_objects(found))) {
      debug(TM, "quadtree_reinsert_objects() failed");
      return NULL;
    }

    //quadtree_print(found, 0);

    return quadtree_insert(found, object);
  }
  else {
    //debug(TM, "added");
    // add object on list head
    object->parent = found;
    object->next = found->objects;
    found->objects = object;

    found->num++;

    return found;
  }
}





void quadtree_print(quadtree_t * qtree, int level) {

  assert(qtree != NULL);
  if(qtree == NULL) return;

  PRINT_SPACE(level);
  printf("#%d node %p from (%d,%d) to (%d,%d)   %dx%d\n", 
	 level, qtree, 
	 qtree->min_x, qtree->min_y, qtree->max_x, qtree->max_y,
	 qtree->max_x - qtree->min_x, qtree->max_y - qtree->min_y);

  PRINT_SPACE(level);

  printf("+ NW: %p, NE: %p, SW: %p, SE: %p\n",
	 qtree->nw, qtree->ne, qtree->sw, qtree->se);

  quadtree_object_t * obj = qtree->objects;
  while(obj != NULL) {

    PRINT_SPACE(level);
    printf("+ obj type %d at (%d,%d) (%d,%d)\n", 
	   obj->object_type,
	   obj->min_x, obj->min_y, obj->max_x, obj->max_y);

    obj = obj->next;
  }
  
  //puts("");
  if(qtree->nw) quadtree_print(qtree->nw, level + 1);
  if(qtree->ne) quadtree_print(qtree->ne, level + 1);
  if(qtree->sw) quadtree_print(qtree->sw, level + 1);
  if(qtree->se) quadtree_print(qtree->se, level + 1);
}


/**
 * Remove a quadtree object from tree and destroy the quadtree object.
 */
ret_t quadtree_remove_object(quadtree_object_t * obj) {

  assert(obj);
  assert(obj->parent);
  if(!obj || !obj->parent) return RET_INV_PTR;

  quadtree_object_t * ptr = obj->parent->objects;
  
  if(obj == ptr) {
    obj->parent->objects = obj->next;
    quadtree_object_destroy(obj);
    obj->parent->num--;
    return RET_OK;
  }

  while(ptr) {
    if(obj == ptr->next) {
      ptr->next = obj->next;
      quadtree_object_destroy(obj);
      obj->parent->num--;
      return RET_OK;
    }

    ptr = ptr->next;
  }
  return RET_OK;
}
