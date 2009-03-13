#include "globals.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <wand/magick-wand.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

#include "graphics.h"
#include "memory_map.h"

#ifdef HAVE_MMAP64
#define MMAP mmap64
#else
#define MMAP mmap
#endif

#define TM "graphics.c"

#define CHECK_XY_IN_IMG(img, x, y) (img != NULL && x < img->width && y < img->height)

#define ThrowWandException(wand) { \
  char *description; \
  ExceptionType severity; \
  description=MagickGetException(wand,&severity); \
  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description); \
  description=(char *) MagickRelinquishMemory(description); \
  exit(-1); \
}

/**
 * Creates an empty image struct. It does not allocate memory for the image
 * data. Use gr_alloc_memory() or gr_map_file() to do it.
 * @param width width of image
 * @param height height of image
 * @returns pointer to structure or NULL on failure
 */
image_t * gr_create_image(unsigned int width, unsigned int height, IMAGE_TYPE image_type) {
  image_t * ptr;

  int bytes_per_pixel;
  if((ptr = (image_t *)malloc(sizeof(struct image))) == NULL) return NULL;
  memset(ptr, 0, sizeof(image_t));
	
  switch(image_type) {
  case IMAGE_TYPE_GS: bytes_per_pixel = 1; break;
  case IMAGE_TYPE_RGBA: bytes_per_pixel = 4; break;
  case IMAGE_TYPE_RGB: bytes_per_pixel = 3; break;
  default:
    bytes_per_pixel = 4;
  }

  if((ptr->map = mm_create(width, height, bytes_per_pixel)) == NULL) {
    free(ptr);
    return NULL;
  }
  ptr->width = width;
  ptr->height = height;
  ptr->image_type = image_type;
  
  return ptr;
}

image_t * gr_create_memory_image(unsigned int width, unsigned int height, IMAGE_TYPE image_type) {
  image_t * img = gr_create_image(width, height, image_type);
  if(img != NULL) {
    if(!RET_IS_OK(gr_alloc_memory(img))) {
      ret_t ret = gr_image_destroy(img);
      assert(ret == RET_OK);
      return NULL;
    }
  }
  return img;
}

/**
 * Allocate (real) memory for an image.
 * @param img the image for that you want to allocate memory
 */
ret_t gr_alloc_memory(image_t * img) {

  assert(img != NULL);
  if(img == NULL) return RET_INV_PTR;
  return mm_alloc_memory(img->map);
}

/**
 * Destroy an image.
 */
ret_t gr_image_destroy(image_t * img) {
  if(!img) return RET_INV_PTR;
  
  ret_t ret = mm_destroy(img->map);

  free(img);
  return ret;
}

/** Extract subimage of size width x height form image img. Interval boundaries are
 * included. It allocates memory the new image.
 * @returns pointer to new image.
*/
image_t * gr_extract_image(image_t * img,
			   unsigned int min_x, unsigned int min_y, unsigned int width, unsigned int height) {

  assert(img != NULL);
  if(!img) return NULL;

  image_t * extracted_img = gr_create_image(width, height, img->image_type);
  if(!extracted_img) {
    debug(TM, "gr_extract_image(): gr_create_image() failed");
    return NULL;
  }

  if(!RET_IS_OK(gr_alloc_memory(extracted_img))) {
    debug(TM, "gr_extract_image(): gr_alloc_memory() failed");
    gr_image_destroy(extracted_img);
    return NULL;
  }

  if(!RET_IS_OK(gr_copy_image(extracted_img, img, min_x, min_y, min_x + width, min_y + height))) {
    debug(TM, "gr_extract_image(): gr_copy_image() failed");
    gr_image_destroy(extracted_img);
    return NULL;
  }

  return extracted_img;
}

/** Extract subimage of size width x height form image img. Interval boundaries are
 * included. It allocates memory the new image. The new image is in grayscale mode.
 * @returns pointer to new image.
*/
image_t * gr_extract_image_as_gs(image_t * img,
				 unsigned int min_x, unsigned int min_y, unsigned int width, unsigned int height) {

  assert(img != NULL);
  if(!img) return NULL;

  image_t * extracted_img = gr_create_image(width, height, IMAGE_TYPE_GS);
  if(!extracted_img) {
    debug(TM, "gr_extract_image(): gr_create_image() failed");
    return NULL;
  }

  if(!RET_IS_OK(gr_alloc_memory(extracted_img))) {
    debug(TM, "gr_extract_image(): gr_alloc_memory() failed");
    gr_image_destroy(extracted_img);
    return NULL;
  }

  if(!RET_IS_OK(gr_copy_image(extracted_img, img, min_x, min_y, min_x + width, min_y + height))) {
    debug(TM, "gr_extract_image(): gr_copy_image() failed");
    gr_image_destroy(extracted_img);
    return NULL;
  }

  return extracted_img;
}

