
#include "QuadTree.h"


#include <list>

#include "QuadTreeTest.h"
#include "globals.h"
#include <stdlib.h>

CPPUNIT_TEST_SUITE_REGISTRATION (QuadTreeTest);

void QuadTreeTest::setUp(void) {

  const BoundingBox bbox(0, 100, 0, 100);
  qt_p = new QuadTree<Rectangle *>(bbox, 4);
  CPPUNIT_ASSERT(qt_p != NULL);
  CPPUNIT_ASSERT(qt_p->is_leave() == true);

  r1 = new Rectangle(5,5, 10, 10);
  CPPUNIT_ASSERT(r1 != NULL);

  r2 = new Rectangle(20,20, 30, 30);
  CPPUNIT_ASSERT(r2 != NULL);

}

void QuadTreeTest::tearDown(void) {
  delete qt_p;


  delete r1;
  delete r2;

  qt_p = NULL;
  r1 = NULL;
  r2 = NULL;
}

void QuadTreeTest::test_insert_pointer(void) {
  
  CPPUNIT_ASSERT(RET_IS_OK(qt_p->insert(r1)));
  CPPUNIT_ASSERT(qt_p->total_size() == 1);

  CPPUNIT_ASSERT(RET_IS_OK(qt_p->insert(r2)));
  CPPUNIT_ASSERT(qt_p->total_size() == 2);

}

void QuadTreeTest::test_iterator_pointer(void) {

  CPPUNIT_ASSERT(qt_p != NULL);

  test_insert_pointer();

  unsigned int i = 0;

  //std::cout << std::endl;
  for(QuadTree<Rectangle *>::iterator it = qt_p->begin();
      it != qt_p->end();
      ++it, i++) {
    //std::cout << "node " << i << std::endl;
    CPPUNIT_ASSERT (it != qt_p->end());
    CPPUNIT_ASSERT(*it != NULL);
    CPPUNIT_ASSERT ((*it == r1) || (*it == r2));
  }
  //std::cout << std::endl;
  
  CPPUNIT_ASSERT(qt_p->total_size() == i);
  CPPUNIT_ASSERT(i == 2);
}


void QuadTreeTest::test_iterator_compare(void) {
    CPPUNIT_ASSERT(qt_p != NULL);
    test_insert_pointer();
    QuadTree<Rectangle *>::iterator it1 = qt_p->begin();
    QuadTree<Rectangle *>::iterator it2 = qt_p->begin();


    CPPUNIT_ASSERT(it1 == it2);
    ++it1;
    CPPUNIT_ASSERT(it1 != it2);
    ++it2;

    CPPUNIT_ASSERT(it1 == it2);

    QuadTree<Rectangle *>::iterator it1_end = qt_p->end();
    QuadTree<Rectangle *>::iterator it2_end = qt_p->end();

    CPPUNIT_ASSERT(it1_end == it2_end);

    ++it1;
    CPPUNIT_ASSERT(it1 != it2);
    ++it1;
    CPPUNIT_ASSERT(it1 == it1_end);

    ++it2;
    ++it2;
    CPPUNIT_ASSERT(it2 == it2_end);

}

void QuadTreeTest::test_split(void) {
    CPPUNIT_ASSERT(qt_p != NULL);

    CPPUNIT_ASSERT(qt_p->total_size() == 0);
    test_insert_pointer();

    CPPUNIT_ASSERT(RET_IS_OK(qt_p->insert(r1)));
    CPPUNIT_ASSERT(RET_IS_OK(qt_p->insert(r1)));
    CPPUNIT_ASSERT(qt_p->total_size() == 4);
    CPPUNIT_ASSERT(qt_p->depth() == 1);

    CPPUNIT_ASSERT(RET_IS_OK(qt_p->insert(r1)));
    CPPUNIT_ASSERT(qt_p->total_size() == 5);
    CPPUNIT_ASSERT(qt_p->depth() == 2);

}

void QuadTreeTest::test_remove(void) {
    const unsigned int max_elements = 10000;

    std::list<Rectangle *> r_list;
    for(unsigned int i = 0; i < max_elements; i++) {
        unsigned int min_x = 1+(int) (90.0*rand()/(RAND_MAX+1.0));
        unsigned int min_y = 1+(int) (90.0*rand()/(RAND_MAX+1.0));
        unsigned int w = 1+(int) (10.0*rand()/(RAND_MAX+1.0));
        unsigned int h = 1+(int) (10.0*rand()/(RAND_MAX+1.0));
        Rectangle * r = new Rectangle(min_x, min_x + w, min_y, min_y + h);
        CPPUNIT_ASSERT(r != NULL);
        r_list.push_back(r);

        CPPUNIT_ASSERT(RET_IS_OK(qt_p->insert(r)));
    }

    CPPUNIT_ASSERT(qt_p->total_size() == max_elements);
    CPPUNIT_ASSERT(qt_p->depth() >= 2);

    for(std::list<Rectangle *>::iterator it = r_list.begin(); it != r_list.end(); ++it) {
        CPPUNIT_ASSERT(RET_IS_OK(qt_p->remove(*it)));
        delete *it;
    }

    CPPUNIT_ASSERT(qt_p->total_size() == 0);
    CPPUNIT_ASSERT(qt_p->depth() == 1);
}

