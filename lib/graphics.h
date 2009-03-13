#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include <stdint.h>
#include "memory_map.h"

#define BYTES_PER_PIXEL 4

#define IS_LITTLE_ENDIAN 1
//#define IS_BIG_ENDIAN 1

#ifdef IS_LITTLE_ENDIAN
#define MASK_R(r) (r & 0xff)
#define MASK_G(g) ((g >> 8) & 0xff)
#define MASK_B(b) ((b >> 16) & 0xff)
#define MASK_A(a) (a >> 24)
#define MERGE_CHANNELS(r,g,b,a) ((a << 24) | (b <<16) | (g << 8) | r)
#endif

#ifdef IS_BIG_ENDIAN
#define MASK_R(r) (r >> 24)
#define MASK_G(g) ((g >> 16) & 0xff)
#define MASK_B(b) ((b >> 8) & 0xff)
#define MASK_A(a) (a & 0xff)
#define MERGE_CHANNELS(r,g,b,a) ((r << 24) | (g <<16) | (b << 8) | a)
#endif

// 75 * 255 + 147 * 255 + 35 * 255 = 65535
#define RGBA_TO_GS(pix_ptr) (((75 * MASK_R(*pix_ptr)) + (147 * MASK_G(*pix_ptr)) + (35 * MASK_B(*pix_ptr))) >> 8)

enum IMAGE_TYPE {
  IMAGE_TYPE_GS = 0,
  IMAGE_TYPE_RGBA = 1,
  IMAGE_TYPE_RGB = 2
};


struct image {
  
  unsigned int width, height;
  memory_map_t * map;
  IMAGE_TYPE image_type;
};

typedef struct image image_t;

image_t * gr_create_image(unsigned int width, unsigned int height, IMAGE_TYPE image_type);
image_t * gr_create_memory_image(unsigned int width, unsigned int height, IMAGE_TYPE image_type);

image_t * gr_extract_image(image_t * img, 
			   unsigned int min_x, unsigned int min_y, unsigned int width, unsigned int height);
image_t * gr_extract_image_as_gs(image_t * img,
				 unsigned int min_x, unsigned int min_y, unsigned int width, unsigned int height);

ret_t gr_alloc_memory(image_t * img);

ret_t gr_image_destroy(image_t * img);

ret_t gr_map_clear(image_t * img);

ret_t gr_map_temp_file(image_t * img, const char * const project_dir);
ret_t gr_map_file(image_t * img, const char * const project_dir, const char * const filename);
ret_t gr_map_file_by_fd(image_t * img, const char * const project_dir, int fd, const char * const filename);

ret_t gr_copy_image(image_t * dst_img, image_t * src_img, 
		    unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y);

ret_t gr_clone_image_data(image_t * dst_img, image_t * src_img);

ret_t gr_import_background_image(image_t * img, 
				 unsigned int offs_x, unsigned int offs_y,
				 const char * const filename);




// get/set pixels
void gr_set_pixval(image_t * img, unsigned int x, unsigned int y, uint32_t pix);
uint32_t gr_get_pixval(image_t * img, unsigned int x, unsigned int y);

uint8_t gr_get_greyscale_pixval(image_t * img, unsigned int x, unsigned int y);
void gr_set_greyscale_pixval(image_t * img, unsigned int x, unsigned int y, uint8_t gs_val);

void gr_copy_pixel(image_t * dst_img, unsigned int dst_x, unsigned int dst_y,
		   image_t * src_img, unsigned int src_x, unsigned int src_y);

void gr_copy_pixel_rgba(image_t * dst_img, unsigned int dst_x, unsigned int dst_y,
			image_t * src_img, unsigned int src_x, unsigned int src_y);

ret_t gr_scale_and_shift_in_place(image_t *img, 
				  double scaling_x, double scaling_y, 
				  unsigned int shift_x, unsigned int shift_y);

ret_t gr_flip_left_right(image_t * image);
ret_t gr_flip_up_down(image_t * image);

#endif
