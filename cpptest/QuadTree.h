
#ifndef __QUADTREE_H__
#define __QUADTREE_H__

#include "Rectangle.h"
#include <vector>
#include <list>
#include <assert.h>
#include "globals.h"
#include <iostream>


template<typename T>
struct is_pointer {
  static const bool value = false;
};

template<typename T> 
struct is_pointer<T*> {
  static const bool value = true;
}; 


template<bool b> 
struct get_bbox_trait_selector { 
  template<typename T>
  static BoundingBox const & get_bounding_box_for_object(T object) {
    return object.get_bounding_box();
  }
}; 

template <> 
struct get_bbox_trait_selector<true> { 
  template<typename T>
  static BoundingBox const & get_bounding_box_for_object(T object) {
    return object->get_bounding_box();
  }
}; 


template <typename T> class QuadTree;

#include "QuadTreeDownIterator.h"
#include "QuadTreeRegionIterator.h"

template <typename T>
class QuadTree {
  
  friend class region_iterator<T>;
  friend class down_iterator<T>;
 private:

  const static int NW = 0;
  const static int NE = 1;
  const static int SW = 2;
  const static int SE = 3;

  const static int bbox_min_size = 10;

  unsigned int  max_entries;

  BoundingBox box;
  std::vector<QuadTree<T> > subtree_nodes;
  std::list<T> children;

  QuadTree * parent;

  QuadTree(BoundingBox const & box, QuadTree * parent, int max_entries = 200);

  QuadTree<T> * const traverse_downto_bounding_box(BoundingBox const & box);

  ret_t split();
  ret_t reinsert_objects();

  
 public:

  QuadTree(BoundingBox const & box, int max_entries = 200);
  ~QuadTree();

  bool is_leave() const;

  ret_t insert(T object);
  ret_t remove(T object);
  unsigned int total_size() const;
  unsigned int depth() const;

  int get_width();
  int get_height();

  down_iterator<T> down_iter_begin();
  down_iterator<T> down_iter_end();

  region_iterator<T> region_iter_begin(int min_x, int max_x, int min_y, int max_y);
  region_iterator<T> region_iter_begin(BoundingBox const & bbox);
  region_iterator<T> region_iter_end();
};




template <typename T>
QuadTree<T>::QuadTree(BoundingBox const & box, int max_entries) {
  this->box = box;
  this->max_entries = max_entries;
  parent = NULL;  
}

template <typename T>
QuadTree<T>::QuadTree(BoundingBox const & box, QuadTree * parent, int max_entries) {
  this->box = box;
  this->max_entries = max_entries;
  this->parent = parent;
}

template <typename T>
QuadTree<T>::~QuadTree() {
}

template <typename T>
int QuadTree<T>::get_width() {
  return box.get_width();
}

template <typename T>
int QuadTree<T>::get_height() {
  return box.get_height();
}



template <typename T>
ret_t QuadTree<T>::split() {
  if(box.get_width() > bbox_min_size &&
     box.get_height() > bbox_min_size &&
     is_leave()) {

    BoundingBox nw(box.get_min_x(), box.get_center_x(),
		   box.get_min_y(), box.get_center_y());
    
    
    BoundingBox sw(box.get_min_x(), box.get_center_x(),
		   box.get_center_y() + 1, box.get_max_y());


    BoundingBox ne(box.get_center_x() + 1, box.get_max_x(),
		   box.get_min_y(), box.get_center_y());


    BoundingBox se(box.get_center_x() + 1, box.get_max_x(),
		   box.get_center_y() + 1,  box.get_max_y());


    QuadTree<T> node_nw(nw, this);
    QuadTree<T> node_ne(ne, this);
    QuadTree<T> node_sw(sw, this);
    QuadTree<T> node_se(se, this);

    subtree_nodes.push_back(node_nw);
    subtree_nodes.push_back(node_ne);
    subtree_nodes.push_back(node_sw);
    subtree_nodes.push_back(node_se);

    return RET_OK;
  }
  assert(1 == 0);
  return RET_ERR;
}

template <typename T>
ret_t QuadTree<T>::reinsert_objects() {
  
  typename std::list<T> children_copy = children;
  
  children.clear();

  for(typename std::list<T>::iterator it = children_copy.begin();
      it != children_copy.end();
      ++it) {
    insert(*it);
  }
  
  return RET_OK;
}

