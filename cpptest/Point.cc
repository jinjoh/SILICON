#include "globals.h"
//#include "Shape.h"
//#include "Rectangle.h"
#include "Point.h"

Point::Point() {
  x = y = 0;
}

Point::Point(int x, int y) : bbox(x, x, y, y) {
  this->x = x;
  this->y = y;
}

Point::Point(const Point& o) : bbox(x, x, y, y) {
  x = o.x;
  y = o.y;
}

BoundingBox const& Point::get_bounding_box() const {
  return bbox;
}

bool Point::in_shape(int x, int y) const {
  return (x == this->x && y == this->y);
}

bool Point::operator==(const Point& other) const {
    return (x == other.x && y == other.y);
}

bool Point::operator!=(const Point& other) const {
    return !(*this == other);
}

/**
 * Check, if bbox is within this.
 */
bool Point::in_bounding_box(BoundingBox const& bbox) const {
  return (x == bbox.get_min_x() &&
	  y == bbox.get_max_x() &&
	  bbox.get_width() == 0 &&
	  bbox.get_height() == 0);
}


int Point::get_x() const {
  return x;
}

int Point::get_y() const {
  return y;
}

void Point::set_x(int x) {
  this->x = x;
}

void Point::set_y(int y) {
  this->y = y;
}


void Point::shift_y(int delta_y) {
  y += delta_y;
}

void Point::shift_x(int delta_x) {
  x += delta_x;
}

void Point::shift(int delta_x, int delta_y) {
  shift_x(delta_x);
  shift_y(delta_y);
}

