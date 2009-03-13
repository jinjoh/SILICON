#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <renderer.h>
#include <project.h>
#include <img_algorithms.h>
#include <globals.h>

#define DEBUG


int img_sep_test_buf(uint8_t * buffer) {
  printf("%02X %02X %02X %02X\n", buffer[0], buffer[1], buffer[2], buffer[3]);
  assert(imgalgo_to_grayscale(buffer, 1, 1) == 1);
  printf("%02X %02X %02X %02X\n", buffer[0], buffer[1], buffer[2], buffer[3]);
  assert(imgalgo_separate_by_threshold(buffer, 1, 1, 120) == 1);
  printf("%02X %02X %02X %02X\n\n", buffer[0], buffer[1], buffer[2], buffer[3]);
  return 1;
}

int img_sep_test() {
  uint8_t buffer[4];

  buffer[0] = 0x64;
  buffer[1] = 0x88;
  buffer[2] = 0x70;
  buffer[3] = 0xff;

  img_sep_test_buf(buffer);
  buffer[1] = 0x10;
  img_sep_test_buf(buffer);

  return 1;
}

int main(void) {
  assert(img_sep_test() == 1);
  return 0;
}
