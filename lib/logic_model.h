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

#ifndef __LOGIC_MODEL_H__
#define __LOGIC_MODEL_H__

#include "globals.h"
#include "quadtree.h"
#include "port_color_manager.h"

typedef struct lmodel_via lmodel_via_t;
typedef struct lmodel_wire lmodel_wire_t;
typedef struct lmodel_gate lmodel_gate_t;
typedef struct lmodel_gate_template_port lmodel_gate_template_port_t;

typedef struct lmodel_gate_port lmodel_gate_port_t;
typedef struct lmodel_connection lmodel_connection_t;
typedef struct lmodel_gate_template lmodel_gate_template_t;
typedef struct lmodel_gate_template_set lmodel_gate_template_set_t;
typedef struct lmodel_gate_set lmodel_gate_set_t;

#define SELECT_STATE_NOT 0
#define SELECT_STATE_DIRECT 1
#define SELECT_STATE_ADJ 2

enum LM_OBJECT_TYPE {
  LM_TYPE_UNDEF = 0,
  LM_TYPE_GATE = 1,
  LM_TYPE_WIRE = 2,
  LM_TYPE_VIA = 4,
  LM_TYPE_GATE_PORT = 8
};

enum LM_PORT_TYPE {
  LM_PT_IN = 1,
  LM_PT_OUT = 2
};

/** modes to destroy lists gate templates */
enum GTS_DESTROY_MODE {
  DESTROY_CHILDREN = 1,
  DESTROY_CONTAINER_ONLY = 2
};

/** modes to destroy gates by template */
enum GS_DESTROY_MODE {
  DESTROY_ALL = 1,
  DESTROY_WO_MASTER = 2
};


enum AUTONAME_ORIENTATION {
  AN_ALONG_ROWS = 1,
  AN_ALONG_COLS = 2
};

enum LM_TEMPLATE_ORIENTATION {
  LM_TEMPLATE_ORIENTATION_UNDEFINED = 0,
  LM_TEMPLATE_ORIENTATION_NORMAL = 1,
  LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN = 2,
  LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT = 3,
  LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH = 4
};
  
typedef union {
  lmodel_via_t * via;
  lmodel_wire_t * wire;
  lmodel_gate_t * gate;
  lmodel_gate_port_t * gate_port;
} object_ptr_t;

struct lmodel_connection {
  LM_OBJECT_TYPE object_type;
  void * obj_ptr;
  lmodel_connection_t * next;
};

struct lmodel_gate_port {
  unsigned int port_id;
  lmodel_connection_t * connections;
  lmodel_gate_t * gate;
  lmodel_gate_template_port_t * tmpl_port;
  int is_selected;
  lmodel_gate_port_t * next;
};

struct lmodel_gate_template_port {
  unsigned int id;
  char * port_name;
  LM_PORT_TYPE port_type;
  unsigned int relative_x_coord;
  unsigned int relative_y_coord;
  unsigned int diameter;
  color_t color;
  lmodel_gate_template_port_t * next;
};


struct lmodel_gate_template {
  unsigned int id; // a value of zero indicates that this is undefined
  unsigned int master_image_min_x;
  unsigned int master_image_min_y;
  unsigned int master_image_max_x;
  unsigned int master_image_max_y;
  char * short_name;
  char * description;
  color_t fill_color; 
  color_t frame_color; 
  unsigned int reference_counter;
  lmodel_gate_template_port_t * ports;
};


struct lmodel_gate_template_set {
  lmodel_gate_template_t * gate; // should be renamed to 'template'
  lmodel_gate_template_set_t * next;
};

struct lmodel_gate_set {
  lmodel_gate_t * gate;
  lmodel_gate_set_t * next;
};

enum LM_VIA_DIR {
  LM_VIA_UP = 0,
  LM_VIA_DOWN = 1
};

struct lmodel_via {
  unsigned int x, y;
  unsigned int diameter;
  unsigned int id;  // object id
  LM_VIA_DIR direction;
  int is_selected;
  color_t color;
  lmodel_connection_t * connections;
  char * name;
};

struct lmodel_wire {
  unsigned int from_x, from_y, to_x, to_y;
  unsigned int diameter;
  unsigned int id; // object id
  int is_selected;
  color_t color1;
  color_t color2;

  lmodel_connection_t * connections;
  char * name;
};

/**
 * Definition for a gate object including its placement.
 */