/**
 * Clear image data.
 * @returns RET_OK on success
 */
ret_t gr_map_clear(image_t * img) {
  if(!img || !img->map) return RET_INV_PTR;
  return mm_clear(img->map);
}


/**
 * Create a temp file and use it as storage for the image data
 */
ret_t gr_map_temp_file(image_t * img, const char * const project_dir) {
  
  if(!img) return RET_INV_PTR;
  return mm_map_temp_file(img->map, project_dir);
}

/**
 * Use storage in file as storage for image data
 */
ret_t gr_map_file(image_t * img, const char * const project_dir, const char * const filename) {
  if(!img) return RET_INV_PTR;
  return mm_map_file(img->map, project_dir, filename);
}

/**
 * Use storage in opend file as storage for image data
 */
ret_t gr_map_file_by_fd(image_t * img, const char * const project_dir, int fd, const char * const filename) {
	
  if(!img) return RET_INV_PTR;
  return mm_map_file_by_fd(img->map, project_dir, fd, filename);
}


/**
 * Copies a rectangular region (min_x, min_y, max_x, max_y) from the source image into destination image.
 * If the rectangle is larger than the source image, the rectangle is broken down to an intersection.
 * If images differ in image type, implicit conversion between RGBA and GS data happens.
 * @params dst_img the destination image
 * @params src_img the source image
 * @params min_x (x position included)
 * @params min_y (y position included)
 * @params max_x (x position included)
 * @params max_y (y position included)
 * @returns RET_OK on success
*/

ret_t gr_copy_image(image_t * dst_img, image_t * src_img, 
		  unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y) {

  assert(max_x > min_x || max_y > min_y);
  assert(dst_img || src_img);
  assert(dst_img->map);
  assert(src_img->map);

  if(max_x <= min_x || max_y <= min_y) return RET_ERR;
  if(!dst_img || !dst_img->map || !src_img || !src_img->map) return RET_INV_PTR;

  // real width and height for copy
  unsigned int width = MIN(MIN(max_x, src_img->width) - min_x, dst_img->width);
  unsigned int height = MIN(MIN(max_y, src_img->height) - min_y, dst_img->height);
  unsigned int dst_x, dst_y;

  for(dst_y = 0; dst_y < height; dst_y++)
    for(dst_x = 0; dst_x < width; dst_x++)
      gr_copy_pixel(dst_img, dst_x, dst_y, src_img, min_x + dst_x, min_y + dst_y);
      

  return RET_OK;

}

ret_t gr_clone_image_data(image_t * dst_img, image_t * src_img) {
  if(!dst_img || !src_img) return RET_INV_PTR;

  if(dst_img->width == src_img->width &&
     dst_img->height == src_img->height &&
     dst_img->image_type == src_img->image_type) {
    return mm_clone_map_data(dst_img->map, src_img->map);
  }
  else 
    return gr_copy_image(dst_img, src_img, 0, 0, src_img->width - 1, src_img->height - 1);
}

void gr_copy_pixel_rgba(image_t * dst_img, unsigned int dst_x, unsigned int dst_y,
			image_t * src_img, unsigned int src_x, unsigned int src_y) {

  *(uint32_t *)mm_get_ptr(dst_img->map, dst_x, dst_y) = 
    *(uint32_t *)mm_get_ptr(src_img->map, src_x, src_y);

}

