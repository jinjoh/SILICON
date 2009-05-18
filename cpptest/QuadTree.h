
#ifndef __QUADTREE_H__
#define __QUADTREE_H__

#include "Rectangle.h"
#include <vector>
#include <list>
#include <assert.h>
#include "globals.h"

template <typename T>
class QuadTree {

  //friend class QuadTree<T>::iterator;

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
  BoundingBox const & get_bounding_box_for_object(T const * object);
  BoundingBox const & get_bounding_box_for_object(T const & object);

  ret_t split();
  ret_t reinsert_objects();

 public:
  QuadTree(BoundingBox const & box, int max_entries = 200);
  ~QuadTree();

  bool is_leave();

  ret_t insert(T object);
  ret_t remove(T object);
  unsigned int size();


};

template <typename T>
class iterator {
 private:
  QuadTree<T> * node;
  typename std::vector<QuadTree<T> >::iterator subtree_nodes;
  typename std::vector<QuadTree<T> >::iterator subtree_nodes_end;
  typename std::list<T>::iterator children;
  typename std::list<T>::iterator children_end;
  
 public:

  iterator(QuadTree<T> node, 
	   typename std::vector<QuadTree<T> >::iterator subtree_nodes,
	   typename std::list<T>::iterator children,
	   typename std::vector<QuadTree<T> >::iterator subtree_nodes_end,
	   typename std::list<T>::iterator children_end) {

    this->node = node;
    this->subtree_nodes = subtree_nodes;
    this->children = children;
    this->subtree_nodes_end = subtree_nodes_end;
    this->children_end = children_end;
  }
  ~iterator() {}

  iterator& operator=(const iterator& other) {
    node = other.node;
    subtree_nodes = other.subtree_nodes;
    children = other.children;
    return(*this);
  }

  bool operator==(const iterator& other) {
    return (node == other.node &&
	    subtree_nodes == other.subtree_nodes &&
	    children = other.children);
  }

  bool operator!=(const iterator& other) {
    return !(this == other);
  }

  iterator& operator++() {
    
    if(++children == children_end) {
      
    }

    return(*this);
  }


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
BoundingBox const & QuadTree<T>::get_bounding_box_for_object(T const & object) {
  return object.get_bounding_box();
}

template <typename T>
BoundingBox const & QuadTree<T>::get_bounding_box_for_object(T const * object) {
  assert(object != NULL);
  return object->get_bounding_box();
}

template <typename T>
ret_t QuadTree<T>::split() {
  if(box.get_width() > bbox_min_size &&
     box.get_height() > bbox_min_size &&
     is_leave()) {

    BoundingBox nw(box.get_min_x(), box.get_min_y(), 
		   box.get_center_x(), box.get_center_y());
    
    
    BoundingBox sw(box.get_min_x(), box.get_center_y() + 1,
		   box.get_center_x(), box.get_max_y());


    BoundingBox ne(box.get_center_x() + 1, box.get_min_y(),
		   box.get_max_x(), box.get_center_y());


    BoundingBox se(box.get_center_x() + 1, box.get_center_y() + 1,
		   box.get_max_x(),  box.get_max_y());


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
      it != children.end();
      ++it) {
    insert(*it);
  }
  
  return RET_OK;
}

template <typename T>
ret_t QuadTree<T>::insert(T object) {

  ret_t ret;
  const BoundingBox & bbox = get_bounding_box_for_object(object);

  QuadTree<T> * const found = traverse_downto_bounding_box(bbox);
  assert(found != NULL);

  if(found != NULL) {

    if(((found->children).size() == max_entries) && found->is_leave()) {

      if(RET_IS_NOT_OK(ret = found->split())) return ret;
      if(RET_IS_NOT_OK(ret = found->reinsert_objects())) return ret;
      return found->insert(object);
    }
    else {
      children.push_back(object);
      return RET_OK;
    }
  }
  return RET_ERR;
}

template <typename T>
ret_t QuadTree<T>::remove(T object) {
  ret_t ret;
  const BoundingBox & bbox = get_bounding_box_for_object(object);

  QuadTree<T> * const found = traverse_downto_bounding_box(bbox);
  assert(found != NULL);
  if(found != NULL) {
    found->children.remove(object);

    if(!found->is_leave() &&
       found->subtree_nodes[NW].size() == 0 &&
       found->subtree_nodes[NE].size() == 0 &&
       found->subtree_nodes[SW].size() == 0 &&
       found->subtree_nodes[SE].size() == 0) {
      found->subtree_nodes.clear();
    }

    return RET_OK;
  }
  return RET_ERR;
}


template <typename T>
bool QuadTree<T>::is_leave() {
  return subtree_nodes.size() == 4 ? false : true;
}

template <typename T>
QuadTree<T> * const QuadTree<T>::traverse_downto_bounding_box(BoundingBox const & box) {

  if(is_leave()) return this;

  for(typename std::vector< QuadTree<T> >::iterator it = subtree_nodes.begin();
      it != subtree_nodes.end();
      ++it) {

    const BoundingBox & sub_bbox = (*it).box.get_bounding_box();

    if(box.in_bounding_box(sub_bbox)) { // sub_bbox within box?
      return (*it).traverse_downto_bounding_box(box);
    }
  }
  
  return this;

}

template <typename T>
unsigned int QuadTree<T>::size() {
  unsigned int this_node = children.size();
  unsigned int sub_nodes = 0;
  if(!is_leave()) {
    for(typename std::vector< QuadTree<T> >::iterator it = subtree_nodes.begin();
	it != subtree_nodes.end();
	++it) {
      sub_nodes += (*it).size();
    }
  }
  return this_node + sub_nodes;
}

/* missing:
   - traverse in region
   - traverse complete in region
   - traverse complete
   - get object(s) at
   - find object (?)
*/

#endif
