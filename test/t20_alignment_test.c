#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <renderer.h>
#include <project.h>
#include <alignment_marker.h>
#include <globals.h>

#define DEBUG

#define EPSILON 2
#define CHECK_DIST_OK(a, b) (fabs((double)a - (double)b)  < EPSILON)

void test01(void) {
  double scaling_x[2], scaling_y[2];
  int shift_x[2], shift_y[2];
  alignment_marker_set_t *amset = amset_create(2);
  assert(amset != NULL);

  assert(amset_add_marker(amset, 0, MARKER_TYPE_M1_UP,   242, 249) == 1);
  assert(amset_add_marker(amset, 0, MARKER_TYPE_M2_UP,   489, 489) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M1_DOWN, 198, 200) == 1);
  
  assert(amset_complete(amset) == 0);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M2_DOWN, 509, 658) == 1);
  assert(amset_complete(amset) == 1);

  amset_print(amset);

  assert(amset_calc_transformation(amset, scaling_x, scaling_y, shift_x, shift_y) == 1);
  
  for(int i = 0; i < 2; i++)
    printf("layer=%d sx=%f sy=%f dx=%d dy=%d\n", i, scaling_x[i], scaling_y[i], shift_x[i], shift_y[i]);

  assert(amset_apply_transformation_to_markers(amset, scaling_x, scaling_y, shift_x, shift_y) == 1);

  amset_print(amset);

  alignment_marker_t * m1_up = amset_get_marker(amset, 0, MARKER_TYPE_M1_UP);
  alignment_marker_t * m2_up = amset_get_marker(amset, 0, MARKER_TYPE_M2_UP);
  alignment_marker_t * m1_down = amset_get_marker(amset, 1, MARKER_TYPE_M1_DOWN);
  alignment_marker_t * m2_down = amset_get_marker(amset, 1, MARKER_TYPE_M2_DOWN);
  assert(m1_up != NULL);
  assert(m2_up != NULL);
  assert(m1_down != NULL);
  assert(m2_down != NULL);

  assert(CHECK_DIST_OK(m1_up->x, m1_down->x));
  assert(CHECK_DIST_OK(m1_up->y, m1_down->y));
  assert(CHECK_DIST_OK(m2_up->x, m2_down->x));
  assert(CHECK_DIST_OK(m2_up->y, m2_down->y));
  amset_destroy(amset);
}

void test02(void) {
  double scaling_x[3], scaling_y[3];
  int shift_x[3], shift_y[3];
  alignment_marker_set_t *amset = amset_create(3);
  assert(amset != NULL);

  assert(amset_add_marker(amset, 0, MARKER_TYPE_M1_UP,   242, 248) == 1);
  assert(amset_add_marker(amset, 0, MARKER_TYPE_M2_UP,   491, 488) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M1_DOWN, 197, 199) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M2_DOWN, 512, 657) == 1);

  assert(amset_add_marker(amset, 1, MARKER_TYPE_M1_UP,   196, 658) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M2_UP,   511, 200) == 1);
  assert(amset_add_marker(amset, 2, MARKER_TYPE_M1_DOWN, 74, 617) == 1);
  assert(amset_add_marker(amset, 2, MARKER_TYPE_M2_DOWN, 289, 68) == 1);

  assert(amset_complete(amset) == 1);

  amset_print(amset);

  assert(amset_calc_transformation(amset, scaling_x, scaling_y, shift_x, shift_y) == 1);
  
  for(int i = 0; i < 3; i++)
    printf("layer=%d sx=%f sy=%f dx=%d dy=%d\n", i, scaling_x[i], scaling_y[i], shift_x[i], shift_y[i]);

  assert(amset_apply_transformation_to_markers(amset, scaling_x, scaling_y, shift_x, shift_y) == 1);

  amset_print(amset);

  alignment_marker_t * l0_m1_up = amset_get_marker(amset, 0, MARKER_TYPE_M1_UP);
  alignment_marker_t * l0_m2_up = amset_get_marker(amset, 0, MARKER_TYPE_M2_UP);
  alignment_marker_t * l1_m1_down = amset_get_marker(amset, 1, MARKER_TYPE_M1_DOWN);
  alignment_marker_t * l1_m2_down = amset_get_marker(amset, 1, MARKER_TYPE_M2_DOWN);
  alignment_marker_t * l1_m1_up = amset_get_marker(amset, 1, MARKER_TYPE_M1_UP);
  alignment_marker_t * l1_m2_up = amset_get_marker(amset, 1, MARKER_TYPE_M2_UP);
  alignment_marker_t * l2_m1_down = amset_get_marker(amset, 2, MARKER_TYPE_M1_DOWN);
  alignment_marker_t * l2_m2_down = amset_get_marker(amset, 2, MARKER_TYPE_M2_DOWN);

  assert(l0_m1_up != NULL);
  assert(l0_m2_up != NULL);
  assert(l1_m1_down != NULL);
  assert(l1_m2_down != NULL);
  assert(l1_m1_up != NULL);
  assert(l1_m2_up != NULL);
  assert(l2_m1_down != NULL);
  assert(l2_m2_down != NULL);

  assert(CHECK_DIST_OK(l0_m1_up->x, l1_m1_down->x));
  assert(CHECK_DIST_OK(l0_m1_up->y, l1_m1_down->y));
  assert(CHECK_DIST_OK(l0_m2_up->x, l1_m2_down->x));
  assert(CHECK_DIST_OK(l0_m2_up->y, l1_m2_down->y));


  assert(CHECK_DIST_OK(l1_m1_up->x, l2_m1_down->x));
  assert(CHECK_DIST_OK(l1_m1_up->y, l2_m1_down->y));
  assert(CHECK_DIST_OK(l1_m2_up->x, l2_m2_down->x));
  assert(CHECK_DIST_OK(l1_m2_up->y, l2_m2_down->y));

  amset_destroy(amset);

}

