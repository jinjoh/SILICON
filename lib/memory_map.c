#include "globals.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

#include "memory_map.h"

#ifdef HAVE_MMAP64
#define MMAP mmap64
#else
#define MMAP mmap
#endif

#define TM "memory_map.c"

#define CHECK_XY_IN_MAP(map, x, y) (map != NULL && map->mem != NULL && x < map->width && y < map->height)

/**
 * Creates an empty memory map struct. It does not allocate memory for the data.
 * Use mm_alloc_momory() or mm_map_file() to do it.
 * @param width width of map
 * @param height height of map
 * @returns pointer to structure or NULL on failure
 */
memory_map_t * mm_create(unsigned int width, unsigned int height, unsigned int bytes_per_elem) {
  memory_map_t * ptr;

  if((ptr = (memory_map_t *)malloc(sizeof(struct memory_map))) == NULL) return NULL;
  memset(ptr, 0, sizeof(memory_map_t));
	
  ptr->width = width;
  ptr->height = height;
  ptr->bytes_per_elem = bytes_per_elem;
  ptr->fd = -1;
  ptr->storage_type = MAP_STORAGE_TYPE_UNDEF;
  return ptr;
}

/**
 * Allocate memory for an image.
 * @param img the image for that you want to allocate memory
 */
ret_t mm_alloc_memory(memory_map_t * map) {

  assert(map != NULL);
  if(map == NULL) return RET_INV_PTR;

  assert(map->mem == NULL); // if it is not null, it would indicates, that there is already any allocation

  if(map->storage_type == MAP_STORAGE_TYPE_UNDEF) {
    map->mem = (uint8_t *) malloc(map->width * map->height * map->bytes_per_elem);
    if(!map->mem) return RET_MALLOC_FAILED;
    memset(map->mem, 0, map->width * map->height * map->bytes_per_elem);
    map->storage_type = MAP_STORAGE_TYPE_MEM;
  }
  assert(map->mem != NULL);
  return RET_OK;
}

/**
 * Destroy a memory map. If memory was mapped from file, the memory map will
 * be synced to disc and unmapped.
 */
ret_t mm_destroy(memory_map_t * map) {
  ret_t ret = RET_OK;
  assert(map);
  if(!map) return RET_INV_PTR;

  if(map->mem) {

    if(map->storage_type == MAP_STORAGE_TYPE_FILE) {
      if(msync(map->mem, map->width * map->height  * map->bytes_per_elem, MS_SYNC) == -1) {
	debug(TM, "msync() failed");
	ret = RET_ERR;
      }

      if(munmap(map->mem, map->width * map->height * map->bytes_per_elem) == -1) {
	debug(TM, "munmap failed");
	ret = RET_ERR;
      }

      map->mem = NULL;
    }
    else if(map->storage_type == MAP_STORAGE_TYPE_MEM) {
      free(map->mem);
      map->mem = NULL;
    }
  }
	
  if(map->fd > 0) close(map->fd);
	
  if(map->filename && map->is_temp_file) {
    if(unlink(map->filename) == -1) {
      debug(TM, "Can't unlink temp file");
      ret = RET_ERR;
    }
  }

  if(map->filename) free(map->filename);
	
  free(map);
  return ret;
}


/**
 * Clear map data.
 * @returns RET_OK on success
 */
ret_t mm_clear(memory_map_t * map) {
  if(!map) return RET_INV_PTR;
  memset(map->mem, 0, map->width * map->height * map->bytes_per_elem);
  return RET_OK;
}

/** 
 * Clear an area given by start point and width and height
 */

ret_t mm_clear_area(memory_map_t * map, unsigned int min_x, unsigned int min_y, 
		    unsigned int width, unsigned int height) {
  
  if(!map) return RET_INV_PTR;
  unsigned int x, y;
  for(y = min_y; y < min_y + height; y++)
    for(x = min_x; x < min_x + width; x++)
      memset(mm_get_ptr(map, x, y), 0, map->bytes_per_elem);

  return RET_OK;
}

/**
 * Create a temp file and use it as storage for the map data.
 */
ret_t mm_map_temp_file(memory_map_t * map, const char * const project_dir) {

  assert(map != NULL);
  assert(project_dir != NULL);
  if(map == NULL || project_dir == NULL) return RET_INV_PTR;  
  static const char template_str[] = "temp.XXXXXX";
  int fd;

  char filename[PATH_MAX];
  strcpy(filename, project_dir);
  strcat(filename, "/");
  strcat(filename, template_str);

  fd = mkstemp(filename);
  printf("temp file: [%s]\n", filename);
  if(fd == -1) {
    perror("mkstemp() failed");
    puts("mkstemp() failed");
    return RET_ERR;
  }

  map->filename = strdup(filename);
  map->is_temp_file = TRUE;
  return mm_map_file_by_fd(map, project_dir, fd, filename);
}

/**
 * Use storage in file as storage for memory map
 */
