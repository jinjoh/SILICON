#ifndef __HLOBJECTSET_H__
#define __HLOBJECTSET_H__

#include "logic_model.h"
#include <set>
#include <list>

using namespace std;


class HlObjectSet {

  class HlObject {

  private:
    pair<LM_OBJECT_TYPE, object_ptr_t *> p;
    
  public:
    HlObject(LM_OBJECT_TYPE object_type, object_ptr_t * obj_ptr);
    HlObject(const HlObject & o);
    bool operator==(const HlObject &a);
    void highlight(bool state);
  };
  
 private:
  list<class HlObject> objects;

 public: 
  void clear();
  void highlight(bool state);
  void add(LM_OBJECT_TYPE object_type, object_ptr_t * obj_ptr);
  void remove(LM_OBJECT_TYPE object_type, object_ptr_t * obj_ptr);
};

#endif