/** If images differ in image type, implicit conversion between RGBA and GS data happens. */
void gr_copy_pixel(image_t * dst_img, unsigned int dst_x, unsigned int dst_y,
		   image_t * src_img, unsigned int src_x, unsigned int src_y) {

  assert(dst_img != NULL);
  assert(src_img != NULL);

  if(src_img->image_type == IMAGE_TYPE_GS &&
     dst_img->image_type == IMAGE_TYPE_GS) {

    uint8_t * dst_ptr = (uint8_t *)mm_get_ptr(dst_img->map, dst_x, dst_y);
    uint8_t * src_ptr = (uint8_t *)mm_get_ptr(src_img->map, src_x, src_y);

    *dst_ptr = *src_ptr;
  }
  else if(src_img->image_type == IMAGE_TYPE_RGBA &&
	  dst_img->image_type == IMAGE_TYPE_RGBA) {

    uint32_t * dst_ptr = (uint32_t *)mm_get_ptr(dst_img->map, dst_x, dst_y);
    uint32_t * src_ptr = (uint32_t *)mm_get_ptr(src_img->map, src_x, src_y);

    *dst_ptr = *src_ptr;
  }
  else if(src_img->image_type == IMAGE_TYPE_GS &&
	  dst_img->image_type == IMAGE_TYPE_RGBA) {

    uint32_t * dst_ptr = (uint32_t *)mm_get_ptr(dst_img->map, dst_x, dst_y);
    uint8_t * src_ptr = (uint8_t *)mm_get_ptr(src_img->map, src_x, src_y);

    *dst_ptr = MERGE_CHANNELS(*src_ptr, *src_ptr, *src_ptr, 0xff);
  }
  else if(src_img->image_type == IMAGE_TYPE_RGBA &&
	  dst_img->image_type == IMAGE_TYPE_GS) {

    uint8_t * dst_ptr = (uint8_t *)mm_get_ptr(dst_img->map, dst_x, dst_y);
    uint32_t * src_ptr = (uint32_t *)mm_get_ptr(src_img->map, src_x, src_y);

    *dst_ptr = RGBA_TO_GS(src_ptr);
  }

}
		   


/* imports a graphics file, decompress it and store data
   in a new file */


ret_t gr_import_background_image(image_t * img, 
			       unsigned int offs_x, unsigned int offs_y,
			       const char * const filename) {
  MagickWand *magick_wand;
  MagickBooleanType status;
  unsigned int src_x, src_y;
	
  assert(img->map != NULL);
  if(!img || !img->map) return RET_INV_PTR;
  MagickWandGenesis();
	
  magick_wand = NewMagickWand();  
  status = MagickReadImage(magick_wand, filename);
  if(status == MagickFalse) ThrowWandException(magick_wand);
	
  unsigned int width = MagickGetImageWidth(magick_wand);
  unsigned int height = MagickGetImageHeight(magick_wand);
	
  uint32_t pixel;

  for(src_y = 0; src_y < MIN(height, img->height); src_y++) {
    for(src_x = 0; src_x < MIN(width, img->width); src_x++) {

      uint8_t * ptr = (uint8_t *)mm_get_ptr(img->map, src_x + offs_x, src_y + offs_y);
      MagickGetImagePixels(magick_wand, src_x, src_y, 1, 1, "RGBA", CharPixel, &pixel);

      switch(img->image_type) {
      case IMAGE_TYPE_GS:
	*ptr = RGBA_TO_GS(&pixel);
	break;
      case IMAGE_TYPE_RGBA:
	*(uint32_t *)ptr = pixel;
	break;
      default:
	puts("not implemented");
	exit(1);
      }
     
    }
  }
  
  magick_wand = DestroyMagickWand(magick_wand);
  //MagickWandTerminus();
  return RET_OK;
}


/**
 * Get a pixel value.
 * Checking for valid pointers and x/y-values is only done with an assert() and at runtime. If you
 * call this function with invalid params, then this function returns 0. But a return value of 0
 * does not indicate an error. So you should check for invalid 
 * @param img the image
 * @param x x-coordinate of the pixel
 * @param y y-coordinate of the pixel
 * @returns pixel value as 32-bit value. Use macros like MASK_R(), MASK_G(), MASK_B() to extract channels.
 */

uint32_t gr_get_pixval(image_t * img, unsigned int x, unsigned int y) {
#ifdef DEBUG_ASSERTS_IN_FCF
  assert(CHECK_XY_IN_IMG(img, x, y));
#endif
  return *((uint32_t *)mm_get_ptr(img->map, x, y));
}

void gr_set_pixval(image_t * img, unsigned int x, unsigned int y, uint32_t pix) {
#ifdef DEBUG_ASSERTS_IN_FCF
  assert(CHECK_XY_IN_IMG(img, x, y));
#endif
  *((uint32_t *)mm_get_ptr(img->map, x, y)) = pix;
}