template <typename T>
ret_t QuadTree<T>::insert(T object) {

  ret_t ret;
  const BoundingBox & bbox = get_bbox_trait_selector<is_pointer<T>::value>::get_bounding_box_for_object(object);

  QuadTree<T> * found = traverse_downto_bounding_box(bbox);
  assert(found != NULL);

  if(found != NULL) {

    if((found->children.size() >= max_entries) && found->is_leave()) {

      if(RET_IS_NOT_OK(ret = found->split())) return ret;
      if(RET_IS_NOT_OK(ret = found->reinsert_objects())) return ret;
      found->children.push_back(object);
      return RET_OK;
    }
    else {
      found->children.push_back(object);
      return RET_OK;
    }
  }
  return RET_ERR;
}

template <typename T>
ret_t QuadTree<T>::remove(T object) {
  const BoundingBox & bbox = get_bbox_trait_selector<is_pointer<T>::value>::get_bounding_box_for_object(object);

  QuadTree<T> * found = traverse_downto_bounding_box(bbox);
  assert(found != NULL);
  if(found != NULL) {
    found->children.remove(object);

    if(!found->is_leave() &&
       found->subtree_nodes[NW].children.size() == 0 &&
       found->subtree_nodes[NE].children.size() == 0 &&
       found->subtree_nodes[SW].children.size() == 0 &&
       found->subtree_nodes[SE].children.size() == 0) {
      found->subtree_nodes.clear();
    }

    return RET_OK;
  }
  return RET_ERR;
}


template <typename T>
bool QuadTree<T>::is_leave() const {
  return subtree_nodes.size() == 4 ? false : true;
}

template <typename T>
QuadTree<T> * const QuadTree<T>::traverse_downto_bounding_box(BoundingBox const & box) {

  if(is_leave()) return this;

  for(typename std::vector< QuadTree<T> >::iterator it = subtree_nodes.begin();
      it != subtree_nodes.end();
      ++it) {

    const BoundingBox & sub_bbox = (*it).box.get_bounding_box();

    if(sub_bbox.in_bounding_box(box)) { // sub_bbox within box?
      return (*it).traverse_downto_bounding_box(box);
    }
  }
  
  return this;
}

template <typename T>
unsigned int QuadTree<T>::total_size() const {
  unsigned int this_node = children.size();
  unsigned int sub_nodes = 0;
  if(!is_leave()) {
    for(typename std::vector< QuadTree<T> >::const_iterator it = subtree_nodes.begin();
	it != subtree_nodes.end();
	++it) {
      sub_nodes += (*it).total_size();
    }
  }
  return this_node + sub_nodes;
}

template <typename T>
unsigned int QuadTree<T>::depth() const {

  unsigned int max_d = 0;
  if(!is_leave()) {
    for(typename std::vector< QuadTree<T> >::const_iterator it = subtree_nodes.begin();
	it != subtree_nodes.end();
	++it) {
      
      unsigned int d = (*it).depth();
      max_d = MAX(max_d, d);
    }
  }
  return 1 + max_d;
}


template <typename T>
down_iterator<T> QuadTree<T>::down_iter_begin() {
  return down_iterator<T>(this);
}

template <typename T>
down_iterator<T> QuadTree<T>::down_iter_end() {
  return down_iterator<T>(NULL);
}

template <typename T>
region_iterator<T> QuadTree<T>::region_iter_begin(int min_x, int max_x, int min_y, int max_y) {

  BoundingBox bbox(min_x, max_x, min_y, max_y);
  return region_iter_begin(bbox);
}

template <typename T>
region_iterator<T> QuadTree<T>::region_iter_begin(BoundingBox const & bbox) {

  QuadTree<T> * found = traverse_downto_bounding_box(bbox);
  assert(found != NULL);
  if(found != NULL)
    return region_iterator<T>(this, found, bbox);
  else
    return region_iterator<T>(NULL, NULL, bbox);
}

template <typename T>
region_iterator<T> QuadTree<T>::region_iter_end() {
  return region_iterator<T>(NULL, NULL);
}


/* missing:
   - get object(s) at
   - find object (?)
*/


#include "QuadTreeIterator.h"
#include "QuadTreeRegionIterator.h"


#endif
