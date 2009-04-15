/*                                                                              
                                                                                
This file is part of the IC reverse engineering tool degate.                    
                                                                                
Copyright 2008, 2009 by Martin Schobert                                         
                                                                                
Degate is free software: you can redistribute it and/or modify                  
it under the terms of the GNU General Public License as published by            
the Free Software Foundation, either version 3 of the License, or               
any later version.                                                              
                                                                                
Degate is distributed in the hope that it will be useful,                       
but WITHOUT ANY WARRANTY; without even the implied warranty of                  
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   
GNU General Public License for more details.                                    
                                                                                
You should have received a copy of the GNU General Public License               
along with degate. If not, see <http://www.gnu.org/licenses/>.                  
                                                                                
*/

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libconfig.h>
#include <assert.h>

#include "project.h"
#include "renderer.h"
#include "port_color_manager.h"
#include "scaling_manager.h"

#define TEMPLATES_DAT "templates.dat"
#define TEMPLATE_PLACEMENT_DAT "template_placements.dat"
#define PROJECT_FILE "project.prj"

project_t * project_create(const char * const project_dir, 
			   unsigned int width, unsigned int height, int num_layers) {

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

  if((ptr->scaling_manager = scalmgr_create(num_layers, ptr->bg_images,
					    project_dir)) == NULL) {
    project_destroy(ptr);
    return NULL;
  }

  if(RET_IS_NOT_OK(scalmgr_set_scalings(ptr->scaling_manager, 8))) {
    project_destroy(ptr);
    return NULL;
  }

  if((ptr->port_color_manager = pcm_create()) == NULL) {
    project_destroy(ptr);
    return NULL;
  }

  if((ptr->alignment_marker_set = amset_create(num_layers)) == NULL) {
    project_destroy(ptr);
    return NULL;
  }

  if((ptr->grid = grid_create()) == NULL) {
    project_destroy(ptr);
    return NULL;
  }

  ptr->project_file_version = strdup(DEGATE_VERSION);
  ptr->project_name = strdup("");
  ptr->project_description = strdup("");
  return ptr;
}

/**
 * Set a project name.
 */
ret_t project_set_name(project_t * const project, const char * const name) {
  assert(project != NULL);
  assert(name != NULL);
  if(project == NULL || name == NULL) return RET_INV_PTR;

  if(project->project_name != NULL) free(project->project_name);
  project->project_name = strdup(name);

  return RET_OK;
}

/**
 * Set a description for a project.
 */

ret_t project_set_description(project_t * const project, const char * const descr) {
  assert(project != NULL);
  assert(descr != NULL);
  if(project == NULL || descr == NULL) return RET_INV_PTR;

  if(project->project_description != NULL) free(project->project_description);
  project->project_description = strdup(descr);

  return RET_OK;
}

/** 
 * Destroy a project and all objects that are referenced within a project. This includes
 * images and the logic model.
 */
ret_t project_destroy(project_t * project) {
  ret_t ret;
  int i;

  assert(project != NULL);
  if(project == NULL) return RET_INV_PTR;

  if(project->project_dir != NULL) free(project->project_dir);
  if(project->lmodel != NULL) 
    if(RET_IS_NOT_OK(ret = lmodel_destroy(project->lmodel))) return ret;
  
 for(i = 0; i < project->num_layers; i++) {
   if(project->bg_images[i] != NULL) 
     if(RET_IS_NOT_OK(ret = gr_image_destroy(project->bg_images[i]))) return ret;
  }

  if(project->alignment_marker_set != NULL)
    if(RET_IS_NOT_OK(ret = amset_destroy(project->alignment_marker_set))) return ret;
  
  if(project->scaling_manager != NULL) 
    if(RET_IS_NOT_OK(ret = scalmgr_destroy(project->scaling_manager))) return ret;

  if(project->port_color_manager != NULL) 
    if(RET_IS_NOT_OK(ret = pcm_destroy(project->port_color_manager))) return ret;

  if(project->project_name != NULL) free(project->project_name);
  if(project->project_description != NULL) free(project->project_description);

  if(project->project_file_version != NULL) free(project->project_file_version);

  if(project->grid != NULL) grid_destroy(project->grid);

  memset(project, 0, sizeof(project_t));
  free(project);
  return RET_OK;
}