ret_t mm_map_file(memory_map_t * map, const char * const project_dir, const char * const filename) {
	
  if(!map) return RET_INV_PTR;
	
  // reset existing resources
  if(map->mem && (munmap(map->mem, map->width * map->height * map->bytes_per_elem) == -1)) {
    puts("munmap failed");
    return RET_ERR;
  }
  if(map->fd > 0) close(map->fd);
  if(map->filename) free(map->filename);
	
	
  // open file
  if((map->filename = (char *) malloc(strlen(filename) + strlen(project_dir) + 2)) == NULL) {
    return RET_MALLOC_FAILED;
  }
  strcpy(map->filename, project_dir);
  strcat(map->filename, "/");
  strcat(map->filename, filename);
	
	
  if((map->fd = open(map->filename, O_RDWR | O_CREAT, 0600)) == -1) {
    perror("can't open file");
    free(map->filename);
    map->filename = NULL;
    return RET_ERR;
  }
	
  // get file size
  size_t filesize = lseek(map->fd, 0, SEEK_END);
  if(filesize < map->width * map->height * map->bytes_per_elem) {
    filesize = map->width * map->height * map->bytes_per_elem;
    lseek(map->fd, filesize - 1, SEEK_SET);
    if(write(map->fd, "\0", 1) != 1) {
      perror("can't open file");
      free(map->filename);
      map->filename = NULL;
      return RET_ERR;
    }
  }
	
  // map the file into memory
  if((map->mem = (uint8_t *) MMAP(NULL, filesize,
				  PROT_READ | PROT_WRITE, 
				  MAP_FILE | MAP_SHARED, map->fd, 0)) == (void *)(-1)) {
    perror("mmap failed");
    free(map->filename);
    map->filename = NULL;
    close(map->fd);
    return RET_ERR;
  }

  map->storage_type = MAP_STORAGE_TYPE_FILE;
	
  return RET_OK;
}

/**
 * Use storage in opend file as storage for memory map
 */
ret_t mm_map_file_by_fd(memory_map_t * map, const char * const project_dir, int fd, const char * const filename) {

  assert(map != NULL);
  assert(project_dir != NULL);
  assert(filename != NULL);
  assert(fd > 0);
  if(map == NULL || project_dir == NULL || filename == NULL) return RET_INV_PTR;
	
  // reset existing resources
  if(map->mem != NULL && (munmap(map->mem, map->width * map->height * map->bytes_per_elem) == -1)) {
    puts("munmap failed");
    return RET_ERR;
  }
  if(map->fd > 0) close(map->fd);
  if(map->filename) free(map->filename);
	
  map->fd = fd;
  map->filename = strdup(filename);

  // get file size
  size_t filesize = lseek(map->fd, 0, SEEK_END);
  if(filesize < map->width * map->height * map->bytes_per_elem) {
    filesize = map->width * map->height * map->bytes_per_elem;
    lseek(map->fd, filesize - 1, SEEK_SET);
    if(write(map->fd, "\0", 1) != 1) {
      free(map->filename);
      map->filename = NULL;
      close(map->fd);
      return RET_ERR;
    }
  }
	
  // map the file into memory
  if((map->mem = (uint8_t *) MMAP(NULL, filesize,
				  PROT_READ | PROT_WRITE, 
				  MAP_FILE | MAP_SHARED, map->fd, 0)) == (void *)(-1)) {
    perror("mmap failed");
    free(map->filename);
    map->filename = NULL;
    close(map->fd);
		
    return RET_ERR;
  }

  map->storage_type = MAP_STORAGE_TYPE_FILE;
  return RET_OK;
}


void * mm_get_ptr(memory_map_t * map, unsigned int x, unsigned int y) {
#ifdef DEBUG_ASSERTS_IN_FCF
  assert(map != NULL);
  assert( map->mem != NULL);
  assert(x < map->width);
  assert(y < map->height);
#endif
  return map->mem + (y * map->width + x) * map->bytes_per_elem;
}


/**
 * In place scaling and translation. It doesn't enlarge the image.
 */
ret_t mm_scale_and_shift_in_place(memory_map_t *map, 
				  double scaling_x, double scaling_y, 
				  unsigned int shift_x, unsigned int shift_y) {
  int dst_x, dst_y;

  assert(scaling_x >= 1 || scaling_y >= 1);
  if(scaling_x < 1 || scaling_y < 1) return RET_ERR;

  uint8_t * buffer = (uint8_t *)malloc(map->bytes_per_elem);

  for(dst_y = map->height - 1; dst_y > 0; dst_y--) {
    int src_y = lrint( ((double)dst_y - (double)shift_y) / scaling_y);
    
    for(dst_x = map->width - 1; dst_x > 0; dst_x--) {
      int src_x = lrint( ((double)dst_x - (double)shift_x) / scaling_x);
      
      if( src_x > 0 && src_x < (int)map->height &&
	  src_y >= 0 && src_y < (int)map->height) {
	memcpy(buffer, mm_get_ptr(map, src_x, src_y), map->bytes_per_elem);
	memcpy(mm_get_ptr(map, dst_x, dst_y), buffer, map->bytes_per_elem);
      }
      else 
	memset(mm_get_ptr(map, dst_x, dst_y), 0, map->bytes_per_elem);
    }
  }

  free(buffer);
  return RET_OK;
}

/**
 * Check if two maps have the same size and copy data from src map
 * to destination map
 */
ret_t mm_clone_map_data(memory_map_t * const dst, memory_map_t * const src) {
  if(!src || !dst) return RET_INV_PTR;

  if(dst->width == src->width &&
     dst->height == src->height &&
     dst->bytes_per_elem == src->bytes_per_elem) {

    memcpy(dst->mem, src->mem, src->width * src->height * src->bytes_per_elem);

    return RET_OK;
  }
  else
    return RET_ERR;
}