void test03(void) {
  double scaling_x[3], scaling_y[3];
  int shift_x[3], shift_y[3];
  alignment_marker_set_t *amset = amset_create(3);
  assert(amset != NULL);

  assert(amset_add_marker(amset, 0, MARKER_TYPE_M1_UP,   100, 100) == 1);
  assert(amset_add_marker(amset, 0, MARKER_TYPE_M2_UP,   200, 200) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M1_DOWN, 100, 100) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M2_DOWN, 200, 200) == 1);

  assert(amset_add_marker(amset, 1, MARKER_TYPE_M1_UP,   100, 100) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M2_UP,   200, 200) == 1);
  assert(amset_add_marker(amset, 2, MARKER_TYPE_M1_DOWN, 300, 300) == 1);
  assert(amset_add_marker(amset, 2, MARKER_TYPE_M2_DOWN, 500, 500) == 1);

  assert(amset_complete(amset) == 1);
  amset_print(amset);

  assert(amset_calc_transformation(amset, scaling_x, scaling_y, shift_x, shift_y) == 1);
  
  for(int i = 0; i < 3; i++)
    printf("layer=%d sx=%f sy=%f dx=%d dy=%d\n", i, scaling_x[i], scaling_y[i], shift_x[i], shift_y[i]);

  assert(amset_apply_transformation_to_markers(amset, scaling_x, scaling_y, shift_x, shift_y) == 1);

  amset_print(amset);

  alignment_marker_t * l0_m1_up = amset_get_marker(amset, 0, MARKER_TYPE_M1_UP);
  alignment_marker_t * l0_m2_up = amset_get_marker(amset, 0, MARKER_TYPE_M2_UP);
  alignment_marker_t * l1_m1_down = amset_get_marker(amset, 1, MARKER_TYPE_M1_DOWN);
  alignment_marker_t * l1_m2_down = amset_get_marker(amset, 1, MARKER_TYPE_M2_DOWN);
  alignment_marker_t * l1_m1_up = amset_get_marker(amset, 1, MARKER_TYPE_M1_UP);
  alignment_marker_t * l1_m2_up = amset_get_marker(amset, 1, MARKER_TYPE_M2_UP);
  alignment_marker_t * l2_m1_down = amset_get_marker(amset, 2, MARKER_TYPE_M1_DOWN);
  alignment_marker_t * l2_m2_down = amset_get_marker(amset, 2, MARKER_TYPE_M2_DOWN);

  assert(l0_m1_up != NULL);
  assert(l0_m2_up != NULL);
  assert(l1_m1_down != NULL);
  assert(l1_m2_down != NULL);
  assert(l1_m1_up != NULL);
  assert(l1_m2_up != NULL);
  assert(l2_m1_down != NULL);
  assert(l2_m2_down != NULL);

  assert(CHECK_DIST_OK(l0_m1_up->x, l1_m1_down->x));
  assert(CHECK_DIST_OK(l0_m1_up->y, l1_m1_down->y));
  assert(CHECK_DIST_OK(l0_m2_up->x, l1_m2_down->x));
  assert(CHECK_DIST_OK(l0_m2_up->y, l1_m2_down->y));


  assert(CHECK_DIST_OK(l1_m1_up->x, l2_m1_down->x));
  assert(CHECK_DIST_OK(l1_m1_up->y, l2_m1_down->y));
  assert(CHECK_DIST_OK(l1_m2_up->x, l2_m2_down->x));
  assert(CHECK_DIST_OK(l1_m2_up->y, l2_m2_down->y));

  amset_destroy(amset);

}


