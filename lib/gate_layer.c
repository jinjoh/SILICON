#include "globals.h"
#include <wand/magick-wand.h>
#include "gate_layer.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define ThrowWandException(wand) \
{ \
  char \
    *description; \
 \
  ExceptionType \
    severity; \
 \
  description=MagickGetException(wand,&severity); \
  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description); \
  description=(char *) MagickRelinquishMemory(description); \
  exit(-1); \
}


#define GL_CLEANUP_AND_RET_ON_ERROR \
	if(ptr->img) free(ptr->img); \
	free(ptr); \
	magick_wand = DestroyMagickWand(magick_wand); \
	MagickWandTerminus(); \
	return NULL;

gate_template_t * glayer_load_template( unsigned int id,
					const char * const filename, 
					const char * const short_name,
					const char * const description,
					unsigned int num_ports_in, unsigned int num_ports_out) {

  MagickWandGenesis();
  MagickWand *magick_wand;
  MagickBooleanType status;

  gate_template_t * ptr;
  char * tmp;
  unsigned int i;

  if((ptr = (gate_template_t *)malloc(sizeof(gate_template_t))) == NULL) return NULL;
  
  memset(ptr, 0, sizeof(gate_template_t));

  magick_wand = NewMagickWand();  
  status = MagickReadImage(magick_wand, filename);
  if(status == MagickFalse) ThrowWandException(magick_wand);
  
  ptr->width = MagickGetImageWidth(magick_wand);
  ptr->height = MagickGetImageHeight(magick_wand);
  ptr->id = id;
  ptr->num_ports_in = num_ports_in;
  ptr->num_ports_out = num_ports_out;
  
  printf("\ttemplate %s: image size is %dx%d\n", filename, ptr->width, ptr->height);
  
 
  if((ptr->img = (char *) malloc(ptr->width * ptr->height)) == NULL) { GL_CLEANUP_AND_RET_ON_ERROR; }

  if((tmp = (char *) alloca(ptr->width * ptr->height * 3)) == NULL) { GL_CLEANUP_AND_RET_ON_ERROR; }

  MagickGetImagePixels(magick_wand, 0, 0, ptr->width, ptr->height, "RGB", CharPixel, tmp);
  
  for(i = 0; i < ptr->width * ptr->height; i++) {
    char * p = tmp + (i*3);
    if(*p == 0x23 && *(p + 1) == 0x42) {
      //if(*(p + 2) > ptr->ports_in * ptr->ports_out) { GL_CLEANUP_AND_RET_ON_ERROR; }
      //else 
	  ptr->img[i] = *(p + 2);	
    }
    else ptr->img[i] = 0;
  }

  ptr->description = strdup(description);
  ptr->short_name = strdup(short_name);

  magick_wand = DestroyMagickWand(magick_wand);
  MagickWandTerminus();
  return ptr;
}

int glayer_write_template_image(gate_template_t * tmpl, const char * const filename) {

  int fd;
  unsigned int i;
  if(!tmpl) return 0;
  
  fd = open(filename, O_RDWR | O_CREAT, 0600);
  if(fd == -1) return 0;

  for(i = 0; i < tmpl->width * tmpl->height; i++) {
	char x = tmpl->img[i];
	if(x > 0) x = 255 / x;
	
    write(fd, &x, 1);
    write(fd, &x, 1);
    write(fd, &x, 1);
  }
  close(fd);
  return 1;
}

gate_set_t * glayer_create_set() {
	gate_set_t * ptr;
	
	if((ptr = (gate_set_t *) malloc(sizeof(gate_set_t))) == NULL) return NULL;
	memset(ptr, 0, sizeof(gate_set_t));	
	return ptr;
}

void glayer_destroy_set(gate_set_t * set) {
	free(set);
}

void glayer_add_template(gate_set_t * set, gate_template_t * tmpl) {
	if(!set || !tmpl || tmpl->id >= MAX_ENTRIES_IN_GATE_SET) return;	
	set->set[tmpl->id] = tmpl;
	set->num++;
}



