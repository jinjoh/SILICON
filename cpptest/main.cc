#include "QuadTree.h"
#include "Shape.h"

int main(void) {

  const BoundingBox bbox(0, 0, 100, 100);
  QuadTree<Rectangle> qt(bbox, 200);


  Rectangle r(5,5, 10, 10);
  qt.insert(r);
}
