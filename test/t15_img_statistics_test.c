#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <graphics.h>
#include <renderer.h>
#include <project.h>
#include <img_algorithms.h>

#include <globals.h>

#define DEBUG

#define W 3
#define H 3

#define LESS_THAN_EPSILON(val) (fabs(val) < 0.01)

int main(void) {

  image_t * img = gr_create_image(W,H);
  assert(gr_alloc_memory(img) == 1);
  assert(img != NULL);
  int m3x3[] = {10, 12, 13, 14, 15, 16, 17, 18, 19};
  int x, y;
  for(x = 0; x < W; x++)
    for(y = 0; y < H; y++) {
      uint8_t p = m3x3[y*W +x];
      gr_set_pixval(img, x, y, MERGE_CHANNELS(p,p,p,p));
      printf("pixval(%d,%d)=%d\n", x, y, gr_get_greyscale_pixval(img, x,y) );
      //printf("%d, ..,\n", gr_get_greyscale_pixval(img, x,y) );
    }

  double t1 = calc_mean_for_img_area(img, 0, 0, W, H);
  double t2 = calc_variance_for_img_area(img, 0, 0, W, H, t1);
  double t3 = calc_standard_deviation_for_img_area(img, 0, 0, W, H, t1);

  printf("mean = %f, variance = %f, std deviation = %f\n", t1, t2, t3);

  assert(LESS_THAN_EPSILON(t1 - 14.888));
  assert(LESS_THAN_EPSILON(t2 - 7.654321));
  assert(LESS_THAN_EPSILON(t3 - 2.766644));

  assert(gr_image_destroy(img));
  return 0;
}