int read_token(char * txt, char * out_buf, char ** next_ptr) {
	char * end = txt + strlen(txt);
	char * ptr = txt;
	char * out_ptr = out_buf;
	
	int in_str = FALSE;
	while(ptr < end) {
		switch(*ptr) {
		case ' ': 
			if(in_str) {
				*out_ptr = *ptr;
				out_ptr++;
			}
			break;
		case ',':
			if(in_str) {
				*out_ptr = *ptr;
				out_ptr++;
			}
			else {
				*out_ptr = '\0';
				*next_ptr = ptr < end ? ptr+1 : NULL;
				return 1;
			}
			break;
		case '"':
			if(in_str) {
				in_str = FALSE;
			}
			else 
				in_str = TRUE;
			break;
		default:
			*out_ptr = *ptr;
			out_ptr++;
		}
		ptr++;
	}		

	*out_ptr = '\0';
	*next_ptr = ptr < end ? ptr+1 : NULL;
	return 1;
	
}

int parse_ports(char *line) {
	char * ptr = line;
	char * out_buf = (char *)alloca(sizeof(line));
	int num = 0;
	while(ptr) {
		read_token(ptr, out_buf, &ptr);
		num++;
	}
	return num;
}

int parse_line(	char * line,
				int * template_id,
				char * short_name,
				char * description,
				char * in_ports,
				char * out_ports,
				char * template_file) {
				
	char * ptr = line;
	char * end = line + strlen(line);
	while(ptr < end && (*ptr == ' ' || *ptr == '\t')) ptr++;
	if(ptr == end) return -1;
		
	if(ptr[0] == '#') return 0;
	
	char * tmpl_id_str = (char *)alloca(strlen(line));
	read_token(ptr, tmpl_id_str, &ptr);
	if(!ptr) return -1;
	*template_id = atoi(tmpl_id_str);
	
	read_token(ptr, short_name, &ptr);
	if(!ptr) return 1;
	
	read_token(ptr, description, &ptr);
	if(!ptr) return 2;
	
	read_token(ptr, in_ports, &ptr);
	if(!ptr) return 3;
	
	read_token(ptr, out_ports, &ptr);
	if(!ptr) return 4;
	
	read_token(ptr, template_file, &ptr);
	
	return 5;
}

gate_set_t * glayer_load_templates(const char * const project_dir, const char * const filename) {
	FILE * file;
	char line[10000];
	gate_set_t * gset = glayer_create_set();
	if(!gset) puts("glayer_create_set() failed");

	char * _filename = (char *) alloca(strlen(filename) + strlen(project_dir) + 2);
	strcpy(_filename, project_dir);
	strcat(_filename, "/");
	strcat(_filename, filename);
	
	if((file = fopen(_filename, "r")) == NULL) return NULL;
	while(!feof(file)) {
		if(fgets(line, sizeof(line), file) != NULL) {
			int len = strlen(line);
			if(len > 1) {
				line[len - 1] = '\0';
				int len = strlen(line);
				int template_id;
				char * short_name = (char *)alloca(len);
				char * description = (char *)alloca(len);
				char * in_ports = (char *)alloca(len);
				char * out_ports = (char *)alloca(len);
				char * template_file = (char *)alloca(len);

				memset(short_name, 0, len);
				memset(description, 0, len);
				memset(in_ports, 0, len);
				memset(out_ports, 0, len);
				memset(template_file, 0, len);
				int ret = parse_line(line, &template_id, short_name, description, in_ports, out_ports, template_file);
				if(ret == 5) {
				  unsigned int 
						num_in_ports = parse_ports(in_ports), 
						num_out_ports = parse_ports(out_ports);
					char * tmpl_file_name = (char *)alloca(strlen(project_dir) + strlen(template_file) + 2);
					strcpy(tmpl_file_name, project_dir);
					strcat(tmpl_file_name, "/");
					strcat(tmpl_file_name, template_file);
					
					gate_template_t * ptr = glayer_load_template(template_id, tmpl_file_name, short_name, description, num_in_ports, num_out_ports);
					if(!ptr) puts("template loading failed");
					glayer_add_template(gset, ptr);

				}
				else if(ret == 0);
				else {
					puts("Syntax error");
					fclose(file);
					return NULL;
				}
			}
		}
	}
	
	return gset;
}