void test04(void) {
  double scaling_x[3], scaling_y[3];
  int shift_x[3], shift_y[3];
  alignment_marker_set_t *amset = amset_create(3);
  assert(amset != NULL);

  assert(amset_add_marker(amset, 0, MARKER_TYPE_M1_UP,   100, 100) == 1);
  assert(amset_add_marker(amset, 0, MARKER_TYPE_M2_UP,   200, 200) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M1_DOWN, 100, 100) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M2_DOWN, 200, 200) == 1);

  assert(amset_add_marker(amset, 1, MARKER_TYPE_M1_UP,   200, 100) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M2_UP,   100, 200) == 1);
  assert(amset_add_marker(amset, 2, MARKER_TYPE_M1_DOWN, 500, 300) == 1);
  assert(amset_add_marker(amset, 2, MARKER_TYPE_M2_DOWN, 300, 500) == 1);

  assert(amset_complete(amset) == 1);
  amset_print(amset);

  assert(amset_calc_transformation(amset, scaling_x, scaling_y, shift_x, shift_y) == 1);
  
  for(int i = 0; i < 3; i++)
    printf("layer=%d sx=%f sy=%f dx=%d dy=%d\n", i, scaling_x[i], scaling_y[i], shift_x[i], shift_y[i]);

  assert(amset_apply_transformation_to_markers(amset, scaling_x, scaling_y, shift_x, shift_y) == 1);

  amset_print(amset);

  alignment_marker_t * l0_m1_up = amset_get_marker(amset, 0, MARKER_TYPE_M1_UP);
  alignment_marker_t * l0_m2_up = amset_get_marker(amset, 0, MARKER_TYPE_M2_UP);
  alignment_marker_t * l1_m1_down = amset_get_marker(amset, 1, MARKER_TYPE_M1_DOWN);
  alignment_marker_t * l1_m2_down = amset_get_marker(amset, 1, MARKER_TYPE_M2_DOWN);
  alignment_marker_t * l1_m1_up = amset_get_marker(amset, 1, MARKER_TYPE_M1_UP);
  alignment_marker_t * l1_m2_up = amset_get_marker(amset, 1, MARKER_TYPE_M2_UP);
  alignment_marker_t * l2_m1_down = amset_get_marker(amset, 2, MARKER_TYPE_M1_DOWN);
  alignment_marker_t * l2_m2_down = amset_get_marker(amset, 2, MARKER_TYPE_M2_DOWN);

  assert(l0_m1_up != NULL);
  assert(l0_m2_up != NULL);
  assert(l1_m1_down != NULL);
  assert(l1_m2_down != NULL);
  assert(l1_m1_up != NULL);
  assert(l1_m2_up != NULL);
  assert(l2_m1_down != NULL);
  assert(l2_m2_down != NULL);

  assert(CHECK_DIST_OK(l0_m1_up->x, l1_m1_down->x));
  assert(CHECK_DIST_OK(l0_m1_up->y, l1_m1_down->y));
  assert(CHECK_DIST_OK(l0_m2_up->x, l1_m2_down->x));
  assert(CHECK_DIST_OK(l0_m2_up->y, l1_m2_down->y));


  assert(CHECK_DIST_OK(l1_m1_up->x, l2_m1_down->x));
  assert(CHECK_DIST_OK(l1_m1_up->y, l2_m1_down->y));
  assert(CHECK_DIST_OK(l1_m2_up->x, l2_m2_down->x));
  assert(CHECK_DIST_OK(l1_m2_up->y, l2_m2_down->y));

  amset_destroy(amset);

}


