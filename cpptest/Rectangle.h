#ifndef __RECTANGLE_H__
#define __RECTANGLE_H__

#include "Shape.h"

class Rectangle : public Shape {

 private:
  int min_x, max_x, min_y, max_y;

 public:
  Rectangle();

  Rectangle(int min_x, int max_x, int min_y, int max_y);
  Rectangle(const Rectangle&);

  BoundingBox const& get_bounding_box() const;
  bool in_shape(int x, int y);
  bool in_bounding_box(BoundingBox const& bbox) const;

  int get_width();
  int get_height();

  int get_min_x();
  int get_max_x();
  int get_min_y();
  int get_max_y();

  int get_center_x();
  int get_center_y();

  void set_min_x(int min_x);
  void set_min_y(int min_y);
  void set_max_x(int max_x);
  void set_max_y(int max_y);

  void shift_x(int delta_x);
  void shift_y(int delta_y);
  void shift(int delta_x, int delta_y);

};


#endif
