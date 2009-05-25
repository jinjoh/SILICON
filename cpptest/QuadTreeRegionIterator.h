#ifndef __QUADTREEREGIONITERATOR_H__
#define	__QUADTREEREGIONITERATOR_H__

#include "QuadTree.h"

template<typename T>
QuadTree<T>::region_iterator::region_iterator(QuadTree<T> * tree,
        QuadTree<T> * node) : subnode_iter(node) {

  this->node = node;
  this->tree = tree;
  subnode_iter_end = node->end();

}

template<typename T>
QuadTree<T>::region_iterator::region_iterator(QuadTree<T> * tree,
        QuadTree<T> * node,
        BoundingBox const & bbox) : subnode_iter(node) {

  this->node = node;
  this->tree = tree;
  subnode_iter_end = node->end();
  this->bbox = bbox;

}

template<typename T>
typename QuadTree<T>::region_iterator& QuadTree<T>::region_iterator::operator++() {

  while(subnode_iter != subnode_iter_end &&
          bbox.in_bounding_box(algorithm_selector<is_pointer<T>::value>::get_bounding_box_for_object(*subnode_iter))
          ) {
      ++subnode_iter;
      // check and increment
  }


   {
      if(children_iter != children_iter_end) {
          ++children_iter;
          // check and increment
      }
      else {
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
typename QuadTree<T>::region_iterator& QuadTree<T>::region_iterator::operator=(const QuadTree<T>::region_iterator& other) {
  return(*this);
}

template<typename T>
bool QuadTree<T>::region_iterator::operator==(const QuadTree<T>::region_iterator& other) {
    return true;
}


template<typename T>
bool QuadTree<T>::region_iterator::operator!=(const QuadTree<T>::region_iterator& other) {
  return !(*this == other);
}

template<typename T>
T * QuadTree<T>::region_iterator::operator->() const {
  if(subnode_iter != subnode_iter_end) return &*children_iter;
  else return &*children_iter;
}

template<typename T>
T QuadTree<T>::region_iterator::operator*() const {
  //  std::cout << std::endl << "deref" << std::endl;
  if(subnode_iter != subnode_iter_end) return *subnode_iter;
  else return *children_iter;
}

#endif