void test05(void) {
  double scaling_x[3], scaling_y[3];
  int shift_x[3], shift_y[3];
  alignment_marker_set_t *amset = amset_create(3);
  assert(amset != NULL);

  assert(amset_add_marker(amset, 0, MARKER_TYPE_M1_UP,     61,   63) == 1);
  assert(amset_add_marker(amset, 0, MARKER_TYPE_M2_UP,   1070, 4378) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M1_DOWN,  326,  465) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M2_DOWN, 1309, 4778) == 1);

  assert(amset_add_marker(amset, 1, MARKER_TYPE_M1_UP,    310,  449) == 1);
  assert(amset_add_marker(amset, 1, MARKER_TYPE_M2_UP,   1148, 4786) == 1);
  assert(amset_add_marker(amset, 2, MARKER_TYPE_M1_DOWN,  187,  313) == 1);
  assert(amset_add_marker(amset, 2, MARKER_TYPE_M2_DOWN, 1052, 4584) == 1);

  assert(amset_complete(amset) == 1);
  amset_print(amset);

  assert(amset_calc_transformation(amset, scaling_x, scaling_y, shift_x, shift_y) == 1);
  
  for(int i = 0; i < 3; i++)
    printf("layer=%d sx=%f sy=%f dx=%d dy=%d\n", i, scaling_x[i], scaling_y[i], shift_x[i], shift_y[i]);

  assert(amset_apply_transformation_to_markers(amset, scaling_x, scaling_y, shift_x, shift_y) == 1);

  amset_print(amset);

  alignment_marker_t * l0_m1_up = amset_get_marker(amset, 0, MARKER_TYPE_M1_UP);
  alignment_marker_t * l0_m2_up = amset_get_marker(amset, 0, MARKER_TYPE_M2_UP);
  alignment_marker_t * l1_m1_down = amset_get_marker(amset, 1, MARKER_TYPE_M1_DOWN);
  alignment_marker_t * l1_m2_down = amset_get_marker(amset, 1, MARKER_TYPE_M2_DOWN);
  alignment_marker_t * l1_m1_up = amset_get_marker(amset, 1, MARKER_TYPE_M1_UP);
  alignment_marker_t * l1_m2_up = amset_get_marker(amset, 1, MARKER_TYPE_M2_UP);
  alignment_marker_t * l2_m1_down = amset_get_marker(amset, 2, MARKER_TYPE_M1_DOWN);
  alignment_marker_t * l2_m2_down = amset_get_marker(amset, 2, MARKER_TYPE_M2_DOWN);

  assert(l0_m1_up != NULL);
  assert(l0_m2_up != NULL);
  assert(l1_m1_down != NULL);
  assert(l1_m2_down != NULL);
  assert(l1_m1_up != NULL);
  assert(l1_m2_up != NULL);
  assert(l2_m1_down != NULL);
  assert(l2_m2_down != NULL);

  assert(CHECK_DIST_OK(l0_m1_up->x, l1_m1_down->x));
  assert(CHECK_DIST_OK(l0_m1_up->y, l1_m1_down->y));
  assert(CHECK_DIST_OK(l0_m2_up->x, l1_m2_down->x));
  assert(CHECK_DIST_OK(l0_m2_up->y, l1_m2_down->y));


  assert(CHECK_DIST_OK(l1_m1_up->x, l2_m1_down->x));
  assert(CHECK_DIST_OK(l1_m1_up->y, l2_m1_down->y));
  assert(CHECK_DIST_OK(l1_m2_up->x, l2_m2_down->x));
  assert(CHECK_DIST_OK(l1_m2_up->y, l2_m2_down->y));

  amset_destroy(amset);

}

int main(void) {

  
  test01();
  test03();
  test04();
  test02();
  
  test05();
  return 0;
}
