#ifndef __QUADTREEITERATOR_H__
#define	__QUADTREEITERATOR_H__

#include "QuadTree.h"

template<typename T>
QuadTree<T>::iterator::iterator(QuadTree<T> * node) {
  this->node = NULL;
  if(node == NULL) {
      done = true;
  }
  else {
    open_list.push_back(node);
    done = false;
    next_node();
  }

}

template<typename T>
void QuadTree<T>::iterator::next_node() {
  if(node == NULL) {
    if(open_list.size() > 0) {
      node = open_list.front();
      open_list.pop_front();

      // add subtree nodes to open list
      for(typename std::vector<QuadTree<T> >::iterator it = node->subtree_nodes.begin();
	  it != node->subtree_nodes.end();
	  ++it)
	open_list.push_back(&*it);

      // reset iterator for current
      children_iter = node->children.begin();
      children_iter_end = node->children.end();

      //++children_iter;
    }
    else {
      done = true;
    }
  }

}

template<typename T>
typename QuadTree<T>::iterator& QuadTree<T>::iterator::operator++() {

  if(!done) {
    ++children_iter;
  }

  //std::cout << std::endl << "++ called" << std::endl;
  if(!done && children_iter == children_iter_end) {
    node = NULL;
    next_node();
  }


  return (*this);
}

template<typename T>
typename QuadTree<T>::iterator& QuadTree<T>::iterator::operator=(const QuadTree<T>::iterator& other) {
  node = other.node;
  done = other.done;
  open_list = other.open_list;
  children_iter = other.children_iter;
  children_iter_end = other.children_iter_end;
  return(*this);
}

template<typename T>
bool QuadTree<T>::iterator::operator==(const QuadTree<T>::iterator& other) {

    if(node == NULL && other.node == NULL)
        return (
          open_list == other.open_list &&
          done == other.done
          );
    else
        return (node == other.node &&
          children_iter == other.children_iter &&
          open_list == other.open_list &&
          done == other.done
          );
}


template<typename T>
bool QuadTree<T>::iterator::operator!=(const QuadTree<T>::iterator& other) {
  return !(*this == other);
}

template<typename T>
T * QuadTree<T>::iterator::operator->() const {
  return &*children_iter;
}

template<typename T>
T QuadTree<T>::iterator::operator*() const {
  //  std::cout << std::endl << "deref" << std::endl;

  return *children_iter;
}

#endif

