#ifndef __QUADTREETEST_H__
#define __QUADTREETEST_H__

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "QuadTree.h"

using namespace std;

class QuadTreeTest : public CPPUNIT_NS :: TestFixture {

  CPPUNIT_TEST_SUITE(QuadTreeTest);

  CPPUNIT_TEST (test_insert_pointer);
  CPPUNIT_TEST (test_iterator_compare);
  CPPUNIT_TEST (test_iterator_pointer);
  CPPUNIT_TEST (test_split);
  CPPUNIT_TEST (test_remove);
  CPPUNIT_TEST (test_region_iterator);

  CPPUNIT_TEST_SUITE_END ();

 public:
  void setUp (void);
  void tearDown (void);

 protected:
  void test_insert_pointer(void);
  void test_iterator_pointer(void);
  void test_iterator_compare(void);

  void test_split(void);
  void test_remove(void);


  void test_region_iterator(void);

 private:
  QuadTree<Rectangle *> * qt_p;

  Rectangle *r1, *r2;
};

#endif
