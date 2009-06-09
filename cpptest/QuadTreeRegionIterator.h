#ifndef __QUADTREEREGIONITERATOR_H__
#define	__QUADTREEREGIONITERATOR_H__

#include "QuadTree.h"

template<typename T>
QuadTree<T>::region_iterator::region_iterator(QuadTree<T> * tree,
					      QuadTree<T> * node) : 
  subnode_iter(node), 
  subnode_iter_end(node->end())
 {
  
  this->node = node;
  this->tree = tree;


  if(node != NULL && node->parent != NULL) {
    children_iter = node->parent->children.begin();
    children_iter_end = node->parent->children.end();
  }

}

template<typename T>
QuadTree<T>::region_iterator::region_iterator(QuadTree<T> * tree,
					      QuadTree<T> * node,
					      BoundingBox const & bounding_box) : 
  subnode_iter(node), 
  subnode_iter_end(node->end()),
  bbox(bounding_box) {
  
  this->node = node;
  this->tree = tree;

  if(node != NULL && node->parent != NULL) {
    children_iter = node->parent->children.begin();
    children_iter_end = node->parent->children.end();
  }

}

template<typename T>
typename QuadTree<T>::region_iterator& QuadTree<T>::region_iterator::operator++() {

  
  // go down in quad tree via normal qtree iterator
  if(subnode_iter != subnode_iter_end) {
    std::cout << "-> leaves: iterating over children" << std::endl;

    // skip object(s) if its bounding box s not in the search region
    if(subnode_iter == subnode_iter_end) {
      std::cout << "equals end!!!\n";
    }

    while(subnode_iter != subnode_iter_end &&
	  !bbox.intersects(algorithm_selector<is_pointer<T>::value>::get_bounding_box_for_object(*subnode_iter))) {
      std::cout << "increment" << std::endl;
      ++subnode_iter;
    }
  }

  // iterated over subtree objects, now go up in qtree
  else {
    std::cout << "-> root: iterating over children" << std::endl;

    // iterate over
    while(children_iter != children_iter_end &&
	  !bbox.in_bounding_box(algorithm_selector<is_pointer<T>::value>::get_bounding_box_for_object(*children_iter)))
      ++children_iter;
    
    if(children_iter == children_iter_end) {
      if(node != tree) {
	node = node->parent;
	
	children_iter = node->children.begin();
	children_iter_end = node->children.end();
      }
    }
  }

  return (*this);
}

template<typename T>
typename QuadTree<T>::region_iterator& 
QuadTree<T>::region_iterator::operator=(const QuadTree<T>::region_iterator& other) {

  subnode_iter = other.subnode_iter;
  subnode_iter_end = other.subnode_iter_end;

  node = other.node;
  tree = other.tree;
  bbox = other.bbox;

  children_iter = other.children_iter;
  children_iter_end = other.children_iter_end;
  
  return(*this);
}

template<typename T>
bool QuadTree<T>::region_iterator::operator==(const QuadTree<T>::region_iterator& other) const {

  if(subnode_iter == other.subnode_iter &&
     subnode_iter_end == other.subnode_iter_end &&

     node == other.node &&
     tree == other.tree &&
     bbox == other.bbox &&

     children_iter == other.children_iter &&
     children_iter_end == other.children_iter_end)
  
    return true;
  else return false;
}


template<typename T>
bool QuadTree<T>::region_iterator::operator!=(const QuadTree<T>::region_iterator& other) const {
  return !(*this == other);
}

template<typename T>
T * QuadTree<T>::region_iterator::operator->() const {
  if(subnode_iter != subnode_iter_end) return &*subnode_iter;
  else return &*children_iter;
}

template<typename T>
T QuadTree<T>::region_iterator::operator*() const {
  //  std::cout << std::endl << "deref" << std::endl;
  if(subnode_iter != subnode_iter_end) return *subnode_iter;
  else return *children_iter;
}

#endif

