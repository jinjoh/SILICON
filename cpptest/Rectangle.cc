#include "Rectangle.h"
#include "globals.h"

Rectangle::Rectangle() {
  max_y = min_y = max_x = min_x = 0;
}

Rectangle::Rectangle(int min_x, int max_x, int min_y, int max_y) {
  this->min_x = MIN(min_x, max_x);
  this->max_x = MAX(min_x, max_x);
  this->min_y = MIN(min_y, max_y);
  this->max_y = MAX(min_y, max_y);
}

Rectangle::Rectangle(const Rectangle& o) {
  this->min_x = o.min_x;
  this->max_x = o.max_x;
  this->min_y = o.min_y;
  this->max_y = o.max_y;
}

BoundingBox const& Rectangle::get_bounding_box() const {
  return *this;
}

bool Rectangle::in_shape(int x, int y) {
  return (x >= min_x && x <= max_x && y >= min_y && y <= max_y) ? true : false;
}

/**
 * Check, if bbox is within this.
 */
bool Rectangle::in_bounding_box(BoundingBox const& bbox) const {
  if(min_x <= bbox.min_x &&
     max_x >= bbox.max_x &&
     min_y <= bbox.min_y &&
     max_y >= bbox.max_y) return true;
  return false;
}

int Rectangle::get_width() {
  return max_x - min_x;
}

int Rectangle::get_height() {
  return max_y - min_y;
}

int Rectangle::get_min_x() {
  return max_x;
}

int Rectangle::get_max_x() {
  return max_x;
}

int Rectangle::get_min_y() {
  return max_y;
}

int Rectangle::get_max_y() {
  return max_y;
}

void Rectangle::set_min_x(int min_x) {
  this->min_x = MIN(min_x, max_x);
  this->max_x = MAX(min_x, max_x);
}

void Rectangle::set_min_y(int min_y) {
  this->min_y = MIN(min_y, max_y);
  this->max_y = MAX(min_y, max_y);
}

void Rectangle::set_max_x(int max_x) {
  this->min_x = MIN(min_x, max_x);
  this->max_x = MAX(min_x, max_x);
}

void Rectangle::set_max_y(int max_y) {
  this->min_y = MIN(min_y, max_y);
  this->max_y = MAX(min_y, max_y);
}

void Rectangle::shift_y(int delta_y) {
  min_y += delta_y;
  max_y += delta_y;
}

void Rectangle::shift_x(int delta_x) {
  min_y += delta_x;
  max_y += delta_x;
}

void Rectangle::shift(int delta_x, int delta_y) {
  shift_x(delta_x);
  shift_y(delta_y);
}

int Rectangle::get_center_x() {
  return min_x + get_width() / 2;
}

int Rectangle::get_center_y() {
  return min_y + get_height() / 2;
}
