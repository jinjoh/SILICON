#ifndef __RECTANGLE_H__
#define __RECTANGLE_H__

#include "BoundingBox.h"
#include "Shape.h"

class Rectangle : public Shape {

 private:
  int min_x, max_x, min_y, max_y;
  BoundingBox bbox;

 public:
  Rectangle();

  Rectangle(int min_x, int max_x, int min_y, int max_y);
  Rectangle(const Rectangle&);

  BoundingBox const& get_bounding_box() const;
  bool in_shape(int x, int y);
  bool in_bounding_box(BoundingBox const& bbox) const;
  bool operator==(const Rectangle& other) const;
  bool operator!=(const Rectangle& other) const;

  bool intersects(Rectangle const & rect) const;
  bool complete_within(Rectangle const & rect) const;

  int get_width() const;
  int get_height() const;

  int get_min_x() const;
  int get_max_x() const;
  int get_min_y() const;
  int get_max_y() const;

  int get_center_x() const;
  int get_center_y() const;

  void set_min_x(int min_x);
  void set_min_y(int min_y);
  void set_max_x(int max_x);
  void set_max_y(int max_y);

  void shift_x(int delta_x);
  void shift_y(int delta_y);
  void shift(int delta_x, int delta_y);

};


#endif
