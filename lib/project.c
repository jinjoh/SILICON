#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libconfig.h>

#include "project.h"
#include "renderer.h"

#define TEMPLATES_DAT "templates.dat"
#define TEMPLATE_PLACEMENT_DAT "template_placements.dat"
#define PROJECT_FILE "project.prj"

#define TM "project.c"

project_t * project_create(const char * const project_dir, unsigned int width, unsigned int height, int num_layers) {

  project_t * ptr = (project_t *)malloc(sizeof(project_t));
  if(!ptr) return NULL;
  
  memset(ptr, 0, sizeof(project_t));
  
  /* create logic modell */
  if((ptr->lmodel = lmodel_create(num_layers, width, height)) == NULL) {
    project_destroy(ptr);
    return NULL;
  }
  
  /* create image */
  if((ptr->bg_images = (image_t **)malloc(num_layers * sizeof(image_t *))) == NULL) {
    project_destroy(ptr);
    return NULL;
  }
  
  memset(ptr->bg_images, 0, num_layers * sizeof(image_t *));
  
  int i;
  for(i = 0; i < num_layers; i++) {
    if((ptr->bg_images[i] = gr_create_image(width, height, IMAGE_TYPE_RGBA)) == NULL) {
      project_destroy(ptr);
      return NULL;
    }
  }
  
  ptr->project_dir = strdup(project_dir);
  ptr->width = width;
  ptr->height = height;
  ptr->num_layers = num_layers;
  ptr->pin_diameter = 4;
  ptr->lambda = 4;
  ptr->wire_diameter = ptr->pin_diameter;
  
  if((ptr->alignment_marker_set = amset_create(num_layers)) == NULL) {
    project_destroy(ptr);
    return NULL;
  }
  return ptr;
}

void project_destroy(project_t * project) {
  int i;
  if(project->project_dir) free(project->project_dir);
  lmodel_destroy(project->lmodel);
  for(i = 0; i < project->num_layers; i++) {
    if(project->bg_images[i]) gr_image_destroy(project->bg_images[i]);
  }
  amset_destroy(project->alignment_marker_set);
  
  memset(project, 0, sizeof(project_t));
  free(project);
}

int project_map_background_memfiles(project_t * const project) {
  int i;

  for(i = 0; i < project->num_layers; i++) {
    char bg_mapping_filename[PATH_MAX];
    snprintf(bg_mapping_filename, sizeof(bg_mapping_filename), "bg_layer_%02d.dat", i);
    if(!RET_IS_OK(gr_map_file(project->bg_images[i],  project->project_dir, bg_mapping_filename))) {
      puts("mapping failed");
      return 0;
    }
  }
  return 1;
}

#define TEMPLATE_DAT_HEADER "# foo"
#define TEMPLATE_PLACEMENT_DAT_HEADER "# bar"

int project_init_directory(const char * const directory, int enable_mkdir) {
  char filename[PATH_MAX];
  FILE * f;
	
  if(!directory) return 0;
	
  if(enable_mkdir && (mkdir(directory, 0600) == -1)) return 0;

  snprintf(filename, sizeof(filename), "%s/%s", directory, TEMPLATES_DAT);
  if((f = fopen(filename, "w+")) == NULL) return 0;

  if(fwrite(TEMPLATE_DAT_HEADER, 1, sizeof(TEMPLATE_DAT_HEADER), f) != sizeof(TEMPLATE_DAT_HEADER)) {
    fclose(f);
    return 0;
  }

  fclose(f);

  snprintf(filename, sizeof(filename), "%s/%s", directory, TEMPLATE_PLACEMENT_DAT);
  if((f = fopen(filename, "w+")) == NULL) return 0;

  if(fwrite(TEMPLATE_PLACEMENT_DAT_HEADER, 1, sizeof(TEMPLATE_PLACEMENT_DAT_HEADER), f) != 
     sizeof(TEMPLATE_PLACEMENT_DAT_HEADER)) {
    fclose(f);
    return 0;
  }

  fclose(f);
	
  return 1;
}

#define PROJECT_READ_INT(name, variable) \
  if((setting = config_lookup(&cfg, name)) == NULL) { \
    printf("can't read config item %s\n", name); \
    config_destroy(&cfg); \
    return NULL; \
  } \
  else { \
    variable = config_setting_get_int(setting); \
  }

#define PROJECT_READ_FLOAT(name, variable) \
  if((setting = config_lookup(&cfg, name)) == NULL) { \
    printf("can't read config item %s\n", name); \
    config_destroy(&cfg); \
    return NULL; \
  } \
  else { \
    variable = config_setting_get_float(setting); \
  }