ret_t project_map_background_memfiles(project_t * const project) {
  int i;
  ret_t ret;

  assert(project != NULL);
  assert(project->project_dir != NULL);
  assert(project->bg_images != NULL);
  if(project == NULL || project->project_dir == NULL || project->bg_images == NULL) 
    return RET_INV_PTR;
  
  for(i = 0; i < project->num_layers; i++) {
    char bg_mapping_filename[PATH_MAX];
    snprintf(bg_mapping_filename, sizeof(bg_mapping_filename), "bg_layer_%02d.dat", i);
    if(RET_IS_NOT_OK(ret = gr_map_file(project->bg_images[i],  
				       project->project_dir, bg_mapping_filename))) {
      puts("mapping failed");
      return ret;
    }
#ifdef MAP_FILES_ON_DEMAND
    if(RET_IS_NOT_OK(ret = gr_deactivate_mapping(project->bg_images[i]))) return ret;
#endif
  }
  return RET_OK;
}

#define TEMPLATE_DAT_HEADER "# foo"
#define TEMPLATE_PLACEMENT_DAT_HEADER "# bar"

ret_t project_init_directory(const char * const directory, int enable_mkdir) {
  char filename[PATH_MAX];
  FILE * f = NULL;
	
  if(directory == NULL) return RET_INV_PTR;
	
  if(enable_mkdir && (mkdir(directory, 0600) == -1)) return RET_ERR;

  snprintf(filename, sizeof(filename), "%s/%s", directory, TEMPLATES_DAT);
  if((f = fopen(filename, "w+")) == NULL) return RET_ERR;

  if(fwrite(TEMPLATE_DAT_HEADER, 1, sizeof(TEMPLATE_DAT_HEADER), f) != sizeof(TEMPLATE_DAT_HEADER)) {
    fclose(f);
    return RET_ERR;
  }

  fclose(f);

  snprintf(filename, sizeof(filename), "%s/%s", directory, TEMPLATE_PLACEMENT_DAT);
  if((f = fopen(filename, "w+")) == NULL) return RET_ERR;

  if(fwrite(TEMPLATE_PLACEMENT_DAT_HEADER, 1, sizeof(TEMPLATE_PLACEMENT_DAT_HEADER), f) != 
     sizeof(TEMPLATE_PLACEMENT_DAT_HEADER)) {
    fclose(f);
    return RET_ERR;
  }

  fclose(f);
	
  return RET_OK;
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

#define PROJECT_READ_INT_WO_CHECK(name, variable) \
  if((setting = config_lookup(&cfg, name)) != NULL) { \
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


#define PROJECT_READ_STRING(name, variable) \
  if((setting = config_lookup(&cfg, name)) == NULL) { \
    printf("can't read config item %s\n", name); \
    config_destroy(&cfg); \
    return NULL; \
  } \
  else { \
    variable = strdup(config_setting_get_string(setting)); \
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

  if((project = project_create(base_dir, width, height, num_layers)) == NULL) {
    config_destroy(&cfg);
    return NULL;
  }

  if(RET_IS_NOT_OK(project_map_background_memfiles(project))) {
    config_destroy(&cfg);
    project_destroy(project);
    return NULL;
  }

  PROJECT_READ_INT("lambda", project->lambda);
  PROJECT_READ_INT("pin_diameter", project->pin_diameter);
  PROJECT_READ_INT("wire_diameter", project->wire_diameter);
  PROJECT_READ_INT("object_id_counter", project->lmodel->object_id_counter);
  PROJECT_READ_STRING("project_name", project->project_name);
  PROJECT_READ_STRING("project_description", project->project_description);
  PROJECT_READ_STRING("project_file_version", project->project_file_version);

  // grid
  long grid_mode;
  PROJECT_READ_INT_WO_CHECK("grid.mode", grid_mode);
  project->grid->grid_mode = (GRID_MODE)grid_mode;

  // regular grid
  PROJECT_READ_INT("grid.offset_x", project->grid->offset_x);
  PROJECT_READ_INT("grid.offset_y", project->grid->offset_y);
  PROJECT_READ_FLOAT("grid.dist_x", project->grid->dist_x);
  PROJECT_READ_FLOAT("grid.dist_y", project->grid->dist_y);
  PROJECT_READ_INT("grid.horizontal_lines_enabled", project->grid->horizontal_lines_enabled);
  PROJECT_READ_INT("grid.vertical_lines_enabled", project->grid->vertical_lines_enabled);
  
  // unregular grid
  PROJECT_READ_INT_WO_CHECK("grid.uhg_enabled", project->grid->uhg_enabled);
  PROJECT_READ_INT_WO_CHECK("grid.uvg_enabled", project->grid->uvg_enabled);
  PROJECT_READ_INT_WO_CHECK("grid.num_uhg_entries", project->grid->num_uhg_entries);
  PROJECT_READ_INT_WO_CHECK("grid.num_uvg_entries", project->grid->num_uvg_entries);
  
  if(RET_IS_NOT_OK(grid_alloc_mem(project->grid, 
				  project->grid->num_uhg_entries, 
				  project->grid->num_uvg_entries)) ) {
    config_destroy(&cfg);
    project_destroy(project);
    return NULL;
  }
  
  if((setting = config_lookup(&cfg, "grid.uhg_offsets")) != NULL) {
    unsigned int i;
    for(i = 0; i < project->grid->num_uhg_entries; i++) {
      long offs = config_setting_get_int_elem(setting, i);
      project->grid->uhg_offsets[i] = offs;
    }
  }

  if((setting = config_lookup(&cfg, "grid.uvg_offsets")) != NULL) {
    unsigned int i;
    for(i = 0; i < project->grid->num_uvg_entries; i++) {
      long offs = config_setting_get_int_elem(setting, i);
      project->grid->uvg_offsets[i] = offs;
    }
  }
  
  
  // load layer types
  if((setting = config_lookup(&cfg, "layer_type")) == NULL) {
    printf("can't read config item layer_type\n");
    config_destroy(&cfg);
    project_destroy(project);
    return NULL;
  }
  else {
    int i;
    for(i = 0; i < num_layers; i++) {
      const char * layer_type_str = config_setting_get_string_elem(setting, i);
      if(!layer_type_str) {
	config_destroy(&cfg);
	project_destroy(project);
	return NULL;
      }
      else {
	if(!RET_IS_OK(lmodel_set_layer_type_from_string(project->lmodel, i, layer_type_str))) {
	  puts("can't convert");
	  config_destroy(&cfg);
	  project_destroy(project);
	  return NULL;
	}
      }
    }
  }


  // load alignment markers
  if((setting = config_lookup(&cfg, "alignment_marker_set")) == NULL) {
    printf("can't read config item alignment_marker_set\n");
    config_destroy(&cfg);
    project_destroy(project);
    return NULL;
  }
  else {
    int i;
    for(i = 0; i < config_setting_length(setting); i +=4 ) {
      const char * marker_type_str = config_setting_get_string_elem(setting, i);
      long layer = config_setting_get_int_elem(setting, i+1);
      long x = config_setting_get_int_elem(setting, i+2);
      long y = config_setting_get_int_elem(setting, i+3);

      if(RET_IS_NOT_OK(amset_add_marker(project->alignment_marker_set, layer,
					amset_mtype_str_to_mtype(marker_type_str), x, y))) {
	printf("can't add marker\n");
	config_destroy(&cfg);
	project_destroy(project);
	return NULL;
      }
    }
    amset_print(project->alignment_marker_set);
  }

  // load port colors
  if((setting = config_lookup(&cfg, "port_colors")) == NULL) {
    printf("can't read config item port_colors\n");
    config_destroy(&cfg);
    project_destroy(project);
    return NULL;
  }
  else {
    int i;
    for(i = 0; i < config_setting_length(setting); i +=5 ) {
      const char * port_name_str = config_setting_get_string_elem(setting, i);
      long red = config_setting_get_int_elem(setting, i+1);
      long green = config_setting_get_int_elem(setting, i+2);
      long blue = config_setting_get_int_elem(setting, i+3);
      long alpha = config_setting_get_int_elem(setting, i+4);

      if(RET_IS_NOT_OK(pcm_add_color_as_rgba(project->port_color_manager, port_name_str, 
					     red, green, blue, alpha))) {
	debug(TM, "Can't add color.");
	config_destroy(&cfg);
	project_destroy(project);
	return NULL;
      }

    }
  }
  
  config_destroy(&cfg);


  if(RET_IS_NOT_OK(scalmgr_load_scalings(project->scaling_manager))) {
    project_destroy(project);
    return NULL;
  }


  if(RET_IS_NOT_OK(lmodel_load_files(project->lmodel, base_dir, num_layers))) {
    debug(TM, "Can't load logic model data from file");
    project_destroy(project);
    return NULL;
  }


  if(RET_IS_NOT_OK(lmodel_apply_colors_to_ports(project->lmodel, project->port_color_manager))) {
    project_destroy(project);
    return NULL;
  }

  // cleanup directory
  if(RET_IS_NOT_OK(project_cleanup(base_dir))) {
    project_destroy(project);
    return NULL;
  }

  return project;
		
}


#define PROJECT_STORE_INT(_group, name, value) \
  if((setting = config_setting_add(_group, name, CONFIG_TYPE_INT)) == NULL) { \
    printf("Error in project_save(): can't store %s = %d\n", name, value); \
    config_destroy(&cfg); \
    return RET_ERR; \
  } \
  else { \
    config_setting_set_int(setting, value); \
  }

#define PROJECT_STORE_FLOAT(_group, name, value) \
  if((setting = config_setting_add(_group, name, CONFIG_TYPE_FLOAT)) == NULL) { \
    printf("Error in project_save(): can't store %s = %f\n", name, value); \
    config_destroy(&cfg); \
    return RET_ERR; \
  } \
  else { \
    printf("project_save(): store %s = %f\n", name, value); \
    config_setting_set_float(setting, value); \
  }

#define PROJECT_STORE_STRING(_group, name, value) \
  if((setting = config_setting_add(_group, name, CONFIG_TYPE_STRING)) == NULL) { \
    printf("Error in project_save(): can't store %s = %s\n", name, value); \
    config_destroy(&cfg); \
    return RET_ERR; \
  } \
  else { \
    config_setting_set_string(setting, value); \
  }


ret_t project_save(const project_t * const project) {
  struct config_t cfg;
  config_setting_t *setting = NULL, *group = NULL, *array = NULL;
  char filename[PATH_MAX];
  
  assert(project != NULL);
  if(project == NULL) return RET_INV_PTR;

  config_init(&cfg);
  snprintf(filename, sizeof(filename), "%s/%s", project->project_dir, PROJECT_FILE);

  PROJECT_STORE_INT(cfg.root, "width", project->width);
  PROJECT_STORE_INT(cfg.root, "height", project->height);
  PROJECT_STORE_INT(cfg.root, "num_layers", project->num_layers);
  PROJECT_STORE_INT(cfg.root, "lambda", project->lambda);
  PROJECT_STORE_INT(cfg.root, "pin_diameter", project->pin_diameter);
  PROJECT_STORE_INT(cfg.root, "wire_diameter", project->wire_diameter);
  PROJECT_STORE_INT(cfg.root, "object_id_counter", project->lmodel->object_id_counter);
  PROJECT_STORE_STRING(cfg.root, "project_name", project->project_name);
  PROJECT_STORE_STRING(cfg.root, "project_description", project->project_description);
  PROJECT_STORE_STRING(cfg.root, "project_file_version", project->project_file_version);

  // store grid
  if((group = config_setting_add(cfg.root, "grid", CONFIG_TYPE_GROUP)) == NULL) {
    config_destroy(&cfg);
    return RET_ERR;
  }
  PROJECT_STORE_INT(group, "offset_x", project->grid->offset_x);
  PROJECT_STORE_INT(group, "offset_y", project->grid->offset_y);
  PROJECT_STORE_FLOAT(group, "dist_x", project->grid->dist_x);
  PROJECT_STORE_FLOAT(group, "dist_y", project->grid->dist_y);
  PROJECT_STORE_INT(group, "horizontal_lines_enabled", project->grid->horizontal_lines_enabled);
  PROJECT_STORE_INT(group, "vertical_lines_enabled", project->grid->vertical_lines_enabled);

  PROJECT_STORE_INT(group, "mode", project->grid->grid_mode);
  PROJECT_STORE_INT(group, "uhg_enabled", project->grid->uhg_enabled);
  PROJECT_STORE_INT(group, "uvg_enabled", project->grid->uvg_enabled);
  PROJECT_STORE_INT(group, "num_uhg_entries", project->grid->num_uhg_entries);
  PROJECT_STORE_INT(group, "num_uvg_entries", project->grid->num_uvg_entries);


  {
    unsigned int i;
    if((array = config_setting_add(group, "uhg_offsets", CONFIG_TYPE_ARRAY)) == NULL) {
      puts("can't add node");
      config_destroy(&cfg);
      return RET_ERR;
    }
    else {
      for(i = 0; i < project->grid->num_uhg_entries; i++) {
	if(config_setting_set_int_elem(array, -1, project->grid->uhg_offsets[i]) == NULL) {
	  config_destroy(&cfg);
	  return RET_ERR;
	}
      }
    }

    if((array = config_setting_add(group, "uvg_offsets", CONFIG_TYPE_ARRAY)) == NULL) {
      puts("can't add node");
      config_destroy(&cfg);
      return RET_ERR;
    }
    else {
      for(i = 0; i < project->grid->num_uvg_entries; i++) {
	if(config_setting_set_int_elem(array, -1, project->grid->uvg_offsets[i]) == NULL) {
	  config_destroy(&cfg);
	  return RET_ERR;
	}
      }
    }
    
  }

  // store layer types
  if(project->lmodel != NULL && project->lmodel->layer_type != NULL) {
    int i;
    if((array = config_setting_add(cfg.root, "layer_type", CONFIG_TYPE_ARRAY)) == NULL) {
      puts("can't add node");
      config_destroy(&cfg);
      return RET_ERR;
    }
    
    for(i = 0; i < project->num_layers; i++) {
      char layer_type_str[100];
      lmodel_get_layer_type_as_string(project->lmodel, i, layer_type_str, sizeof(layer_type_str));

      if((config_setting_set_string_elem(array, -1, layer_type_str)) == NULL) {
	puts("can't add value");
	config_destroy(&cfg);
	return RET_ERR;
      }
    }
   
  }
  else return RET_ERR;

  // store alignment markers
  if(project->alignment_marker_set != NULL && project->alignment_marker_set->markers != NULL) {
    int i;
    if((array = config_setting_add(cfg.root, "alignment_marker_set", CONFIG_TYPE_LIST)) == NULL) {
      puts("Can't add node.");
      config_destroy(&cfg);
      return RET_ERR;    
    }
    for(i = 0; i < project->alignment_marker_set->max_markers; i++) {
      alignment_marker_t * marker = project->alignment_marker_set->markers[i];
      if(marker != NULL) {
	
	char * mtype_str = amset_marker_type_to_str(marker->marker_type);

	if( (config_setting_set_string_elem(array, -1, mtype_str) == NULL) ||
	    (config_setting_set_int_elem(array, -1, marker->layer) == NULL) ||
	    (config_setting_set_int_elem(array, -1, marker->x) == NULL) ||
	    (config_setting_set_int_elem(array, -1, marker->y) == NULL) ) {
	  puts("Can't add value.");
	  config_destroy(&cfg);
	  free(mtype_str);
	  return RET_ERR;
	}
	free(mtype_str);

      }
    }
  }

  // store port colors
  if(project->port_color_manager != NULL) {

    if((array = config_setting_add(cfg.root, "port_colors", CONFIG_TYPE_LIST)) == NULL) {
      puts("Can't add node.");
      config_destroy(&cfg);
      return RET_ERR;    
    }

    port_color_list_t * ptr = project->port_color_manager->port_color_list;
    while(ptr != NULL) {
      if( (config_setting_set_string_elem(array, -1, ptr->port_name) == NULL) ||
	  (config_setting_set_int_elem(array, -1, MASK_R(ptr->color) ) == NULL) ||
	  (config_setting_set_int_elem(array, -1, MASK_G(ptr->color) ) == NULL) ||
	  (config_setting_set_int_elem(array, -1, MASK_B(ptr->color) ) == NULL) ||
	  (config_setting_set_int_elem(array, -1, MASK_A(ptr->color) ) == NULL)) {
	debug(TM, "Can't write color definition for port.");
	config_destroy(&cfg);
	return RET_ERR;
      }
      ptr = ptr->next;
    }
    
  }

  config_write_file(&cfg, filename);
  config_destroy(&cfg);

  if(RET_IS_NOT_OK(lmodel_serialize_to_file(project->lmodel, project->project_dir))) {
    debug(TM, "Can't save logic model.");
    return RET_ERR;
  }

  return RET_OK;
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