struct lmodel_gate {
  unsigned int min_x, min_y, max_x, max_y;
  unsigned int id;  // object id
  lmodel_gate_template_t * gate_template;
  LM_TEMPLATE_ORIENTATION template_orientation;
  int is_selected;

  lmodel_gate_port_t * ports;
  char * name;
};


/* layer types */
enum LAYER_TYPE {
  LM_LAYER_TYPE_UNDEF = 0,
  LM_LAYER_TYPE_METAL = 1,
  LM_LAYER_TYPE_LOGIC = 2,
  LM_LAYER_TYPE_TRANSISTOR = 3 };


typedef struct id_list id_list_t;
struct id_list {
  unsigned int id;
  unsigned int sub_id;
  id_list_t * next;
};

typedef  struct connection_build_helper connection_build_helper_t;
struct connection_build_helper {
  LM_OBJECT_TYPE object_type;
  void * obj_ptr;
  unsigned int from_sub_port;
  id_list_t * to;
  connection_build_helper_t * next;
};


struct logic_model {
  quadtree_t ** root;

  LAYER_TYPE *layer_type;
  int num_layers;

  lmodel_gate_template_set_t * gate_template_set;
  lmodel_gate_set_t * gate_set;
  
  unsigned int object_id_counter;
  unsigned int width, height;

  connection_build_helper_t * conn_build_helper;
};
 
typedef struct logic_model logic_model_t;

typedef uint32_t lmodel_map_elem_t;

logic_model_t * lmodel_create(int num_layers, unsigned int max_x, unsigned int max_y);
ret_t lmodel_destroy(logic_model_t * const lmodel);
ret_t lmodel_destroy_gate_set(lmodel_gate_set_t * gset);
ret_t lmodel_destroy_gate_template_set(lmodel_gate_template_set_t * gset, GTS_DESTROY_MODE mode);
ret_t lmodel_destroy_gate_template(lmodel_gate_template_t * tmpl);
ret_t lmodel_destroy_gate_ports(lmodel_gate_port_t * gate_port);
ret_t lmodel_destroy_connections(lmodel_connection_t * connections);

ret_t lmodel_destroy_gate(lmodel_gate_t * gate);
ret_t lmodel_destroy_wire(lmodel_wire_t * wire);
ret_t lmodel_destroy_via(lmodel_via_t * via);

ret_t lmodel_add_gate_template(logic_model_t * const lmodel, const lmodel_gate_template_t * const tmpl);
lmodel_gate_template_set_t * lmodel_create_gate_template_set(lmodel_gate_template_t * const tmpl);

ret_t lmodel_add_gate_to_gate_set(logic_model_t * const lmodel, lmodel_gate_t * const gate);
ret_t lmodel_remove_gate_from_gate_set(logic_model_t * const lmodel, lmodel_gate_t * const gate);
lmodel_gate_t * lmodel_get_gate_from_set_by_name(lmodel_gate_set_t * gate_set);

ret_t lmodel_serialize_to_file(const logic_model_t * const lmodel, const char * const project_dir);
ret_t lmodel_load_files(logic_model_t * const lmodel, const char * const project_dir, int num_layers);

ret_t lmodel_set_layer_type(logic_model_t * const lmodel, int layer, LAYER_TYPE layer_type);
LAYER_TYPE  lmodel_get_layer_type(logic_model_t * const lmodel, int layer);
int lmodel_get_layer_num_by_type(const logic_model_t * const lmodel, LAYER_TYPE layer_type);

ret_t lmodel_get_layer_type_as_string(logic_model_t * const lmodel, int layer, char * const out_str, int len);
ret_t lmodel_set_layer_type_from_string(logic_model_t * const lmodel, int layer, const char * const type_str);

ret_t lmodel_object_to_string(const logic_model_t * const lmodel, int layer, 
			      unsigned int x, unsigned int y, char * const str_buffer, unsigned int buf_len);

ret_t lmodel_get_printable_string_for_obj(LM_OBJECT_TYPE object_type, void * obj_ptr, char * const msg, unsigned int len);

ret_t lmodel_get_object(const logic_model_t * const lmodel, int layer, unsigned int real_x, unsigned int real_y,
			LM_OBJECT_TYPE * result_object_type, void ** result_object_ptr);

ret_t lmodel_get_gate_in_region(const logic_model_t * const lmodel, int layer, 
				unsigned int from_real_x, unsigned int from_real_y,
				unsigned int to_real_x, unsigned int to_real_y,
				lmodel_gate_t ** result_object_ptr);