ret_t project_conv_file_to_dir(char * const filename) {

  char * ptr = strstr(filename, PROJECT_FILE);
  if(ptr != NULL) {
    *ptr = '\0';
  }
  return RET_OK;
}

project_t * project_load(const char * const project_dir) {
  //FILE * file;
  char filename[PATH_MAX];
  char base_dir[PATH_MAX];
  int width, height, num_layers;
  project_t * project;

  struct config_t cfg;
  config_setting_t *setting = NULL;

  if(!project_dir) return NULL;
  
  strncpy(base_dir, project_dir, sizeof(filename));
  if(RET_IS_NOT_OK(project_conv_file_to_dir(base_dir))) return NULL;

  snprintf(filename, sizeof(filename), "%s/%s", base_dir, PROJECT_FILE);

  config_init(&cfg);

  if (!config_read_file(&cfg, filename)) {
    printf("can't load config file %s\n", filename);
    config_destroy(&cfg);
    return NULL;
  }

  PROJECT_READ_INT("width", width);
  PROJECT_READ_INT("height", height);
  PROJECT_READ_INT("num_layers", num_layers);

  if((project = project_create(base_dir, width, height, num_layers)) == NULL) return NULL;

  project_map_background_memfiles(project);

  PROJECT_READ_INT("lambda", project->lambda);
  PROJECT_READ_INT("pin_diameter", project->pin_diameter);
  PROJECT_READ_INT("wire_diameter", project->wire_diameter);
  PROJECT_READ_INT("object_id_counter", project->lmodel->object_id_counter);

  PROJECT_READ_INT("grid.offset_x", project->grid.offset_x);
  PROJECT_READ_INT("grid.offset_y", project->grid.offset_y);
  PROJECT_READ_FLOAT("grid.dist_x", project->grid.dist_x);
  PROJECT_READ_FLOAT("grid.dist_y", project->grid.dist_y);
  PROJECT_READ_INT("grid.horizontal_lines_enabled", project->grid.horizontal_lines_enabled);
  PROJECT_READ_INT("grid.vertical_lines_enabled", project->grid.vertical_lines_enabled);

  // load layer types
  if((setting = config_lookup(&cfg, "layer_type")) == NULL) {
    printf("can't read config item layer_type\n");
    config_destroy(&cfg);
    return NULL;
  }
  else {
    int i;
    for(i = 0; i < num_layers; i++) {
      const char * layer_type_str = config_setting_get_string_elem(setting, i);
      if(!layer_type_str) {
	config_destroy(&cfg);
	return NULL;
      }
      else {
	if(!RET_IS_OK(lmodel_set_layer_type_from_string(project->lmodel, i, layer_type_str))) {
	  puts("can't convert");
	  config_destroy(&cfg);
	  return NULL;
	}
      }
    }
  }

  // load alignment markers
  if((setting = config_lookup(&cfg, "alignment_marker_set")) == NULL) {
    printf("can't read config item alignment_marker_set\n");
    config_destroy(&cfg);
    return NULL;
  }
  else {
    int i;
    for(i = 0; i < config_setting_length(setting); i +=4 ) {
      const char * marker_type_str = config_setting_get_string_elem(setting, i);
      long layer = config_setting_get_int_elem(setting, i+1);
      long x = config_setting_get_int_elem(setting, i+2);
      long y = config_setting_get_int_elem(setting, i+3);

      if(!amset_add_marker(project->alignment_marker_set, layer,
			   amset_mtype_str_to_mtype(marker_type_str), x, y)) {
	printf("can't add marker\n");
	config_destroy(&cfg);
	return NULL;
      }
    }
    amset_print(project->alignment_marker_set);
  }

  config_destroy(&cfg);


  if(RET_IS_NOT_OK(lmodel_load_files(project->lmodel, base_dir, num_layers))) {
    debug(TM, "Can't load logic model data from file");
    return NULL;
  }


  // cleanup directory
  if(RET_IS_NOT_OK(project_cleanup(base_dir))) return NULL;

  return project;
		
}


#define PROJECT_STORE_INT(_group, name, value) \
  if((setting = config_setting_add(_group, name, CONFIG_TYPE_INT)) == NULL) { \
    printf("Error in project_save(): can't store %s = %d\n", name, value); \
    config_destroy(&cfg); \
    return 0; \
  } \
  else { \
    config_setting_set_int(setting, value); \
  }

#define PROJECT_STORE_FLOAT(_group, name, value) \
  if((setting = config_setting_add(_group, name, CONFIG_TYPE_FLOAT)) == NULL) { \
    printf("Error in project_save(): can't store %s = %f\n", name, value); \
    config_destroy(&cfg); \
    return 0; \
  } \
  else { \
    printf("project_save(): store %s = %f\n", name, value); \
    config_setting_set_float(setting, value); \
  }



