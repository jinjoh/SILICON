#ifndef __SHAPE_H__
#define __SHAPE_H__


#include "Rectangle.h"

typedef class Rectangle BoundingBox;

class Shape {

 public:

  virtual BoundingBox const& get_bounding_box() const = 0;
  virtual bool in_shape(int x, int y) = 0;
  virtual bool in_bounding_box(BoundingBox const& bbox) const = 0;

  virtual void shift_x(int delta_x) = 0;
  virtual void shift_y(int delta_y) = 0;
  virtual void shift(int delta_x, int delta_y) = 0;

  //virtual bool operator==(const Shape& other) const = 0;
  //virtual bool operator!=(const Shape& other) const = 0;
};

#endif