ret_t lmodel_set_select_state(LM_OBJECT_TYPE object_type, void * obj_ptr, int state);
int lmodel_get_select_state(LM_OBJECT_TYPE object_type, void * obj_ptr);

ret_t lmodel_clear_layer(logic_model_t * const lmodel, int layer);

ret_t lmodel_clear_area(logic_model_t * const lmodel, int layer, 
			unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y);




lmodel_wire_t * lmodel_create_wire(logic_model_t * const lmodel,
				   unsigned int from_x, unsigned int from_y,
				   unsigned int to_x, unsigned int to_y,
				   unsigned int diameter,
				   char * name,
				   unsigned int obj_id);

lmodel_via_t * lmodel_create_via(logic_model_t * const lmodel,
				 unsigned int x, unsigned int y,
				 LM_VIA_DIR direction,
				 unsigned int diameter,
				 char * name,
				 unsigned int obj_id);

lmodel_gate_t * lmodel_create_gate(logic_model_t * const lmodel,
				   unsigned int min_x, unsigned int min_y,
				   unsigned int max_x, unsigned int max_y,
				   lmodel_gate_template_t * gate_template,
				   char * name,
				   unsigned int obj_id);

ret_t lmodel_add_wire(logic_model_t * const lmodel, int layer, lmodel_wire_t * wire);
ret_t lmodel_add_via(logic_model_t * const lmodel, int layer, lmodel_via_t * via);
ret_t lmodel_add_gate(logic_model_t * const lmodel, int layer, lmodel_gate_t * gate);


ret_t lmodel_add_wire_with_autojoin(logic_model_t * const lmodel, int layer,
				    lmodel_wire_t * wire);
ret_t lmodel_add_via_with_autojoin(logic_model_t * const lmodel, int layer,
				   lmodel_via_t * via);

ret_t lmodel_remove_refs_to_gate_template(logic_model_t * const lmodel, lmodel_gate_template_t * const tmpl);

ret_t lmodel_remove_object_by_ptr(logic_model_t * const lmodel, int layer, void * ptr, LM_OBJECT_TYPE object_type);
ret_t lmodel_destroy_gates_by_template_type(logic_model_t * const lmodel, const lmodel_gate_template_t * const tmpl,
					    GS_DESTROY_MODE mode);

bool lmodel_object_is_connectable(LM_OBJECT_TYPE object_type);

ret_t lmodel_remove_all_connections_from_object(LM_OBJECT_TYPE object_type, void * obj);
ret_t lmodel_set_connections_for_object(LM_OBJECT_TYPE object_type, void * obj, lmodel_connection_t * conn);
lmodel_connection_t * lmodel_get_connections_from_object(LM_OBJECT_TYPE object_type, void * obj);
lmodel_connection_t ** lmodel_get_connection_head_from_object(LM_OBJECT_TYPE object_type, void * obj);
ret_t lmodel_connect_objects(LM_OBJECT_TYPE type1, void * obj1,
			     LM_OBJECT_TYPE type2, void * obj2);

ret_t lmodel_connect_object(lmodel_connection_t ** conn_list, LM_OBJECT_TYPE object_type, object_ptr_t * obj);
lmodel_connection_t * lmodel_find_connection_to_object(lmodel_connection_t * from_conn, object_ptr_t * obj);
//lmodel_connection_t * lmodel_get_all_connected_objects(const lmodel_connection_t * const connections);

ret_t lmodel_set_gate_name(lmodel_gate_t * gate, const char * const new_name);
ret_t lmodel_set_wire_name(lmodel_wire_t * wire, const char * const new_name);
ret_t lmodel_set_via_name(lmodel_via_t * via, const char * const new_name);
ret_t lmodel_set_name(LM_OBJECT_TYPE object_type, void * object, const char * const new_name);
char * lmodel_get_name(LM_OBJECT_TYPE object_type, void * obj_ptr);

// template handling
lmodel_gate_template_t * lmodel_create_gate_template();
ret_t lmodel_destroy_gate_template(lmodel_gate_template_t * tmpl);
ret_t lmodel_add_gate_template(logic_model_t * const lmodel, 
			       lmodel_gate_template_t * const tmpl, 
			       unsigned int obj_id);