int project_save(const project_t * const project, const render_params_t * const render_params) {
  struct config_t cfg;
  config_setting_t *setting = NULL, *group = NULL, *array = NULL;
  char filename[PATH_MAX];

  config_init(&cfg);
  snprintf(filename, sizeof(filename), "%s/%s", project->project_dir, PROJECT_FILE);

  PROJECT_STORE_INT(cfg.root, "width", project->width);
  PROJECT_STORE_INT(cfg.root, "height", project->height);
  PROJECT_STORE_INT(cfg.root, "num_layers", project->num_layers);
  PROJECT_STORE_INT(cfg.root, "lambda", project->lambda);
  PROJECT_STORE_INT(cfg.root, "pin_diameter", project->pin_diameter);
  PROJECT_STORE_INT(cfg.root, "wire_diameter", project->wire_diameter);
  PROJECT_STORE_INT(cfg.root, "object_id_counter", project->lmodel->object_id_counter);

  // store grid
  if((group = config_setting_add(cfg.root, "grid", CONFIG_TYPE_GROUP)) == NULL) {
    config_destroy(&cfg);
    return 0;    
  }
  PROJECT_STORE_INT(group, "offset_x", project->grid.offset_x);
  PROJECT_STORE_INT(group, "offset_y", project->grid.offset_y);
  PROJECT_STORE_FLOAT(group, "dist_x", project->grid.dist_x);
  PROJECT_STORE_FLOAT(group, "dist_y", project->grid.dist_y);
  PROJECT_STORE_INT(group, "horizontal_lines_enabled", project->grid.horizontal_lines_enabled);
  PROJECT_STORE_INT(group, "vertical_lines_enabled", project->grid.vertical_lines_enabled);


  // store layer types
  if(render_params && render_params->lmodel && render_params->lmodel->layer_type) {
    int i;
    if((array = config_setting_add(cfg.root, "layer_type", CONFIG_TYPE_ARRAY)) == NULL) {
      puts("can't add node");
      config_destroy(&cfg);
      return 0;    
    }
    
    for(i = 0; i < project->num_layers; i++) {
      char layer_type_str[100];
      lmodel_get_layer_type_as_string(project->lmodel, i, layer_type_str, sizeof(layer_type_str));

      if((config_setting_set_string_elem(array, -1, layer_type_str)) == NULL) {
	puts("can't add value");
	config_destroy(&cfg);
	return 0;    
      }
    }
    
  }

  // store alignment markers
  if(project->alignment_marker_set && project->alignment_marker_set->markers) {
    int i;
    if((array = config_setting_add(cfg.root, "alignment_marker_set", CONFIG_TYPE_LIST)) == NULL) {
      puts("can't add node");
      config_destroy(&cfg);
      return 0;    
    }
    for(i = 0; i < project->alignment_marker_set->max_markers; i++) {
      alignment_marker_t * marker = project->alignment_marker_set->markers[i];
      if(marker) {
	
	char * mtype_str = amset_marker_type_to_str(marker->marker_type);

	if( ((config_setting_set_string_elem(array, -1, mtype_str)) == NULL) ||
	    ((config_setting_set_int_elem(array, -1, marker->layer)) == NULL) ||
	    ((config_setting_set_int_elem(array, -1, marker->x)) == NULL) ||
	    ((config_setting_set_int_elem(array, -1, marker->y)) == NULL) ) {
	  puts("can't add value");
	  config_destroy(&cfg);
	  free(mtype_str);
	  return 0;    
	}
	free(mtype_str);

      }
    }
  }

  config_write_file(&cfg, filename);
  config_destroy(&cfg);

  if(RET_IS_NOT_OK(lmodel_serialize_to_file(project->lmodel, project->project_dir))) {
    debug(TM, "Can't save logic model.");
    return 0;
  }

  return 1;
}


/**
   Remove temp files from project dir. In case of a crashed application, temp files are left.
 */
ret_t project_cleanup(const char * const project_dir) {

  DIR * dir;
  struct dirent * dir_ent;

  if((dir = opendir(project_dir)) == NULL) return RET_ERR;

  while((dir_ent = readdir(dir)) != NULL) {

    if(strlen(dir_ent->d_name) > 5 // there is already a length field but it's name depends on platform
       && !strncmp(dir_ent->d_name, "temp.", 5)) {

      char temp_file[PATH_MAX];
      snprintf(temp_file, PATH_MAX, "%s/%s", project_dir, dir_ent->d_name);
      
      debug(TM, "unlink temp file %s", temp_file);
      if(unlink(temp_file) == -1) return RET_ERR;
    }
  }

  return RET_OK;
}
