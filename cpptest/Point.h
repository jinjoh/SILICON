#ifndef __POINT_H__
#define __POINT_H__

class Shape;
class Rectangle;
class Point;

#include "Shape.h"

class Point : public Shape {

 private:
  int x, y;
  BoundingBox bbox;

 public:
  Point();
  Point(int x, int y);
  Point(const Point& o);

  BoundingBox const& get_bounding_box() const;
  bool in_shape(int x, int y) const;
  bool in_bounding_box(BoundingBox const& bbox) const;

  bool operator==(const Point& other) const;
  bool operator!=(const Point& other) const;

  int get_x() const;
  int get_y() const;

  void set_x(int x);
  void set_y(int y);

  void shift_x(int delta_x);
  void shift_y(int delta_y);
  void shift(int delta_x, int delta_y);

};


#endif
