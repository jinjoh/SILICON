#ifndef __QUADTREEREGIONITERATOR_H__
#define	__QUADTREEREGIONITERATOR_H__

#include "QuadTree.h"

template<typename T>
class region_iterator : public std::iterator<std::forward_iterator_tag, T> {

 private:
  down_iterator<T> subnode_iter;
  down_iterator<T> subnode_iter_end;
  QuadTree<T> 
    * node, // the subtree, where we need to iterate over all objectes
    * tree; // the whole quad tree 
  BoundingBox bbox;
  typename std::list<T>::iterator children_iter;
  typename std::list<T>::iterator children_iter_end;

  mutable T i;
  bool done;
 public:
  region_iterator(QuadTree<T> * tree, QuadTree<T> * node);
  region_iterator(QuadTree<T> * tree, QuadTree<T> * node, BoundingBox const & bbox);
  virtual ~region_iterator() {}
  virtual region_iterator& operator=(const region_iterator& other);
  virtual region_iterator& operator++();
  virtual bool operator==(const region_iterator& other) const;
  virtual bool operator!=(const region_iterator& other) const;
  virtual T * operator->() const;
  virtual T operator*() const;
  };

template<typename T>
region_iterator<T>::region_iterator(QuadTree<T> * tree,
				    QuadTree<T> * node) : 
  subnode_iter(node), 
  subnode_iter_end(node->down_iter_end()),
  done(false) {
  
  this->node = node;
  this->tree = tree;


  if(tree->total_size() == 0) done = true;
  else {
    if(node != NULL && node->parent != NULL) {
      children_iter = node->parent->children.begin();
      children_iter_end = node->parent->children.end();
    }
    
    if(node == NULL && tree == NULL) {
      children_iter = children_iter_end = node->children.end();
      done = true;
    }
  }
}

template<typename T>
region_iterator<T>::region_iterator(QuadTree<T> * tree,
				    QuadTree<T> * node,
				    BoundingBox const & bounding_box) : 
subnode_iter(node), 
  subnode_iter_end(node->down_iter_end()),
  bbox(bounding_box),
  done(false) {
  
  this->node = node;
  this->tree = tree;

  if(tree->total_size() == 0) done = true;
  else {
    if(node != NULL && node->parent != NULL) {
      children_iter = node->parent->children.begin();
      children_iter_end = node->parent->children.end();
    }
    
    if(node == NULL && tree == NULL) {
      children_iter = children_iter_end = node->children.end();
      done = true;
    }
  }

}

template<typename T>
region_iterator<T>& region_iterator<T>::operator++() {

  if(done) return (*this);

  // go down in quad tree via normal qtree iterator
  if(subnode_iter != subnode_iter_end) {

    // skip object(s) if its bounding box s not in the search region

    while(subnode_iter != subnode_iter_end &&
	  !bbox.intersects(get_bbox_trait_selector<is_pointer<T>::value>::get_bounding_box_for_object(*subnode_iter))) {
      BoundingBox const& b = get_bbox_trait_selector<is_pointer<T>::value>::get_bounding_box_for_object(*subnode_iter);
      std::cout << "current:" << std::endl;
      b.print();
      std::cout << "search box:" << std::endl;
      bbox.print();
      std::cout << "increment1-a" << std::endl;
      ++subnode_iter;
    }

    std::cout << "increment1-b" << std::endl;
    ++subnode_iter;
  }

  // iterated over subtree objects, now go up in qtree
  if(subnode_iter == subnode_iter_end) {
    std::cout << "-> root: iterating over children" << std::endl;

    // iterate over
    while(children_iter != children_iter_end &&
	  ! bbox.intersects(get_bbox_trait_selector<is_pointer<T>::value>::get_bounding_box_for_object(*children_iter))) {
      std::cout << "increment2" << std::endl;
      ++children_iter;
    }
    
    if(children_iter == children_iter_end) {
      if(node != tree) {
	node = node->parent;
	
	children_iter = node->children.begin();
	children_iter_end = node->children.end();
      }
    }

    if(children_iter != children_iter_end) {
      std::cout << "increment2b" << std::endl;
      ++children_iter;
    }
    else {
      std::cout << "set iterator into finished state" << std::endl;
    
      done = true;
    }
  }

  return (*this);
}

template<typename T>
region_iterator<T>& region_iterator<T>::operator=(const region_iterator<T> & other) {

  subnode_iter = other.subnode_iter;
  subnode_iter_end = other.subnode_iter_end;

  node = other.node;
  tree = other.tree;
  bbox = other.bbox;
  done = other.done;

  children_iter = other.children_iter;
  children_iter_end = other.children_iter_end;
  
  return(*this);
}

template<typename T>
bool region_iterator<T>::operator==(const region_iterator<T> & other) const {


  if(done == true && done == other.done)
    return true;
    
  else if(subnode_iter == other.subnode_iter &&
	  subnode_iter_end == other.subnode_iter_end &&
     
	  node == other.node &&
	  tree == other.tree &&
	  done == other.done &&
	  bbox == other.bbox &&
	  children_iter == other.children_iter &&
	  children_iter_end == other.children_iter_end) {
      
      return true;
    
  }
  
  else {
    return false;
  }
}


template<typename T>
bool region_iterator<T>::operator!=(const region_iterator<T>& other) const {
  return !(*this == other);
}

template<typename T>
T * region_iterator<T>::operator->() const {
  if(subnode_iter != subnode_iter_end) {
    i = *subnode_iter;
    return &i;
  }
  else return &*children_iter;

}

template<typename T>
T region_iterator<T>::operator*() const {
  //  std::cout << std::endl << "deref" << std::endl;
  if(subnode_iter != subnode_iter_end) return *subnode_iter;
  else return *children_iter;

}

#endif

