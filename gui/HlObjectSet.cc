#include "HlObjectSet.h"
#include <algorithm>

using namespace std;

HlObjectSet::HlObject::HlObject(LM_OBJECT_TYPE object_type, object_ptr_t * obj_ptr) : p(object_type, obj_ptr) {
 
}

HlObjectSet::HlObject::HlObject(const HlObject & o) {
  p = o.p;
}

void HlObjectSet::HlObject::highlight(bool state = true) {
  lmodel_set_select_state(p.first, p.second, state == true ? SELECT_STATE_DIRECT : SELECT_STATE_NOT);
}

bool HlObjectSet::HlObject::operator==(const HlObject &a) {
  return (a.p == p);
}


void HlObjectSet::clear() {
  highlight(false);
  objects.clear();
}

void HlObjectSet::highlight(bool state = true) {
  list<class HlObject>::iterator it;
  for(it = objects.begin(); it != objects.end(); ++it) {
    (*it).highlight(state);
  } 
}


void HlObjectSet::add(LM_OBJECT_TYPE object_type, object_ptr_t * obj_ptr) {
  HlObject o(object_type, obj_ptr);
  o.highlight();
  objects.push_back(o);
}


void HlObjectSet::remove(LM_OBJECT_TYPE object_type, object_ptr_t * obj_ptr) {

  list<HlObject>::iterator it = find(objects.begin(), objects.end(), 
				     HlObjectSet::HlObject(object_type, obj_ptr));
  if(it != objects.end()) {
    
    (*it).highlight(false);
    objects.erase(it);
  }

}