ret_t lmodel_add_gate_template_to_gate_template_set(lmodel_gate_template_set_t * const gate_template_set, 
						    lmodel_gate_template_t * const tmpl, 
					   unsigned int obj_id);

ret_t lmodel_gate_template_set_master_region(lmodel_gate_template_t * const tmpl,
					     unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y);

ret_t lmodel_gate_template_set_text(lmodel_gate_template_t * const tmpl, 
				    const char * const short_name,
				    const char * const description);

ret_t lmodel_set_template_for_gate(logic_model_t * lmodel, lmodel_gate_t * gate, lmodel_gate_template_t * tmpl);
lmodel_gate_template_t * lmodel_get_template_for_gate(lmodel_gate_t * gate);

ret_t lmodel_set_gate_orientation(lmodel_gate_t * gate, LM_TEMPLATE_ORIENTATION orientation);

ret_t lmodel_adjust_templates_port_locations(lmodel_gate_template * const tmpl, 
					     LM_TEMPLATE_ORIENTATION trans);

LM_TEMPLATE_ORIENTATION lmodel_get_gate_orientation(lmodel_gate_t * gate);
LM_TEMPLATE_ORIENTATION lmodel_apply_transformation_chaining(LM_TEMPLATE_ORIENTATION trans1, 
							     LM_TEMPLATE_ORIENTATION trans2);
ret_t lmodel_adjust_gate_orientation_for_all_gates(logic_model_t * lmodel, lmodel_gate_t * except_for_gate, 
						   LM_TEMPLATE_ORIENTATION transformation);

ret_t lmodel_reset_gate_shape(lmodel_gate_t * gate);

lmodel_gate_template_t * lmodel_get_gate_template_by_id(logic_model_t * const lmodel, unsigned int obj_id);


int lmodel_gate_template_get_num_ports(const lmodel_gate_template_t * const tmpl);
ret_t lmodel_gate_template_add_port(logic_model_t * lmodel, 
				    lmodel_gate_template_t * const tmpl, const lmodel_gate_template_port_t * const port);
ret_t lmodel_gate_template_set_port(logic_model_t * lmodel, 
				    lmodel_gate_template_t * const tmpl, unsigned int id, 
				    const char * const port_name, LM_PORT_TYPE port_type);

ret_t lmodel_gate_template_remove_port(lmodel_gate_template_t * const tmpl, unsigned int port_id);

ret_t lmodel_remove_gate_template(logic_model_t * const lmodel, lmodel_gate_template_t * const tmpl);
ret_t lmodel_remove_gate_template_from_template_set(lmodel_gate_template_set_t ** tmpl_set, 
						    const lmodel_gate_template_t * const tmpl);

// port handling

lmodel_gate_template_port_t * lmodel_create_gate_template_port();
lmodel_gate_port_t * lmodel_get_port_from_gate_by_id(lmodel_gate_t * gate, unsigned int port_id);

lmodel_gate_port_t * lmodel_create_gate_port(lmodel_gate_t * gate, lmodel_gate_template_port_t * tmpl_port);
ret_t lmodel_add_gate_port_to_gate(lmodel_gate_t * gate, lmodel_gate_port_t * port);

ret_t get_line_function_for_wire(lmodel_wire_t * wire, double * m, double * n);

ret_t lmodel_update_gate_ports(lmodel_gate_t * gate);
ret_t lmodel_update_all_gate_ports(logic_model_t * lmodel, lmodel_gate_template_t * tmpl);

int lmodel_gate_is_master(const lmodel_gate_t * const gate);

ret_t lmodel_get_view_for_object(const logic_model_t * const lmodel, 
				 LM_OBJECT_TYPE object_type, const object_ptr_t * const obj_ptr,
				 unsigned int * center_x, unsigned int * center_y, unsigned int * layer);

ret_t lmodel_gate_template_set_color(lmodel_gate_template_t * gate_template, 
				     color_t fill_color, color_t frame_color);

ret_t lmodel_gate_template_get_color(lmodel_gate_template_t * gate_template, 
				     color_t * fill_color, color_t * frame_color);

ret_t lmodel_apply_colors_to_ports(logic_model_t * lmodel, const port_color_manager_t * const pcm);

ret_t lmodel_autoname_gates(logic_model_t * lmodel, unsigned int layer, 
			    AUTONAME_ORIENTATION orientation);

lmodel_gate_t * lmodel_get_gate_by_name(const logic_model_t * lmodel, const char * const short_name);

#endif
 