/**
 * Works like the gr_get_pixval() function, but returns greyscaled pixel value.
 */
uint8_t gr_get_greyscale_pixval(image_t * img, unsigned int x, unsigned int y) {
  if(img->image_type == IMAGE_TYPE_RGBA) {
    uint32_t * ptr = (uint32_t *)mm_get_ptr(img->map, x, y);
    return RGBA_TO_GS(ptr);
  }
  else if(img->image_type == IMAGE_TYPE_GS) {
    return *(uint8_t *)mm_get_ptr(img->map, x, y);
  }
  else {
    puts("not implemented");
    exit(1);
  }
  return 0;
}

/**
 * set greyscale pixel value
 */
void gr_set_greyscale_pixval(image_t * img, unsigned int x, unsigned int y, uint8_t gs_val) {
  if(img->image_type == IMAGE_TYPE_RGBA) {
    uint32_t * ptr = (uint32_t *)mm_get_ptr(img->map, x, y);
    *ptr = MERGE_CHANNELS(gs_val, gs_val, gs_val, 0xff);
  }
  else if(img->image_type == IMAGE_TYPE_GS) {
    uint8_t * ptr = (uint8_t *)mm_get_ptr(img->map, x, y);
    *ptr = gs_val;
  }
  else {
    puts("not implemented");
    exit(1);
  }
}

/**
 * In place scaling and translation. It doesn't enlarge the image.
 */
ret_t gr_scale_and_shift_in_place(image_t *img, 
				double scaling_x, double scaling_y, 
				unsigned int shift_x, unsigned int shift_y) {

  return mm_scale_and_shift_in_place(img->map, scaling_x, scaling_y, shift_x, shift_y);
}


/** In-place flipping of RBGA an GS images. */
ret_t gr_flip_up_down(image_t * img) {
  assert(img);
  if(!img) return RET_ERR;
  
  unsigned int x, y;

  if(img->height == 1) return RET_OK;

  if(img->image_type == IMAGE_TYPE_RGBA) {

    for(y = 0; y < (img->height >> 1); y++) {
      for(x = 0; x < img->width; x++) {
	uint32_t * ptr1 = (uint32_t *)mm_get_ptr(img->map, x, y);
	uint32_t * ptr2 = (uint32_t *)mm_get_ptr(img->map, x, img->height -1 - y);
	uint32_t temp = *ptr1;
	*ptr1 = *ptr2;
	*ptr2 = temp;
      }
    }
  }
  else if(img->image_type == IMAGE_TYPE_GS) {
    for(y = 0; y < (img->height >> 1); y++) {
      for(x = 0; x < img->width; x++) {
	uint8_t * ptr1 = (uint8_t *)mm_get_ptr(img->map, x, y);
	uint8_t * ptr2 = (uint8_t *)mm_get_ptr(img->map, x, img->height -1 - y);
	uint8_t temp = *ptr1;
	*ptr1 = *ptr2;
	*ptr2 = temp;
      }
    }
  }
  else return RET_ERR;

  return RET_OK;
}

/** In-place flipping of RGBA and GS images. */
ret_t gr_flip_left_right(image_t * img) {
  assert(img);
  if(!img) return RET_ERR;
  
  unsigned int x, y;

  if(img->width == 1) return RET_OK;

  if(img->image_type == IMAGE_TYPE_RGBA) {

    for(y = 0; y < img->height; y++) {
      for(x = 0; x < (img->width >> 1); x++) {
	uint32_t * ptr1 = (uint32_t *)mm_get_ptr(img->map, x, y);
	uint32_t * ptr2 = (uint32_t *)mm_get_ptr(img->map, img->width - 1 - x, y);
	uint32_t temp = *ptr1;
	*ptr1 = *ptr2;
	*ptr2 = temp;
      }
    }
  }
  else if(img->image_type == IMAGE_TYPE_GS) {
    for(y = 0; y < img->height; y++) {
      for(x = 0; x < (img->width >> 1); x++) {
	uint8_t * ptr1 = (uint8_t *)mm_get_ptr(img->map, x, y);
	uint8_t * ptr2 = (uint8_t *)mm_get_ptr(img->map, img->width - 1 - x, y);
	uint8_t temp = *ptr1;
	*ptr1 = *ptr2;
	*ptr2 = temp;
      }
    }
  }
  else return RET_ERR;

  return RET_OK;
}
