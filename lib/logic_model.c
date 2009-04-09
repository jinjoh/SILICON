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
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>

#include "globals.h"
#include "logic_model.h"
#include "quadtree.h"
#include <FileContent.h>
#include "graphics.h"

#define EPSILON 0.001

#define CHECK(lmodel, layer) \
   assert(lmodel != NULL); \
   assert(layer < lmodel->num_layers); \
   assert(lmodel->root != NULL); \
   assert(lmodel->root[layer] != NULL); \
   if(!lmodel) return RET_INV_PTR; \
   if(layer >= lmodel->num_layers) return RET_ERR; \
   if(lmodel->root == NULL) return RET_INV_PTR; \
   if(lmodel->root[layer] == NULL) return RET_INV_PTR;


// helper structure for serializing objects
typedef struct { 
  FileContent_t * file_content; 
  int layer;
} lmodel_qtree_ser_data_t;

// helper structure to check, if an object is at (x,y)
typedef struct {
  unsigned int x, y;
  // results:
  LM_OBJECT_TYPE object_type;
  void * object;
} lmodel_is_object_at_t;

// helper structure to check, if there is a gate in a region
typedef struct {
  unsigned int from_x, from_y, to_x, to_y;
  lmodel_gate_t * object;
} lmodel_is_gate_in_region_t;

// helper structure to search for object
typedef struct {
  unsigned int object_id, sub_id;
  // results:
  LM_OBJECT_TYPE object_type;
  void * object;
} lmodel_lookup_object_t;

// helper struct to check collisions
typedef struct {
  logic_model_t * lmodel;
  int layer;
  LM_OBJECT_TYPE object_type;
  void * object;
} lmodel_check_collision_t;

ret_t cb_join_on_contact(quadtree_t * qtree, lmodel_check_collision_t * data);

/**
 * Create a new logic model.
 */
logic_model_t * lmodel_create(int num_layers, unsigned int max_x, unsigned int max_y) {
	
  logic_model_t * lmodel;
	
  if((lmodel = (logic_model_t *) malloc(sizeof(logic_model_t))) == NULL) {
    return NULL;
  }
  else {
		
    memset(lmodel, 0, sizeof(logic_model_t));
    lmodel->num_layers = num_layers;
    lmodel->width = max_x;
    lmodel->height = max_y;
    lmodel->object_id_counter = 1;   
    
    if((lmodel->root = (quadtree_t **) malloc(num_layers * sizeof(quadtree_t *))) == NULL) {
      lmodel_destroy(lmodel);
      return NULL;
    }
    else {
      int i;
      for(i = 0; i < num_layers; i++) {
	if((lmodel->root[i] = quadtree_create(0, 0, max_x, max_y)) == NULL) {
	  lmodel_destroy(lmodel);
	  return NULL;
	}
      }
    }

    if((lmodel->layer_type = (LAYER_TYPE*) malloc(num_layers * sizeof(LAYER_TYPE))) == NULL) {
      lmodel_destroy(lmodel);
      return NULL;
    }
		
    return lmodel;
  }
}

/** 
 * Destroy data structure for connections for an object.
 */
ret_t lmodel_destroy_connections(lmodel_connection_t * connections) {
  lmodel_connection_t * ptr, * ptr_next;
  assert(connections);
  if(!connections) return RET_INV_PTR;
  ptr = connections;
  while(ptr != NULL) {
    ptr_next = ptr->next;
    memset(ptr, 0, sizeof(lmodel_connection_t));
    free(ptr);
    ptr = ptr_next;
  }
  
  return RET_OK;
}

/**
 * Destroy all ports from a gate.
 */

ret_t lmodel_destroy_gate_ports(lmodel_gate_port_t * gate_port) {
  lmodel_gate_port_t * ptr, * ptr_next;
  ret_t ret;
  assert(gate_port);
  if(!gate_port) return RET_INV_PTR;

  ptr = gate_port;
  while(ptr) {
    if(ptr->connections) {
      if(RET_IS_NOT_OK(ret = lmodel_remove_all_connections_from_object(LM_TYPE_GATE_PORT, ptr))) return ret;
      if(ptr->connections && RET_IS_NOT_OK(ret = lmodel_destroy_connections(ptr->connections))) return ret;
    }
    ptr_next = ptr->next;
    free(ptr);
    ptr = ptr->next;
  }

  return RET_OK;
}


/**
 * Destroy a gate object: Free its memory. Isolate its ports and destroy its ports as well.
 * It does not remove the gate from the gate set. It does not remove the gate from the quadtree.
 *
 * @see lmodel_remove_object_by_ptr()
 */
ret_t lmodel_destroy_gate(lmodel_gate_t * gate) {
  ret_t ret;
  assert(gate != NULL);
  if(gate == NULL) return RET_INV_PTR;

  if(gate->name != NULL) free(gate->name);
  if(gate->ports != NULL) if(RET_IS_NOT_OK(ret = lmodel_destroy_gate_ports(gate->ports))) return ret;
  gate->ports = NULL;

  if(gate->gate_template != NULL) {
    assert(gate->gate_template->reference_counter > 0);
    gate->gate_template->reference_counter--;
  }

  return RET_OK;
}

/**
 * Free memory for a wire. It removes connection references to the wire first. This function
 * does not remove the wire from the underlying quadtree.
 *
 * @see lmodel_remove_object_by_ptr()
 */
ret_t lmodel_destroy_wire(lmodel_wire_t * wire) {
  ret_t ret;
  assert(wire);
  if(!wire) return RET_INV_PTR;

  if(wire->name) free(wire->name);
  if(wire->connections) {
    if(RET_IS_NOT_OK(ret = lmodel_remove_all_connections_from_object(LM_TYPE_WIRE, wire))) return ret;
    if(wire->connections && RET_IS_NOT_OK(ret = lmodel_destroy_connections(wire->connections))) return ret;
  }
  return RET_OK;
}

/**
 * Free memory for a via. It removes connection references to the via first. This function
 * does not remove the via from the underlying quadtree.
 *
 * @see lmodel_remove_object_by_ptr()
 */

ret_t lmodel_destroy_via(lmodel_via_t * via) {
  ret_t ret;
  assert(via);
  if(!via) return RET_INV_PTR;

  if(via->name) free(via->name);
  if(via->connections) {
    if(RET_IS_NOT_OK(ret = lmodel_remove_all_connections_from_object(LM_TYPE_VIA, via))) return ret;
    if(via->connections && RET_IS_NOT_OK(ret = lmodel_destroy_connections(via->connections))) return ret;
  }
  return RET_OK;
}

ret_t cb_destroy_objects(quadtree_t * qtree, void * ptr) {

  quadtree_object_t * object, * next;
  ret_t ret;

  assert(qtree);
  if(!qtree) return RET_INV_PTR;

  object = qtree->objects;
  
  while(object != NULL) {
    switch(object->object_type) {
    case LM_TYPE_GATE:
      if(RET_IS_NOT_OK(ret = lmodel_destroy_gate((lmodel_gate_t *) object->object))) {
	debug(TM, "Can't destroy gate");
	return ret;
      }
      break;
    case LM_TYPE_WIRE:
      if(RET_IS_NOT_OK(ret = lmodel_destroy_wire((lmodel_wire_t *) object->object))) {
	debug(TM, "Can't destroy wire");
	return ret;
      }
      break;
    case LM_TYPE_VIA:
      if(RET_IS_NOT_OK(ret = lmodel_destroy_via((lmodel_via_t *) object->object))) {
	debug(TM, "Can't destroy via");
	return ret;
      }
      break;
    case LM_TYPE_UNDEF:
    default:
      debug(TM, "unknown object type");
      return RET_ERR;
    }

    next = object->next;
    if(RET_IS_NOT_OK(ret = quadtree_remove_object(object))) {
      debug(TM, "quadtree_remove_object() failed");
      return ret;
    }

    object = qtree->objects;
  }
  return RET_OK;
}

/**
 * Destroy a complete logic model including all objects.
 */
ret_t lmodel_destroy(logic_model_t * const lmodel) {
  int i;
  ret_t ret;
  assert(lmodel);
  if(!lmodel) return RET_INV_PTR;

  for(i=0; i < lmodel->num_layers; i++) {
    quadtree_traverse_complete(lmodel->root[i], (quadtree_traverse_func_t) &cb_destroy_objects, NULL);

    if(lmodel->root[i]) quadtree_destroy(lmodel->root[i]);
  }
  
  if(lmodel->root) free(lmodel->root);
  if(lmodel->layer_type) free(lmodel->layer_type);
  
  if(lmodel->gate_template_set &&
     RET_IS_NOT_OK(ret = lmodel_destroy_gate_template_set(lmodel->gate_template_set, DESTROY_CHILDREN))) return ret;

  if(lmodel->gate_set && RET_IS_NOT_OK(ret = lmodel_destroy_gate_set(lmodel->gate_set))) return ret;
  free(lmodel);

  return RET_OK;
}

/**
 * Destroy a template set.
 */
ret_t lmodel_destroy_gate_template_set(lmodel_gate_template_set_t * gset, GTS_DESTROY_MODE mode) {
  ret_t ret;
  lmodel_gate_template_set_t * ptr = gset;

  assert(gset);
  if(!gset) return RET_INV_PTR;
  
  while(ptr) {
    if(mode == DESTROY_CHILDREN && RET_IS_NOT_OK(ret = lmodel_destroy_gate_template(ptr->gate))) return ret;
    lmodel_gate_template_set_t * ptr_next = ptr->next;
    memset(ptr, 0, sizeof(lmodel_gate_template_set_t));
    free(ptr);
    ptr = ptr_next;
  }
  return RET_OK;
}

ret_t lmodel_destroy_gate_set(lmodel_gate_set_t * gset) {
  lmodel_gate_set_t * ptr = gset;
  assert(gset);
  if(!gset) return RET_INV_PTR;
  
  while(ptr) {
    lmodel_gate_set_t * ptr_next = ptr->next;
    memset(ptr, 0, sizeof(lmodel_gate_set_t));
    free(ptr);
    ptr = ptr_next;
  }
  return RET_OK;
}

ret_t lmodel_destroy_gate_template(lmodel_gate_template_t * tmpl) {
  lmodel_gate_template_port_t * ptr, * ptr_next;

  assert(tmpl);
  if(!tmpl) return RET_INV_PTR;

  if(tmpl->short_name) free(tmpl->short_name);
  if(tmpl->description) free(tmpl->description);

  ptr = tmpl->ports;
  while(ptr) {
    if(ptr->port_name) free(ptr->port_name);
    ptr_next = ptr->next;
    memset(ptr, 0, sizeof(lmodel_gate_template_port_t));
    free(ptr);
    ptr = ptr_next;
  }

  free(tmpl);
  return RET_OK;
}

static int write_out(const void *buffer, size_t size, void *app_key) {
  FILE *out_fp = (FILE *)app_key;
  size_t wrote;
  
  wrote = fwrite(buffer, 1, size, out_fp);

  return (wrote == size) ? 0 : -1;
}


ret_t lmodel_serialize_connections(lmodel_connection_t * lm_connections, void * list_ptr) {
  lmodel_connection_t * ptr;
  assert(lm_connections);
  assert(list_ptr);
  if(!lm_connections || !list_ptr) return RET_INV_PTR;

  ptr = lm_connections;
  while(ptr) {
    int id;
    int sub_id = 0;
    lmodel_gate_port_t * gport;
    assert(ptr->obj_ptr);
    if(!ptr->obj_ptr) return RET_INV_PTR;
    LogicModelConnection_t * conn = (LogicModelConnection_t *)calloc(1, sizeof(LogicModelConnection_t));

    switch(ptr->object_type) {
    case LM_TYPE_VIA: 
      id = ((lmodel_via_t *)ptr->obj_ptr)->id; 
      break;
    case LM_TYPE_WIRE: 
      id = ((lmodel_wire_t *)ptr->obj_ptr)->id; 
      break;
    case LM_TYPE_GATE_PORT:
      gport = (lmodel_gate_port_t *) ptr->obj_ptr;
      id =  gport->gate->id;
      sub_id = gport->port_id;
      break;
    default:
      debug(TM, "error in logic model");
      return RET_ERR;     
    }

    asn_long2INTEGER(&conn->object_id, id);
    asn_long2INTEGER(&conn->sub_id, sub_id);

    ASN_SEQUENCE_ADD(list_ptr, conn);

    ptr = ptr->next;
  }

  return RET_OK;
}


ret_t lmodel_serialize_gate_to_file(const lmodel_gate_t * const gate, lmodel_qtree_ser_data_t * ser_data) {

  Object_t *object; 
  ret_t ret;

  object = (Object_t *)calloc(1, sizeof(Object_t)); /* not malloc! */
  if(!object) return RET_ERR;
  object->present = Object_PR_gate;
  asn_long2INTEGER(&(object->choice.gate.min_x), gate->min_x);
  asn_long2INTEGER(&(object->choice.gate.min_y), gate->min_y);
  asn_long2INTEGER(&(object->choice.gate.max_x), gate->max_x);
  asn_long2INTEGER(&(object->choice.gate.max_y), gate->max_y);
  asn_long2INTEGER(&(object->choice.gate.gate_id), gate->gate_template ? gate->gate_template->id : 0 );
  asn_long2INTEGER(&(object->choice.gate.id), gate->id);

  OCTET_STRING_fromBuf(&object->choice.gate.name, gate->name, -1);

  switch(gate->template_orientation) {
  case LM_TEMPLATE_ORIENTATION_UNDEFINED: 
    asn_long2INTEGER(&(object->choice.gate.master_orientation), TemplateOrientationType_undefined);
    break;
  case LM_TEMPLATE_ORIENTATION_NORMAL: 
    asn_long2INTEGER(&(object->choice.gate.master_orientation), TemplateOrientationType_normal);
    break;
  case LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN: 
    asn_long2INTEGER(&(object->choice.gate.master_orientation), TemplateOrientationType_flipped_up_down);
    break;
  case LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT: 
    asn_long2INTEGER(&(object->choice.gate.master_orientation), TemplateOrientationType_flipped_left_right);
    break;
  case LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH: 
    asn_long2INTEGER(&(object->choice.gate.master_orientation), TemplateOrientationType_flipped_both);
    break;
  default: 
    return RET_ERR;
  }

  asn_long2INTEGER(&(object->choice.gate.layer), ser_data->layer);

  // serialize gate ports
  lmodel_gate_port_t * ptr = gate->ports;
  while(ptr != NULL) {
    assert(ptr->gate != NULL);
    assert(ptr->gate->gate_template != NULL);
    assert(ptr->tmpl_port != NULL);
    GatePort_t * p = (GatePort_t *) calloc(1, sizeof(GatePort_t));
    asn_long2INTEGER(&p->port_id, ptr->port_id);

    if(ptr->connections &&
     RET_IS_NOT_OK(ret = lmodel_serialize_connections(ptr->connections, 
						      &p->connections.list)))
    return ret;


    ASN_SEQUENCE_ADD(&object->choice.gate.ports.list, p);
    ptr = ptr->next;
  }

  ASN_SEQUENCE_ADD(&ser_data->file_content->list, object);
    
  
  return RET_OK;
}

ret_t lmodel_serialize_color(Color_t * dst_col, color_t col) {
  assert(dst_col != NULL);
  if(dst_col == NULL) return RET_INV_PTR;
  dst_col->red = MASK_R(col);
  dst_col->green = MASK_G(col);
  dst_col->blue = MASK_B(col);
  dst_col->alpha = MASK_A(col);
  return RET_OK;
}

ret_t lmodel_serialize_wire_to_file(const lmodel_wire_t * const wire, lmodel_qtree_ser_data_t * ser_data) {
  ret_t ret;
  Object_t * object; 

  object = (Object_t *)calloc(1, sizeof(Object_t)); /* not malloc! */
  if(!object) return RET_ERR;
  object->present = Object_PR_wire;
  asn_long2INTEGER(&(object->choice.wire.from_x), wire->from_x);
  asn_long2INTEGER(&(object->choice.wire.from_y), wire->from_y);
  asn_long2INTEGER(&(object->choice.wire.to_x), wire->to_x);
  asn_long2INTEGER(&(object->choice.wire.to_y), wire->to_y);
  asn_long2INTEGER(&(object->choice.wire.diameter), wire->diameter);
  asn_long2INTEGER(&(object->choice.wire.id), wire->id);

  OCTET_STRING_fromBuf(&object->choice.wire.name, wire->name, -1);
  asn_long2INTEGER(&(object->choice.wire.layer), ser_data->layer);
  
  if(RET_IS_NOT_OK(ret = lmodel_serialize_color(&(object->choice.wire.col1), wire->color1))) {
    debug(TM, "Can't serialize wire color");
    return ret;
  }
  if(RET_IS_NOT_OK(ret = lmodel_serialize_color(&(object->choice.wire.col2), wire->color2))) {
    debug(TM, "Can't serialize wire color");
    return ret;
  }

  if(wire->connections &&
     RET_IS_NOT_OK(ret = lmodel_serialize_connections(wire->connections, 
						      &object->choice.wire.connections.list)))
    return ret;

  ASN_SEQUENCE_ADD(&ser_data->file_content->list, object);
    
  return RET_OK;
}

ret_t lmodel_serialize_via_to_file(const lmodel_via_t * const via, lmodel_qtree_ser_data_t * ser_data) {
  ret_t ret;
  Object_t *object; 

  object = (Object_t *)calloc(1, sizeof(Object_t)); /* not malloc! */
  if(!object) return RET_ERR;
  object->present = Object_PR_via;
  asn_long2INTEGER(&(object->choice.via.x), via->x);
  asn_long2INTEGER(&(object->choice.via.y), via->y);
  asn_long2INTEGER(&(object->choice.via.diameter), via->diameter);
  asn_long2INTEGER(&(object->choice.via.id), via->id);

  OCTET_STRING_fromBuf(&object->choice.via.name, via->name, -1);

  if(RET_IS_NOT_OK(ret = lmodel_serialize_color(&(object->choice.via.col), via->color))) {
    debug(TM, "Can't serialize via color");
    return ret;
  }

  switch(via->direction) {
  case LM_VIA_UP: 
    asn_long2INTEGER(&(object->choice.via.direction), ViaDirection_up); 
    break;
  case LM_VIA_DOWN: 
    asn_long2INTEGER(&(object->choice.via.direction), ViaDirection_down); 
    break;
  default: return RET_ERR;
  }

  asn_long2INTEGER(&(object->choice.via.layer), ser_data->layer);

  if(via->connections && 
     RET_IS_NOT_OK(ret = lmodel_serialize_connections(via->connections, 
						      &object->choice.via.connections.list)))
    return ret;

  ASN_SEQUENCE_ADD(&ser_data->file_content->list, object);
    
  return RET_OK;
}

ret_t lmodel_serialize_gate_template_to_file(const lmodel_gate_template_t * const tmpl, 
					     FileContent_t * file_content) {
  ret_t ret;
  Object_t *object;
  lmodel_gate_template_port_t * gport_list;
  assert(tmpl);
  assert(file_content);
  if(!tmpl || !file_content) return RET_INV_PTR;

  object = (Object_t *)calloc(1, sizeof(Object_t)); /* not malloc! */
  if(!object) return RET_ERR;
  object->present = Object_PR_gate_template;

  asn_long2INTEGER(&(object->choice.gate_template.gate_id), tmpl->id);

  asn_long2INTEGER(&(object->choice.gate_template.master_image_min_x), tmpl->master_image_min_x);
  asn_long2INTEGER(&(object->choice.gate_template.master_image_min_y), tmpl->master_image_min_y);
  asn_long2INTEGER(&(object->choice.gate_template.master_image_max_x), tmpl->master_image_max_x);
  asn_long2INTEGER(&(object->choice.gate_template.master_image_max_y), tmpl->master_image_max_y);
  
  OCTET_STRING_fromBuf(&object->choice.gate_template.short_name, tmpl->short_name, -1);
  OCTET_STRING_fromBuf(&object->choice.gate_template.description, tmpl->description, -1);

  if(RET_IS_NOT_OK(ret = lmodel_serialize_color(&(object->choice.gate_template.frame_col), 
						tmpl->frame_color))) {
    debug(TM, "Can't serialize gate template's frame color");
    return ret;
  }
  if(RET_IS_NOT_OK(ret = lmodel_serialize_color(&(object->choice.gate_template.fill_col), 
						tmpl->fill_color))) {
    debug(TM, "Can't serialize gate template's fill color");
    return ret;
  }

  gport_list = tmpl->ports;
  while(gport_list != NULL) {

    GateTemplatePort_t * gport = (GateTemplatePort_t *) calloc(1, sizeof(GateTemplatePort_t));
    asn_long2INTEGER(&gport->id, gport_list->id); 
    asn_long2INTEGER(&gport->relative_x_coord, gport_list->relative_x_coord); 
    asn_long2INTEGER(&gport->relative_y_coord, gport_list->relative_y_coord); 
    asn_long2INTEGER(&gport->diameter, gport_list->diameter); 
    OCTET_STRING_fromBuf(&gport->port_name, gport_list->port_name, -1);
    if(gport_list->port_type == LM_PT_IN)
      asn_long2INTEGER(&gport->port_type, PortType_in);
    else
      asn_long2INTEGER(&gport->port_type, PortType_out);

    if(RET_IS_NOT_OK(ret = lmodel_serialize_color(&gport->col, gport_list->color))) {
      debug(TM, "Can't serialize gate template's port color");
      return ret;
    }

    ASN_SEQUENCE_ADD(&object->choice.gate_template.ports.list, gport);

    gport_list = gport_list->next;
  }


  ASN_SEQUENCE_ADD(&file_content->list, object);
  
  return RET_OK;
}


ret_t cb_serialize_objects(quadtree_t * qtree, lmodel_qtree_ser_data_t * ser_data) {

  quadtree_object_t * object;
  ret_t ret;

  assert(qtree);
  assert(ser_data);
  if(!qtree || !ser_data) return RET_INV_PTR;

  object = qtree->objects;
  
  while(object != NULL) {
    switch(object->object_type) {
    case LM_TYPE_GATE:
      if(RET_IS_NOT_OK(ret = lmodel_serialize_gate_to_file((lmodel_gate_t *) object->object, ser_data))) return ret;
      break;
    case LM_TYPE_WIRE:
      if(RET_IS_NOT_OK(ret = lmodel_serialize_wire_to_file((lmodel_wire_t *) object->object, ser_data))) return ret;
      break;
    case LM_TYPE_VIA:
      if(RET_IS_NOT_OK(ret = lmodel_serialize_via_to_file((lmodel_via_t *) object->object, ser_data))) return ret;
      break;
    default:
      debug(TM, "unknown object type");
      return RET_ERR;
    }
    object = object->next;
  }
  return RET_OK;
}

ret_t lmodel_serialize_to_file(const logic_model_t * const lmodel, const char * const project_dir) {
  int layer;
  lmodel_gate_template_set_t * gset;
  quadtree_traverse_func_t cb_func = (quadtree_traverse_func_t) &cb_serialize_objects;  
  FILE * fh = NULL;
  char fq_filename[PATH_MAX];
  char fq_filename_tmp[PATH_MAX];
  FileContent_t * file_content;
  asn_enc_rval_t ec;      /* Encoder return value  */
  ret_t ret;

  assert(lmodel != NULL);
  if(lmodel == NULL) return RET_INV_PTR;

  snprintf(fq_filename_tmp, PATH_MAX, "%s/lmodel.tmp", project_dir);
  snprintf(fq_filename, PATH_MAX, "%s/lmodel", project_dir);

  debug(TM, "writing logic model data to %s", fq_filename_tmp);
  if((fh = fopen(fq_filename_tmp, "w+")) == NULL) {
    debug(TM, "Can't write file %s", fq_filename_tmp);
    return RET_ERR;
  }

  file_content = (FileContent_t *)calloc(1, sizeof(FileContent_t)); /* not malloc! */
  if(!file_content) {
    debug(TM, "calloc() failed");
    fclose(fh);
    return RET_ERR;
  }

  gset = lmodel->gate_template_set;
  while(gset != NULL) {
    if(RET_IS_NOT_OK(ret = lmodel_serialize_gate_template_to_file(gset->gate, file_content))) {
      asn_DEF_FileContent.free_struct(&asn_DEF_FileContent, file_content, 0);
      debug(TM, "Can't serialize gate template set");
      fclose(fh);
      return ret;
    }
    else gset = gset->next;
  }

  for(layer = 0; layer < lmodel->num_layers; layer++) {
    lmodel_qtree_ser_data_t ser_data = {file_content, layer};
    quadtree_traverse_complete(lmodel->root[layer], cb_func, &ser_data);
  }


  ec = der_encode(&asn_DEF_FileContent, file_content, write_out, fh);
  if(ec.encoded == -1) {
    fprintf(stderr, "Could not encode (at %s)\n",
	    ec.failed_type ? ec.failed_type->name : "unknown");
    return RET_ERR;
  }
  
  asn_DEF_FileContent.free_struct(&asn_DEF_FileContent, file_content, 0);
  
  fclose(fh);

  if(rename(fq_filename_tmp, fq_filename) == -1) return RET_ERR;

  return RET_OK;
}

ret_t lmodel_gate_template_add_port(logic_model_t * lmodel,
				    lmodel_gate_template_t * const tmpl, lmodel_gate_template_port_t * port) {
  ret_t ret;
  assert(tmpl);
  assert(port);
  if(!tmpl || !port) return RET_INV_PTR;

  if(!tmpl->ports)  tmpl->ports = port;
  else {
    lmodel_gate_template_port_t * ptr = tmpl->ports;
    while(ptr->next) ptr = ptr->next;
    ptr->next = port;
  }

  if(RET_IS_NOT_OK(ret = lmodel_update_all_gate_ports(lmodel, tmpl))) return ret;

  return RET_OK;
}

/**
 * Get the number of defined ports for a gate template
 */
int lmodel_gate_template_get_num_ports(const lmodel_gate_template_t * const tmpl) {
  int counter = 0;
  assert(tmpl != NULL);
  if(tmpl == NULL) return 0;

  lmodel_gate_template_port_t * ptr = tmpl->ports;
  while(ptr) {
    counter++;
    ptr = ptr->next;
  }
  return counter;
}

ret_t lmodel_gate_template_set_port(logic_model_t * lmodel,
				    lmodel_gate_template_t * const tmpl, 
				    unsigned int id, const char * const port_name, 
				    LM_PORT_TYPE port_type) {
  lmodel_gate_template_port_t * ptr;
  assert(tmpl);
  if(!tmpl) return RET_INV_PTR;
  ptr = tmpl->ports;
  while(ptr) {
    if(ptr->id == id) { // change data
      if(ptr->port_name) free(ptr->port_name);
      ptr->port_name = strdup(port_name);
      ptr->port_type = port_type;
      return RET_OK;
    }
    ptr = ptr->next;
  }
  
  if((ptr = lmodel_create_gate_template_port()) == NULL) return RET_ERR;
  ptr->id = id;
  ptr->port_name = strdup(port_name);
  ptr->port_type = port_type;
  return lmodel_gate_template_add_port(lmodel, tmpl, ptr);
}


connection_build_helper_t * lmodel_create_connection_build_helper(LM_OBJECT_TYPE object_type, 
								  void * obj, 
								  unsigned int from_sub_port) {
  connection_build_helper_t * bh;
  assert(obj);
  if(!obj) return NULL;

  if((bh = (connection_build_helper_t *)malloc(sizeof(connection_build_helper_t))) == NULL)
    return NULL;

  memset(bh, 0, sizeof(connection_build_helper_t));
  bh->object_type = object_type;
  bh->obj_ptr = obj;
  bh->from_sub_port = from_sub_port;
  return bh;
}

ret_t lmodel_destroy_id_list(id_list_t * list) {
  id_list_t * ptr, *ptr_next;
  assert(list);
  if(!list) return RET_INV_PTR;
  
  ptr = list;
  while(ptr) {
    ptr_next = ptr->next;
    free(ptr);
    ptr = ptr_next;
  }
  return RET_OK;
}

id_list_t * lmodel_import_connections(LogicModelConnection_t ** list, int count) {
  int i;
  id_list_t * l = NULL, * n = NULL;

  //  assert(list);
  if(!list) return NULL;

  debug(TM, "object has %d adjacent objects", count);

  for(i = 0; i < count; i++) {
    assert(list[i]);

    if(list[i]) {
      long oid, sub_id;
      if(asn_INTEGER2long(&list[i]->object_id, &oid) == -1 ||
	 asn_INTEGER2long(&list[i]->sub_id, &sub_id) == -1) {
	lmodel_destroy_id_list(l);
	return NULL;
      }
      
      if(( n = (id_list_t *)malloc(sizeof(id_list_t))) == NULL) {
	lmodel_destroy_id_list(l);
	free(n);
	return NULL;
      }
      memset(n, 0, sizeof(id_list_t));

      n->id = oid;
      n->sub_id = sub_id;
      if(l != NULL) n->next = l;
      l = n;
    }
  }
  

  return l;
}

ret_t lmodel_add_connection_build_helper(logic_model_t * lmodel, connection_build_helper_t * bh) {
  assert(lmodel);
  assert(bh);
  if(!lmodel || !bh) return RET_INV_PTR;

  bh->next = lmodel->conn_build_helper;
  lmodel->conn_build_helper = bh;

  return RET_OK;
}

ret_t lmodel_import_color(Color_t * col, color_t * out_col) {
  
  *out_col = MERGE_CHANNELS((col->red & 0xff), 
			    (col->green & 0xff), 
			    (col->blue & 0xff), 
			    (col->alpha & 0xff));
  
  return RET_OK;
}

ret_t lmodel_import_wire(logic_model_t * lmodel, Wire_t * wire, unsigned int * highest_object_id) {
  long from_x, from_y, to_x, to_y, diameter, obj_id, layer;
  color_t col1, col2;
  ret_t ret;
  assert(lmodel != NULL);
  assert(wire != NULL);
  assert(highest_object_id != NULL);

  if(!lmodel || !wire) return RET_INV_PTR;

  if(asn_INTEGER2long(&wire->from_x, &from_x) != -1 &&
     asn_INTEGER2long(&wire->from_y, &from_y) != -1 &&
     asn_INTEGER2long(&wire->to_x, &to_x) != -1 &&
     asn_INTEGER2long(&wire->to_y, &to_y) != -1 &&
     asn_INTEGER2long(&wire->diameter, &diameter) != -1 &&
     asn_INTEGER2long(&wire->id, &obj_id) != -1 &&
     asn_INTEGER2long(&wire->layer, &layer) != -1 &&
     RET_IS_OK(lmodel_import_color(&wire->col1, &col1)) &&
     RET_IS_OK(lmodel_import_color(&wire->col2, &col2)) ) {

    if((unsigned long)obj_id > *highest_object_id) *highest_object_id = obj_id;

    lmodel_wire_t * new_wire = lmodel_create_wire(lmodel, from_x, from_y, to_x, to_y, 
						  diameter, 
						  strdup((char *)wire->name.buf),
						  obj_id);

    if(!new_wire) return RET_ERR;
    new_wire->color1 = col1;
    new_wire->color2 = col2;

    if(RET_IS_NOT_OK(ret = lmodel_add_wire(lmodel, layer, new_wire))) return ret;
    
    connection_build_helper_t * bh = lmodel_create_connection_build_helper(LM_TYPE_WIRE, new_wire, 0);
    debug(TM, "wire has %d connections", wire->connections.list.count);
    bh->to = lmodel_import_connections(wire->connections.list.array, wire->connections.list.count);
    lmodel_add_connection_build_helper(lmodel, bh);

  }
  else {
    debug(TM, "Can't decode wire object");
    return RET_ERR;
  }

  return RET_OK;
}

ret_t lmodel_import_via(logic_model_t * lmodel, Via_t * via, unsigned int * highest_object_id) {
  long x, y, diameter, obj_id, direction, layer;
  color_t col;
  ret_t ret;
  assert(lmodel != NULL);
  assert(via != NULL);
  assert(highest_object_id != NULL);

  if(!lmodel || !via) return RET_INV_PTR;

  if(asn_INTEGER2long(&via->x, &x) != -1 &&
     asn_INTEGER2long(&via->y, &y) != -1 &&
     asn_INTEGER2long(&via->diameter, &diameter) != -1 &&
     asn_INTEGER2long(&via->direction, &direction) != -1 &&
     asn_INTEGER2long(&via->id, &obj_id) != -1 &&
     asn_INTEGER2long(&via->layer, &layer) != -1 &&
     RET_IS_OK(lmodel_import_color(&via->col, &col))) {

    if((unsigned long)obj_id > *highest_object_id) *highest_object_id = obj_id;
    
    LM_VIA_DIR dir;
    switch(direction) {
    case ViaDirection_up: dir = LM_VIA_UP; break;
    case ViaDirection_down: dir = LM_VIA_DOWN; break;
    default: return RET_ERR;
    }
    
    lmodel_via_t * new_via = lmodel_create_via(lmodel, x, y, dir, diameter, 
					       strdup((char *)via->name.buf),
					       obj_id);
    if(!new_via) return RET_ERR;
    new_via->color = col;
    if(RET_IS_NOT_OK(ret = lmodel_add_via(lmodel, layer, new_via))) return ret;

    connection_build_helper_t * bh = lmodel_create_connection_build_helper(LM_TYPE_VIA, new_via, 0);
    bh->to = lmodel_import_connections(via->connections.list.array, via->connections.list.count);
    lmodel_add_connection_build_helper(lmodel, bh);

  }
  else {
    debug(TM, "Can't decode via object");
    return RET_ERR;
  }

  return RET_OK;
}

lmodel_gate_port_t * lmodel_create_gate_port(lmodel_gate_t * gate, unsigned int port_id) {
  lmodel_gate_port_t * port;
  assert(gate);
  if(!gate) return NULL;

  if((port = (lmodel_gate_port_t * )malloc(sizeof(lmodel_gate_port_t))) == NULL) return NULL;
  memset(port, 0, sizeof(lmodel_gate_port_t));
  port->port_id = port_id;
  port->gate = gate;

  /*
  if(RET_IS_NOT_OK(lmodel_update_gate_ports(gate))) {
    free(port);
    return NULL;
  }
  */
  return port;
}

/**
 * Add an existing gate port to a gate.
 */

ret_t lmodel_add_gate_port_to_gate(lmodel_gate_t * gate, lmodel_gate_port_t * port) {
  assert(gate != NULL);
  assert(port != NULL);
  if(gate == NULL || port == NULL) return RET_INV_PTR;

  port->next = gate->ports;
  gate->ports = port;

  return RET_OK;
}


/** 
 * Update data structures for all gates from logic model with the type referrenced by parameter tmpl.
 */
ret_t lmodel_update_all_gate_ports(logic_model_t * lmodel, lmodel_gate_template_t * tmpl) {
  ret_t ret;
  assert(lmodel != NULL);
  assert(tmpl != NULL);

  if(lmodel == NULL || tmpl == NULL) return RET_INV_PTR;

  //debug(TM, "port update on %s", tmpl->short_name);

  lmodel_gate_set_t * gset = lmodel->gate_set;
  while(gset != NULL) {
    //debug(TM, "iterating over gate set");
    if(gset->gate != NULL && gset->gate->gate_template != NULL &&
       gset->gate->gate_template == tmpl) {
      //debug(TM, "there is a gate with a template");
      if(RET_IS_NOT_OK(ret = lmodel_update_gate_ports(gset->gate))) return ret;
    }
    /*else
      debug(TM, "can't update gate"); */

    gset = gset->next;
  }
  return RET_OK;
}


/**
 * Update data structures for gate ports.
 *
 * Gate templates have template ports. Template ports define attributes, that are common to
 * all real ports (of that port type and for all instances of that gate type). If you make
 * changes to template ports, e.g. add a port to a template, it is
 * necessary to update data structure from gates.
 */
ret_t lmodel_update_gate_ports(lmodel_gate_t * gate) {
  ret_t ret;

  assert(gate != NULL);
  if(gate == NULL) return RET_INV_PTR;
  
  lmodel_gate_template_t * gate_tmpl = gate->gate_template;
  lmodel_gate_port_t * port_ptr = gate->ports;

  debug(TM, "update gate ports for %s", 
	gate->gate_template != NULL ? gate->gate_template->short_name : "unid gate");

  if(gate_tmpl == NULL) {
    // no template, but defined ports -> destroy ports
    if(port_ptr != NULL) {
      if(RET_IS_NOT_OK(ret = lmodel_destroy_gate_ports(gate->ports))) return ret;
      gate->ports = NULL;
    }
  }
  else { // template is defined
    
    // ports defined
    if(port_ptr != NULL) {

      lmodel_gate_template_port_t * tmpl_port_ptr = gate_tmpl->ports;
      while(tmpl_port_ptr != NULL) { // iterate over template's ports
	// check if there is a corresponding port in our gate
	lmodel_gate_port_t * present_gport = lmodel_get_port_from_gate_by_id(gate, tmpl_port_ptr->id);
	if(present_gport == NULL) {
	  // create a new gate port
	  debug(TM, "there are ports, but curr port ist undef: add port %d", tmpl_port_ptr->id);
	  lmodel_gate_port_t * new_gate_port = lmodel_create_gate_port(gate, tmpl_port_ptr);
	  assert(new_gate_port);
	  if(new_gate_port == NULL) return RET_MALLOC_FAILED;
	  if(RET_IS_NOT_OK(ret = lmodel_add_gate_port_to_gate(gate, new_gate_port))) return ret;
	}
	else {
	  // port available in gate
	  present_gport->tmpl_port = tmpl_port_ptr;
	  //assert(present_gport->tmpl_port != NULL);
	}
	tmpl_port_ptr = tmpl_port_ptr->next;
      }

    }
    else {
      // ports undefined
      lmodel_gate_template_port_t * tmpl_port_ptr = gate_tmpl->ports;
      while(tmpl_port_ptr != NULL) {
	debug(TM, "ports undefined: add port %d", tmpl_port_ptr->id);
	lmodel_gate_port_t * new_gate_port = lmodel_create_gate_port(gate, tmpl_port_ptr);
	assert(new_gate_port);
	if(new_gate_port == NULL) return RET_MALLOC_FAILED;
	if(RET_IS_NOT_OK(ret = lmodel_add_gate_port_to_gate(gate, new_gate_port))) return ret;
	tmpl_port_ptr = tmpl_port_ptr->next;
      }
    }
  }
  
  //port_ptr = gate->ports;
  //while(port_ptr != NULL) {
    //debug(TM, "gate has port with id %d", port_ptr->port_id);
    //port_ptr = port_ptr->next;
  //}
  return RET_OK;
}


/**
 * Create a gate port. The port is not added to the gate!
 * @see lmodel_add_gate_port()
 */
lmodel_gate_port_t * lmodel_create_gate_port(lmodel_gate_t * gate, lmodel_gate_template_port_t * tmpl_port) {
  lmodel_gate_port_t * gport = NULL;
  if((gport = (lmodel_gate_port_t *) malloc(sizeof(lmodel_gate_port_t))) == NULL)
    return NULL;
  
  memset(gport, 0, sizeof(lmodel_gate_port_t));

  gport->port_id = tmpl_port->id;
  gport->gate = gate;
  gport->tmpl_port = tmpl_port;
  
  return gport;
}

ret_t lmodel_import_gate(logic_model_t * lmodel, Gate_t * gate, unsigned int * highest_object_id) {
  long min_x, min_y, max_x, max_y, gate_id, obj_id, layer, master_orientation;
  ret_t ret;
  int port_i;
  assert(lmodel != NULL);
  assert(gate != NULL);
  assert(highest_object_id != NULL);

  if(!lmodel || !gate) return RET_INV_PTR;

  if(asn_INTEGER2long(&gate->min_x, &min_x) != -1 &&
     asn_INTEGER2long(&gate->min_y, &min_y) != -1 &&
     asn_INTEGER2long(&gate->max_x, &max_x) != -1 &&
     asn_INTEGER2long(&gate->max_y, &max_y) != -1 &&
     asn_INTEGER2long(&gate->gate_id, &gate_id) != -1 &&
     asn_INTEGER2long(&gate->id, &obj_id) != -1 &&
     asn_INTEGER2long(&gate->layer, &layer) != -1 &&
     asn_INTEGER2long(&gate->master_orientation, &master_orientation) != -1) {

    if((unsigned long)obj_id > *highest_object_id) *highest_object_id = obj_id;
    
    lmodel_gate_template_t * tmpl = lmodel_get_gate_template_by_id(lmodel, gate_id);

    LM_TEMPLATE_ORIENTATION template_orientation;

    switch(master_orientation) {
    case TemplateOrientationType_undefined: 
      template_orientation = LM_TEMPLATE_ORIENTATION_UNDEFINED; 
      break;
    case TemplateOrientationType_normal: 
      template_orientation = LM_TEMPLATE_ORIENTATION_NORMAL; 
      break;
    case TemplateOrientationType_flipped_up_down: 
      template_orientation = LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN; 
      break;
    case TemplateOrientationType_flipped_left_right: 
      template_orientation = LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT; 
      break;
    case TemplateOrientationType_flipped_both: 
      template_orientation = LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH; 
      break;
    default: return RET_ERR;
    }

    //debug(TM, "Create a gate");
    lmodel_gate_t * new_gate = lmodel_create_gate(lmodel, min_x, min_y, max_x, max_y, 
						  tmpl, 
						  strdup((char *)gate->name.buf),
						  obj_id);
    if(!new_gate) return RET_ERR;
    new_gate->template_orientation = template_orientation;


    for(port_i = 0; port_i < gate->ports.list.count; port_i++) {
      long port_id;
      GatePort_t * gport = gate->ports.list.array[port_i];
      asn_INTEGER2long(&gport->port_id, &port_id);

      //debug(TM, "Parsing port of gate %d with ID %d.", obj_id, port_id);

      lmodel_gate_port_t * is_port_present = lmodel_get_port_from_gate_by_id(new_gate, port_id);
      if(is_port_present != NULL) {
	debug(TM, "There is a problem in the logic model. Port %d from object %d is"
	      "defined twice. We can't ignore this.", port_id, obj_id);
	return RET_ERR;
      }
      else {
	lmodel_gate_port_t * new_gate_port = lmodel_create_gate_port(new_gate, port_id);
	if(RET_IS_NOT_OK(ret = lmodel_add_gate_port_to_gate(new_gate, new_gate_port))) return ret;

	connection_build_helper_t * bh = lmodel_create_connection_build_helper(LM_TYPE_GATE_PORT, 
									       new_gate_port, 
									       port_id);
	bh->to = lmodel_import_connections(gport->connections.list.array, 
					   gport->connections.list.count);
	lmodel_add_connection_build_helper(lmodel, bh);
      }
    }
    //debug(TM, "Add gate to lmodel");
    if(RET_IS_NOT_OK(ret = lmodel_add_gate(lmodel, layer, new_gate))) return ret;

    //if(RET_IS_NOT_OK(ret = lmodel_update_all_gate_ports(lmodel, tmpl))) return ret;


  }
  else {
    debug(TM, "Can't decode gate object");
    return RET_ERR;
  }
  
  return RET_OK;
}

ret_t lmodel_import_gate_template(logic_model_t * lmodel, GateTemplate_t * gate_template) {
  long gate_id, min_x, min_y, max_x, max_y;
  color_t col1, col2;
  ret_t ret;

  assert(lmodel != NULL);
  assert(gate_template != NULL);
  if(!lmodel || !gate_template) return RET_INV_PTR;

  if(asn_INTEGER2long(&gate_template->master_image_min_x, &min_x) != -1 &&
     asn_INTEGER2long(&gate_template->master_image_min_y, &min_y) != -1 &&
     asn_INTEGER2long(&gate_template->master_image_max_x, &max_x) != -1 &&
     asn_INTEGER2long(&gate_template->master_image_max_y, &max_y) != -1 &&
     asn_INTEGER2long(&gate_template->gate_id, &gate_id) != -1 &&
     RET_IS_OK(lmodel_import_color(&gate_template->frame_col, &col1)) && 
     RET_IS_OK(lmodel_import_color(&gate_template->fill_col, &col2))) {
    
    int j;
    lmodel_gate_template_t * tmpl = lmodel_create_gate_template();
    assert(tmpl);
    if(!tmpl) return RET_ERR;
    tmpl->frame_color = col1;
    tmpl->fill_color = col2;

    if(RET_IS_NOT_OK(ret = lmodel_gate_template_set_master_region(tmpl, 
								  min_x, min_y, 
								  max_x, max_y))) return ret;
    if(RET_IS_NOT_OK(ret = lmodel_gate_template_set_text(tmpl,
							 (const char *)gate_template->short_name.buf,
							 (const char *)gate_template->description.buf))) 
      return ret;
    
    
    for(j = 0 ; j < gate_template->ports.list.count; j++) {
      // get gate_template_port
      long id, port_type, x_coord, y_coord, diameter;
      GateTemplatePort_t * encoded_port = gate_template->ports.list.array[j];
      if(!encoded_port) return RET_ERR;
      
      lmodel_gate_template_port_t * gport = lmodel_create_gate_template_port();
      asn_INTEGER2long(&encoded_port->id, &id);
      asn_INTEGER2long(&encoded_port->relative_x_coord, &x_coord);
      asn_INTEGER2long(&encoded_port->relative_y_coord, &y_coord);
      asn_INTEGER2long(&encoded_port->diameter, &diameter);
      asn_INTEGER2long(&encoded_port->port_type, &port_type);
 
      lmodel_import_color(&encoded_port->col, &col1);

      gport->id = id;
      gport->relative_x_coord = x_coord;
      gport->relative_y_coord = y_coord;
      gport->diameter = diameter;
      gport->color = col1;
      gport->port_name = strdup((char *)encoded_port->port_name.buf);
      gport->port_type =  port_type == PortType_in ? LM_PT_IN : LM_PT_OUT;
      
      //debug(TM, "port name : %s", gport->port_name);
      if(RET_IS_NOT_OK(ret = lmodel_gate_template_add_port(lmodel, tmpl, gport))) {
	debug(TM, "gate_template_add_inport()");
	return ret;
      }
    }
    
    if(RET_IS_NOT_OK(ret = lmodel_add_gate_template(lmodel, tmpl, gate_id))) {
      debug(TM, "lmodel_add_gate_template() failed");
      return ret;
    }
  }
  else {
    debug(TM, "Can't decode gate template object");
    return RET_ERR;
  }


  return RET_OK;
}

bool lmodel_check_collision_via_via(lmodel_via_t * v1, lmodel_via_t * v2) {

  assert(v1);
  assert(v2);
  assert(v1 != v2); // should not happen
  if(v1 == v2) return false;

  double dx = (double)v2->x - (double)v1->x;
  double dy = (double)v2->y - (double)v1->y;
  if(sqrt(dx*dx + dy*dy) <= (v1->diameter + v2->diameter) / 2.0) {
    debug(TM, "via-via-collision");
    return true;
  }
  else {
    debug(TM, "no via-via-collision");
    return false;
  }
}

/** Check, if two wires intersect.
 * @return It returns true, if wires intersect, else it returns false. If w1 and w2 point to the same object,
 * the result is false;
 */

bool lmodel_check_collision_wire_wire(lmodel_wire_t * w1, lmodel_wire_t * w2) {
  assert(w1);
  assert(w2);
  double m1, n1, m2, n2;
  ret_t ret1, ret2;
  if(!w1 || !w2 || w1 == w2) return false;


  ret1 = get_line_function_for_wire(w1, &m1, &n1);
  ret2 = get_line_function_for_wire(w2, &m2, &n2);

  if(ret1 == RET_MATH_ERR || ret2 == RET_MATH_ERR) {
    /* W1 or w2 or both wires are parallel to y axis. 
       A linerar function cannot be calculated.
       
       In case a line is parallel to either x or y axis, the lines
       bounding box is the same as the line object itself. So we
       can check, if bounding boxes intersect.
    */
    unsigned int larger_min_x = MAX(MIN(w1->from_x, w1->to_x), MIN(w2->from_x, w2->to_x));
    unsigned int lower_max_x  = MIN(MAX(w1->from_x, w1->to_x), MAX(w2->from_x, w2->to_x));
    unsigned int larger_min_y = MAX(MIN(w1->from_y, w1->to_y), MIN(w2->from_y, w2->to_y));
    unsigned int lower_max_y  = MIN(MAX(w1->from_y, w1->to_y), MAX(w2->from_y, w2->to_y));
    
    if(larger_min_x < lower_max_x && larger_min_y < lower_max_y) {
      debug(TM, "wire %d and wire %d intersect", w1->id, w2->id);
      return true;
    }
    else {
      debug(TM, "they don't intersect");
      return false;
    }
  }

  // ~~ parallel ?
  if(fabs(m1 - m2) < EPSILON) {

    if((fabs( /* y = f(from_x) = */ m1 * (double) w2->from_x + n1 - w2->from_y) < EPSILON &&
	w2->from_x > MIN(w1->from_x, w1->to_x) &&  w2->from_x <  MAX(w1->from_x, w1->to_x))
       ||
       (fabs( /* y = f(to_x  ) = */ m1 * (double) w2->to_x + n1 - w2->to_y) < EPSILON &&
	w2->to_x > MIN(w1->from_x, w1->to_x) &&  w2->to_x <  MAX(w1->from_x, w1->to_x))	) {

      debug(TM, "wire %d and wire %d intersect (parallel)", w1->id, w2->id);
      return true;
    }
    else {
      debug(TM, "they don't intersect");
      return false;
    }
  }


  double xi = - (n1 - n2) / (m1 - m2);
  double yi = n1 + m1 * xi;

  if( ((double)w1->from_x - xi)*(xi - (double)w1->to_x) >= 0 && 
      ((double)w2->from_x - xi)*(xi - (double)w2->to_x) >= 0 && 
      ((double)w1->from_y - yi)*(yi - (double)w1->to_y) >= 0 && 
      ((double)w2->from_y - yi)*(yi - (double)w2->to_y) >= 0) {
    debug(TM, "wire %d and wire %d intersect at %f,%f", w1->id, w2->id, xi, yi);
    return true;
  }
  else {
    debug(TM, "they don't intersect");
    return false;
  }
  
}


bool lmodel_check_collision_wire_via(lmodel_wire_t * wire, lmodel_via_t * v) {
  assert(wire);
  assert(v);
  ret_t ret;
  double m, n;

  if(!wire || !v) return RET_INV_PTR;

  ret = get_line_function_for_wire(wire, &m, &n);

  if(ret == RET_MATH_ERR) {
    if(v->x > MIN(wire->from_x, wire->to_x) &&
       v->x < MAX(wire->from_x, wire->to_x) &&
       v->y > MIN(wire->from_y, wire->to_y) &&
       v->y < MAX(wire->from_y, wire->to_y)) {
      debug(TM, "wire-via collision");
      return true;
    }
    else
      return false;
  }
  else {
    if(fabs( m * v->x + n - v->y) < v->diameter &&
       v->x > MIN(wire->from_x, wire->to_x) &&
       v->x < MAX(wire->from_x, wire->to_x)) {
      debug(TM, "wire-via collision");
      return true;
    }
    else return false;
  }
}

bool lmodel_check_collision(LM_OBJECT_TYPE type1, void * obj1,
			    LM_OBJECT_TYPE type2, void * obj2) {
  assert(obj1);
  assert(obj2);
  assert(obj1 != obj2); // this should not happen
  assert(type1 != LM_TYPE_UNDEF);
  assert(type2 != LM_TYPE_UNDEF);

  if(type1 == LM_TYPE_WIRE) {
    if(type2 == LM_TYPE_WIRE) {
      debug(TM, "\twire - wire");
      return lmodel_check_collision_wire_wire((lmodel_wire_t *) obj1, (lmodel_wire_t *) obj2);
    }
    else if(type2 == LM_TYPE_VIA) {
      debug(TM, "\twire - via");
      return lmodel_check_collision_wire_via((lmodel_wire_t *) obj1, (lmodel_via_t *) obj2);
    }
  }
  else if(type1 == LM_TYPE_VIA) {
    if(type2 == LM_TYPE_WIRE) {
      return lmodel_check_collision_wire_via((lmodel_wire_t *) obj1, (lmodel_via_t *) obj2);
    }
    else if(type2 == LM_TYPE_VIA)
      return lmodel_check_collision_via_via((lmodel_via_t *) obj1, (lmodel_via_t *) obj2);
  }

  return false;
}

ret_t lmodel_connect_objects(LM_OBJECT_TYPE type1, void * obj1,
			     LM_OBJECT_TYPE type2, void * obj2) {
  assert(obj1 != NULL);
  assert(obj2 != NULL);
  assert(type1 != LM_TYPE_UNDEF);
  assert(type2 != LM_TYPE_UNDEF);
  assert(type1 != LM_TYPE_GATE);
  assert(type2 != LM_TYPE_GATE);
  ret_t ret;

  debug(TM, "\tconnect object %p and %p", obj1, obj2);
  if(obj1 == obj2) return RET_OK;

  lmodel_connection_t * list1_ptr = lmodel_get_connections_from_object(type1, obj1);
  lmodel_connection_t * list2_ptr = lmodel_get_connections_from_object(type2, obj2);

  lmodel_connection_t * new_list = NULL;
  lmodel_connection_t * ptr = NULL;

  if(RET_IS_NOT_OK(ret = lmodel_connect_object(&new_list, type1, (object_ptr_t *)obj1))) return ret;
  if(RET_IS_NOT_OK(ret = lmodel_connect_object(&new_list, type2, (object_ptr_t *)obj2))) return ret;

  /* merge lists */
  ptr = list1_ptr;
  while(ptr != NULL) {
    assert(ptr->obj_ptr != NULL);
    if(RET_IS_NOT_OK(ret = lmodel_connect_object(&new_list, ptr->object_type, 
						 (object_ptr_t *)ptr->obj_ptr))) return ret;
    ptr = ptr->next;
  }

  ptr = list2_ptr;
  while(ptr != NULL) {
    assert(ptr->obj_ptr != NULL);
    if(RET_IS_NOT_OK(ret = lmodel_connect_object(&new_list, ptr->object_type, 
						 (object_ptr_t *)ptr->obj_ptr))) return ret;
    ptr = ptr->next;
  }
  
  /* update object from list*/
  ptr = new_list;
  while(ptr != NULL) {

    assert(ptr->obj_ptr != NULL);
    
    lmodel_connection_t * old_list = lmodel_get_connections_from_object(ptr->object_type, ptr->obj_ptr);
    if(old_list != NULL) {

      if(RET_IS_NOT_OK(ret = lmodel_destroy_connections(old_list))) {
	debug(TM, "\tcan't destroy connections");
	return ret;
      }
      // prevent double-free: find objects with same commnection list and set there a NULL
      lmodel_connection_t * ptr2 = ptr->next;
      while(ptr2 != NULL) {
	lmodel_connection_t * old_list2 = lmodel_get_connections_from_object(ptr->object_type, ptr->obj_ptr);
	if(old_list2 == old_list) {
	  if(RET_IS_NOT_OK(ret = lmodel_set_connections_for_object(ptr2->object_type, ptr2->obj_ptr, NULL))) {
	    debug(TM, "error: can't set connections");
	    return ret;
	  }
	}
	ptr2 = ptr2->next;
      }
    }
    
    if(RET_IS_NOT_OK(ret = lmodel_set_connections_for_object(ptr->object_type, ptr->obj_ptr, new_list))) {
      debug(TM, "error: can't set connections");
      return ret;
    }
    ptr = ptr->next;
  }
  
  return RET_OK;
}

/**
 * Create and add a new connection entry to a connection list. The function checks, if object obj
 * is already connected.
 * @returns It returns RET_OK, if obj is already connected or if it was sucessfully connected.
 */
ret_t lmodel_connect_object(lmodel_connection_t ** conn_list, LM_OBJECT_TYPE object_type, object_ptr_t * obj) {
  lmodel_connection_t * ptr;

  assert(conn_list != NULL);
  assert(obj != NULL);
  if(conn_list == NULL || obj == NULL) return RET_INV_PTR;

  if(lmodel_find_connection_to_object(*conn_list, obj) == NULL) {

    if((ptr = (lmodel_connection_t *)malloc(sizeof(lmodel_connection_t))) == NULL)
      return RET_MALLOC_FAILED;
    
    memset(ptr, 0, sizeof(lmodel_connection_t));

    ptr->obj_ptr = obj;
    ptr->object_type = object_type;
    ptr->next = *conn_list;

    *conn_list = ptr;    
  }

  return RET_OK;
}

/**
 * Check if there is a monodirectional connection from from_conn to object obj.
 * @returns Function returns NULL if there is no connection or if a parameter is an invalid pointer.
 *          Else the connection object is returned.
 */
lmodel_connection_t * lmodel_find_connection_to_object(lmodel_connection_t * from_conn, object_ptr_t * obj) {
  assert(obj);
  if(from_conn == NULL || obj == NULL) return NULL;

  lmodel_connection_t * i = from_conn;
  while(i != NULL) {
    if(i->obj_ptr == obj) return i;
    i = i->next;
  }

  return NULL;
}

lmodel_gate_port_t * lmodel_get_port_from_gate_by_id(lmodel_gate_t * gate, unsigned int port_id) {
  assert(gate);
  if(!gate || !gate->ports) return NULL;
  lmodel_gate_port_t * ptr = gate->ports;
  while(ptr) {
    if(ptr->port_id == port_id) return ptr;
    ptr = ptr->next;
  }
  return NULL;
}

int cb_check_object_id(quadtree_object_t * qobj, lmodel_lookup_object_t * data_ptr) {
  assert(qobj);
  assert(qobj->object);
  assert(data_ptr);

  if(qobj->object_type == LM_TYPE_GATE) {
    lmodel_gate_t * gate = (lmodel_gate_t *)(qobj->object);
    if(gate->id == data_ptr->object_id) {

      lmodel_gate_port_t * port_ptr = lmodel_get_port_from_gate_by_id(gate, data_ptr->sub_id);
      if(port_ptr) {
	data_ptr->object = port_ptr;
	data_ptr->object_type = LM_TYPE_GATE_PORT;
	return 1;
      }

    }
  }
  else if(qobj->object_type == LM_TYPE_WIRE) {
    if(((lmodel_wire_t *)(qobj->object))->id == data_ptr->object_id) {
      data_ptr->object = qobj->object;
      data_ptr->object_type = LM_TYPE_WIRE;
      return 1;
    }
  }
  else if(qobj->object_type == LM_TYPE_VIA) {
    if(((lmodel_via_t *)(qobj->object))->id == data_ptr->object_id) {
      data_ptr->object = qobj->object;
      data_ptr->object_type = LM_TYPE_VIA;
      return 1;
    }
  }
  return 0;
}


/**
 * Function looks up objects in the quadtree by object id and
 * in case of gates where 'sub objects' are present, by its sub id.
 * It does not use an index. So it might be slow for large object sets.
 */
ret_t lmodel_get_object_by_id(const logic_model_t * const lmodel,
			      unsigned int object_id, unsigned int sub_id,
			      void ** o_ptr_out, LM_OBJECT_TYPE * obj_type_out, unsigned int * layer_out) {

  int i;
  quadtree_object_t * optr;
  lmodel_lookup_object_t data = {object_id, sub_id, LM_TYPE_UNDEF, NULL};
  assert(lmodel);
  if(!lmodel) return RET_INV_PTR;

  for(i=0; i < lmodel->num_layers; i++) {
    if((optr = quadtree_find_object(lmodel->root[i],
				    (quadobject_traverse_func_t) &cb_check_object_id, &data))) {
      if(obj_type_out) *obj_type_out = data.object_type;
      if(o_ptr_out) *o_ptr_out = data.object;
      if(layer_out != NULL) *layer_out = i;
      return RET_OK;
    }
  }
  return RET_OK;
}


int cb_check_object_by_ptr(quadtree_object_t * qobj, lmodel_lookup_object_t * data_ptr) {
  assert(qobj);
  assert(qobj->object);
  assert(data_ptr);

  if(qobj->object == data_ptr->object && qobj->object_type == data_ptr->object_type) {
    return 1;
  }
  else 
    return 0;
}
  

lmodel_connection_t * lmodel_get_connections_from_object(LM_OBJECT_TYPE object_type, void * obj) {
  assert(obj);
  assert(object_type != LM_TYPE_GATE);
  assert(object_type != LM_TYPE_UNDEF);
  if(!obj) return NULL;

  switch(object_type) {
    case LM_TYPE_WIRE:
      return ((lmodel_wire_t *) obj)->connections;
      break;
    case LM_TYPE_VIA:
      return ((lmodel_via_t *) obj)->connections;
      break;
    case LM_TYPE_GATE_PORT:
      return ((lmodel_gate_port_t *) obj)->connections;
      break;
    case LM_TYPE_GATE:
      debug(TM, "connection from unknown type: gate");
      return NULL;
      break;
    default:
      debug(TM, "lmodel_get_connections_from_object(): connection from unknown type");
      return NULL;
  }
}

lmodel_connection_t ** lmodel_get_connection_head_from_object(LM_OBJECT_TYPE object_type, void * obj) {
  assert(obj != NULL);
  assert(object_type != LM_TYPE_GATE);
  assert(object_type != LM_TYPE_UNDEF);

  if(obj == NULL) return NULL;

  switch(object_type) {
    case LM_TYPE_WIRE:
      return &((lmodel_wire_t *) obj)->connections;
      break;
    case LM_TYPE_VIA:
      return &((lmodel_via_t *) obj)->connections;
      break;
    case LM_TYPE_GATE_PORT:
      return &((lmodel_gate_port_t *) obj)->connections;
      break;
    case LM_TYPE_GATE:
      debug(TM, "lmodel_get_connection_head_from_object(): connection from unknown type: gate");
      return NULL;
      break;
    default:
      debug(TM, "lmodel_get_connection_head_from_object(): connection from unknown type %d", object_type);
      assert( 1 == 0);
      return NULL;
  }
}

ret_t lmodel_set_connections_for_object(LM_OBJECT_TYPE object_type, void * obj, lmodel_connection_t * conn) {
  assert(obj);
  if(!obj) return RET_INV_PTR;

  switch(object_type) {
    case LM_TYPE_WIRE:
      ((lmodel_wire_t *) obj)->connections = conn;
      break;
    case LM_TYPE_VIA:
      ((lmodel_via_t *) obj)->connections = conn;
      break;
    case LM_TYPE_GATE_PORT:
      ((lmodel_gate_port_t *) obj)->connections = conn;
      break;
    default:
      debug(TM, "connection from unknown type");
      return RET_ERR;
  }

  return RET_OK;
}

/** 
 * Remove connections from obj.
 */

ret_t lmodel_remove_all_connections_from_object(LM_OBJECT_TYPE object_type, void * obj) {
  assert(obj != NULL);
  if(obj == NULL) return RET_INV_PTR;


  lmodel_connection_t 
    * i = NULL, 
    * ptr_next = NULL, 
    * ptr = lmodel_get_connections_from_object(object_type, obj);

  if(ptr == NULL) return RET_OK;

  debug(TM, "isolate object %p", obj);

  // is it the first?
  if(ptr->obj_ptr == obj) {

    ptr_next = ptr->next;

    i = ptr_next;
    while(i != NULL) {
      if(lmodel_get_connections_from_object(i->object_type, i->obj_ptr) == ptr) {
	debug(TM, "there is a node that references ptr");
	lmodel_set_connections_for_object(i->object_type, i->obj_ptr, ptr->next);
      }
      i = i->next;
    }

    memset(ptr, 0, sizeof(lmodel_connection_t));
    free(ptr);

    lmodel_set_connections_for_object(object_type, obj, NULL);

    return RET_OK;
  }


  while(ptr && ptr->next) {
    if(ptr->next->obj_ptr == obj) {
      ptr_next = ptr->next->next;
      free(ptr->next);
      memset(ptr->next, 0, sizeof(lmodel_connection_t));
      ptr->next = ptr_next;
      lmodel_set_connections_for_object(object_type, obj, NULL);

      return RET_OK;
    }
    ptr = ptr->next;
  }

  assert(1 == 0);
  debug(TM, "ERROR");
  return RET_OK;
  
}

/**
 * Call this function to remove an object from the logic model and from the underlying quadtree as well.
 */
ret_t lmodel_remove_object_by_ptr(logic_model_t * const lmodel, int layer, void * ptr, LM_OBJECT_TYPE object_type) {
  quadtree_object_t * optr;
  lmodel_lookup_object_t data = {0, 0, object_type, ptr};

  ret_t ret;
  CHECK(lmodel, layer);

  optr = quadtree_find_object(lmodel->root[layer], (quadobject_traverse_func_t) &cb_check_object_by_ptr, &data);
  if(!optr) return RET_OK;


  switch(object_type) {
  case LM_TYPE_WIRE:
    if(RET_IS_NOT_OK(ret = lmodel_destroy_wire((lmodel_wire_t *)ptr))) return ret;
    break;
  case LM_TYPE_VIA:
    if(RET_IS_NOT_OK(ret = lmodel_destroy_via((lmodel_via_t *)ptr))) return ret;
    break;
  case LM_TYPE_GATE:
    // remove from list
    if(RET_IS_NOT_OK(ret = lmodel_remove_gate_from_gate_set(lmodel, (lmodel_gate_t *)ptr))) return ret;
    if(RET_IS_NOT_OK(ret = lmodel_destroy_gate((lmodel_gate_t *)ptr))) return ret;
    break;
  default:
    return RET_ERR;
  }
  

  return quadtree_remove_object(optr);
}

ret_t lmodel_build_object_connections_from_helper_structs(logic_model_t * const lmodel) {

  connection_build_helper_t * ptr;
  id_list_t * dst_list_ptr;
  ret_t ret;
  assert(lmodel);
  //assert(lmodel->conn_build_helper);
  if(!lmodel /*|| !lmodel->conn_build_helper*/) return RET_INV_PTR;

  ptr = lmodel->conn_build_helper;
  while(ptr) {

    dst_list_ptr = ptr->to;
    
    while(dst_list_ptr) {
      void * dst_ptr = NULL;
      LM_OBJECT_TYPE obj_type_out;

      if(RET_IS_NOT_OK(ret = lmodel_get_object_by_id(lmodel, dst_list_ptr->id, dst_list_ptr->sub_id, 
						     &dst_ptr, &obj_type_out, NULL)))
	return ret;
      
      if(RET_IS_NOT_OK(ret = lmodel_connect_objects(ptr->object_type, ptr->obj_ptr, 
						    obj_type_out, dst_ptr))) 
	return ret;

      dst_list_ptr = dst_list_ptr->next;
    }
      

    ptr = ptr->next;
  }
  
  return RET_OK;
}

ret_t lmodel_decode_file(logic_model_t * const lmodel, FileContent_t * file_content) {
  
  int i;
  ret_t ret;
  unsigned int highest_object_id = 0;
  assert(lmodel != NULL);
  assert(file_content != NULL);

  if(!lmodel || !file_content) return RET_INV_PTR;
  //if(layer >= lmodel->num_layers) return RET_ERR;

  debug(TM, "There are %d objects", file_content->list.count);

  for(i = 0 ; i < file_content->list.count; i++) {
    if(file_content->list.array[i]) {
      
      Object_t * obj = file_content->list.array[i];
      assert(obj);
      if(!obj) return RET_INV_PTR;
      
      if(obj->present == Object_PR_wire) {
	
	if(RET_IS_NOT_OK(ret = lmodel_import_wire(lmodel, &obj->choice.wire, &highest_object_id))) return ret;
      }
      else if(obj->present == Object_PR_via) {
	if(RET_IS_NOT_OK(ret = lmodel_import_via(lmodel, &obj->choice.via, &highest_object_id))) return ret;
      }
      else if(obj->present == Object_PR_gate) {
	//debug(TM, "Parsing a gate");
	if(RET_IS_NOT_OK(ret = lmodel_import_gate(lmodel, &obj->choice.gate, &highest_object_id))) return ret;
      }
      else if(obj->present == Object_PR_gate_template) {
	//debug(TM, "Parsing a gate template");
	if(RET_IS_NOT_OK(ret = lmodel_import_gate_template(lmodel, &obj->choice.gate_template))) 
	  return ret;
      }
    }
  }

  lmodel->object_id_counter = highest_object_id + 1;
  return RET_OK;
}

lmodel_gate_template_port_t * lmodel_create_gate_template_port() {
  lmodel_gate_template_port_t * port = (lmodel_gate_template_port_t *) malloc(sizeof(lmodel_gate_template_port_t));
  assert(port);
  if(port) {
    memset(port, 0, sizeof(lmodel_gate_template_port_t));
  }
  return port;
}

ret_t lmodel_load_files(logic_model_t * const lmodel, const char * const project_dir, int num_layers) {

  char fq_filename[PATH_MAX];
  FILE * fh;
  FileContent_t * file_content;
  asn_dec_rval_t rval;
  char buf[1024 * 100]; 
  size_t size;
  int running = 1;
  char *ptr = buf;
  struct stat stat_buf;
  ret_t ret;

  assert(lmodel != NULL);
  assert(project_dir != NULL);
  assert(num_layers > 0);
  if(lmodel == NULL || project_dir == NULL) return RET_INV_PTR;

  snprintf(fq_filename, PATH_MAX, "%s/lmodel", project_dir);
  debug(TM, "loading logic model data from %s", fq_filename);

  if(stat(fq_filename, &stat_buf) == -1 && errno == ENOENT) {
    debug(TM, "Warning: file %s does not exist. Ignoring.");
    return RET_OK;
  }

  if((fh = fopen(fq_filename, "rb")) == NULL) {
    debug(TM, "Can't open file %s", fq_filename);
    return RET_ERR;
  }
  

  file_content = (FileContent_t *)calloc(1, sizeof(FileContent_t)); /* not malloc! */
  if(!file_content) {
    debug(TM, "calloc() failed");
    fclose(fh);
    return RET_ERR;
  }

  do {

    // read data
    assert(ptr >= buf);
    size = fread(ptr, 1, sizeof(buf) - (ptr - buf), fh);
    debug(TM, "got %d bytes from file - should read: %d", size, sizeof(buf) - (ptr - buf));
    if(size == 0) {
      if(feof(fh)) {
	running = 0;
	debug(TM, "eof");
      }
      else if(ferror(fh)) {
	running = 0;
      }
    }

    if(ptr-buf > 0 || size != 0) {
      // try to decode data
      rval = asn_DEF_FileContent.ber_decoder(0, &asn_DEF_FileContent, 
					     (void **)&file_content, buf, size + (ptr - buf), 0);
      if(rval.code == RC_OK) {
	debug(TM, "BER decoding ok");
	if(RET_IS_NOT_OK(lmodel_decode_file(lmodel, file_content))) {
	  debug(TM, "parsing results failed.");
	  asn_DEF_FileContent.free_struct(&asn_DEF_FileContent, file_content, 0);
	  fclose(fh);
	  return RET_ERR;
	}
	ptr = buf;
      }
      else if(rval.code == RC_WMORE) {
	int remaining = size + (ptr - buf) - rval.consumed;
	debug(TM, "need more data, remaining data in buffer = %d", remaining);
	memcpy(buf, buf + rval.consumed, remaining);
	ptr = buf + remaining;
      }
      else {
	fprintf(stderr, "Could not decode\n");
	asn_DEF_FileContent.free_struct(&asn_DEF_FileContent, file_content, 0);
	fclose(fh);
	return RET_ERR;
      }
    }
  } while(running);

  asn_DEF_FileContent.free_struct(&asn_DEF_FileContent, file_content, 0);

  if(!feof(fh)) {
    fclose(fh);
    debug(TM, "IO error");
    return RET_ERR;
  }
  fclose(fh);


  if(RET_IS_NOT_OK(ret = lmodel_build_object_connections_from_helper_structs(lmodel))) return ret;

  return RET_OK;
}

/**
 * @return The function returns the first (minimal) number of a layer,
 *         that matches the layer type. If there is no layer, the function
 *         returns -1
 */
int lmodel_get_layer_num_by_type(const logic_model_t * const lmodel, LAYER_TYPE layer_type) {
  int i;
  assert(lmodel);
  if(!lmodel) return 0;
  for(i = 0; i < lmodel->num_layers; i++)
    if(lmodel->layer_type[i] == layer_type) return i;
  return -1;
}

ret_t lmodel_set_layer_type(logic_model_t * const lmodel, int layer, LAYER_TYPE layer_type) {

  CHECK(lmodel, layer);

  lmodel->layer_type[layer] = layer_type;
  return RET_OK;
}

LAYER_TYPE lmodel_get_layer_type(logic_model_t * const lmodel, int layer) {
  return (lmodel && layer < lmodel->num_layers) ? lmodel->layer_type[layer] : LM_LAYER_TYPE_UNDEF;
}

ret_t lmodel_get_layer_type_as_string(logic_model_t * const lmodel, int layer, char * const out_str, int len) {
  CHECK(lmodel, layer);

  switch(lmodel->layer_type[layer]) {
  case LM_LAYER_TYPE_METAL:
    strncpy(out_str, "LM_LAYER_TYPE_METAL", len);
    break;
  case LM_LAYER_TYPE_LOGIC:
    strncpy(out_str, "LM_LAYER_TYPE_LOGIC", len);
    break;
  case LM_LAYER_TYPE_TRANSISTOR:
    strncpy(out_str, "LM_LAYER_TYPE_TRANSISTOR", len);
    break;
  default:
    strncpy(out_str, "LM_LAYER_TYPE_UNDEF", len);
    break;
  }
  return RET_OK;
}

ret_t lmodel_set_layer_type_from_string(logic_model_t * const lmodel, int layer, const char * const type_str) {

  LAYER_TYPE layer_type;
  CHECK(lmodel, layer);

  if(!strcmp(type_str, "LM_LAYER_TYPE_METAL")) layer_type = LM_LAYER_TYPE_METAL;
  else if(!strcmp(type_str, "LM_LAYER_TYPE_LOGIC")) layer_type = LM_LAYER_TYPE_LOGIC;
  else if(!strcmp(type_str, "LM_LAYER_TYPE_TRANSISTOR")) layer_type = LM_LAYER_TYPE_TRANSISTOR;
  else if(!strcmp(type_str, "LM_LAYER_TYPE_UNDEF")) layer_type = LM_LAYER_TYPE_UNDEF;
  else return RET_ERR;
  
  lmodel->layer_type[layer] = layer_type;
  return RET_OK;
}

ret_t lmodel_clear_layer(logic_model_t * const lmodel, int layer) {
  CHECK(lmodel, layer);

  return quadtree_traverse_complete(lmodel->root[layer], (quadtree_traverse_func_t) &cb_destroy_objects, NULL);
}



ret_t lmodel_clear_area(logic_model_t * const lmodel, int layer,
		      unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y) {

  CHECK(lmodel, layer);
  // XXX                                                                                                                                          
  return RET_ERR;
}

lmodel_via_t * lmodel_create_via(logic_model_t * const lmodel,
				 unsigned int x, unsigned int y,
				 LM_VIA_DIR direction,
				 unsigned int diameter,
				 char * name,
				 unsigned int obj_id) {

  lmodel_via_t * via;
  assert(lmodel);
  if(!lmodel) return NULL;

  // create object
  if((via = (lmodel_via_t *) malloc(sizeof(lmodel_via_t))) == NULL) {
    debug(TM, "Can't malloc mem for via.");
    return NULL;
  }

  memset(via, 0, sizeof(lmodel_via_t));

  via->x = x;
  via->y = y;
  via->diameter = diameter;
  via->direction = direction;
  via->name = name ? name : strdup("");
  via->id = obj_id ? obj_id : lmodel->object_id_counter++;

  return via;
}

ret_t lmodel_add_via_with_autojoin(logic_model_t * const lmodel, int layer,
				   lmodel_via_t * via) {

  ret_t ret;
  unsigned int radius = via->diameter >> 1;
  unsigned int min_x = via->x > radius ? via->x - radius : 0;
  unsigned int min_y = via->y > radius ? via->y - radius : 0;
  unsigned int max_x = via->x + radius;
  unsigned int max_y = via->y + radius;

  lmodel_check_collision_t data = {lmodel, layer, LM_TYPE_VIA, via};
  quadtree_traverse_func_t cb_func = (quadtree_traverse_func_t) &cb_join_on_contact;

  if(RET_IS_NOT_OK(ret = quadtree_traverse_complete_within_region(lmodel->root[layer], 
								  min_x, min_y,max_x, max_y, 
								  cb_func, &data)))
    return ret;

  
  return lmodel_add_via(lmodel, layer, via);
}


ret_t lmodel_add_via(logic_model_t * const lmodel, int layer,
		     lmodel_via_t * via) {

  quadtree_object_t * obj;
  unsigned int max_rad;
  CHECK(lmodel, layer);
  assert(via);
  if(!via) return RET_INV_PTR;

  max_rad = via->diameter / 2 + 1;

  obj = quadtree_object_create(LM_TYPE_VIA, (void *) via, 
			       via->x >  max_rad ? via->x - max_rad : 0,
			       via->y > max_rad ? via->y - max_rad : 0,
			       via->x + max_rad < lmodel->width - 1? via->x + max_rad : lmodel->width - 1,
			       via->y + max_rad < lmodel->height - 1 ? via->y + max_rad : lmodel->height - 1);

  if(!obj) return RET_ERR;

  // add object to quadtree
  if(!quadtree_insert(lmodel->root[layer], obj)) {
    debug(TM, "Can't insert via into quad tree.");
    lmodel_destroy_via(via);
    quadtree_object_destroy(obj);
    return RET_ERR;
  }

  return RET_OK;
}

lmodel_wire_t * lmodel_create_wire(logic_model_t * const lmodel,
				   unsigned int from_x, unsigned int from_y,
				   unsigned int to_x, unsigned int to_y,
				   unsigned int diameter,
				   char * name,
				   unsigned int obj_id) {

  lmodel_wire_t * wire;
  assert(lmodel);
  if(!lmodel) return NULL;
  // create object
  if((wire = (lmodel_wire_t *) malloc(sizeof(lmodel_wire_t))) == NULL)
    return NULL;

  memset(wire, 0, sizeof(lmodel_wire_t));

  wire->from_x = from_x; // there is no clipping
  wire->from_y = from_y;
  wire->to_x = to_x;
  wire->to_y = to_y;
  wire->diameter = diameter;
  wire->name = name ? name : strdup("");
  wire->id = obj_id ? obj_id : lmodel->object_id_counter++;

  return wire;
}


			    
ret_t cb_join_on_contact(quadtree_t * qtree, lmodel_check_collision_t * data) {
  assert(data);
  assert(qtree);
  if(!qtree || !data) return RET_INV_PTR;
  ret_t ret;
  
  quadtree_object_t * ptr = qtree->objects;
  while(ptr) {
    //debug(TM, "there is an object. check for collision");
    if(lmodel_check_collision(data->object_type, data->object, 
			      (LM_OBJECT_TYPE)ptr->object_type, ptr->object)) {
      // join
      //debug(TM, "\tthere is a collision");
      if(RET_IS_NOT_OK(ret = lmodel_connect_objects(data->object_type, data->object, 
						    (LM_OBJECT_TYPE)ptr->object_type, ptr->object)))
	return ret;
	 
    }
    else {
      debug(TM, "\tno collision");
    }
    ptr = ptr->next;
  }
  return RET_OK;
}

ret_t lmodel_add_wire_with_autojoin(logic_model_t * const lmodel, int layer,
				    lmodel_wire_t * wire) {

  ret_t ret;
  // XXX if wire is horizontal or vertical, the boundinx box is to small, It might even work.
  unsigned int min_x = MIN(wire->from_x, wire->to_x);
  unsigned int min_y = MIN(wire->from_y, wire->to_y);
  unsigned int max_x = MIN(wire->from_x, wire->to_x);
  unsigned int max_y = MAX(wire->from_y, wire->to_y);

  lmodel_check_collision_t data = {lmodel, layer, LM_TYPE_WIRE, wire};
  quadtree_traverse_func_t cb_func = (quadtree_traverse_func_t) &cb_join_on_contact;

  if(RET_IS_NOT_OK(ret = quadtree_traverse_complete_within_region(lmodel->root[layer], 
								  min_x, min_y,max_x, max_y, 
								  cb_func, &data)))
    return ret;

  
  return lmodel_add_wire(lmodel, layer, wire);
}

ret_t lmodel_add_wire(logic_model_t * const lmodel, int layer,
		      lmodel_wire_t * wire) {

  quadtree_object_t * obj;
  CHECK(lmodel, layer);
  assert(wire);
  if(!wire) return RET_INV_PTR;

  unsigned int 
    wire_from_x = wire->from_x, 
    wire_from_y = wire->from_y,
    wire_to_x = wire->to_x, 
    wire_to_y = wire->to_y,
    radius = wire->diameter >> 1;

  // If a wire is horizontal or vertical it's bounding box is to small. We extend it a little bit.
  if(wire_from_x + wire->diameter > wire_to_x) {
    debug(TM, "extend bounding box for wire - x");
    wire_from_x = wire_from_x > radius ? wire_from_x - radius : 0;
    wire_to_x = wire_to_x + radius < lmodel->width - 1 ? wire_to_x + radius : lmodel->width - 1;
  }
  if(wire_from_y + wire->diameter > wire_to_y) {
    debug(TM, "extend bounding box for wire - y");
    wire_from_y = wire_from_y > radius ? wire_from_y - radius : 0;
    wire_to_y = wire_to_y + radius < lmodel->height - 1 ? wire_to_y + radius : lmodel->height - 1;
  }

  obj = quadtree_object_create(LM_TYPE_WIRE, (void *) wire, 
			       wire_from_x, wire_from_y, wire_to_x, wire_to_y);
  if(!obj) return RET_ERR;
  if(!quadtree_insert(lmodel->root[layer], obj)) {
    lmodel_destroy_wire(wire);
    quadtree_object_destroy(obj);
    return RET_ERR;
  }
  
  return RET_OK;
}

lmodel_gate_t * lmodel_create_gate(logic_model_t * const lmodel,
				   unsigned int min_x, unsigned int min_y, 
				   unsigned int max_x, unsigned int max_y,
				   lmodel_gate_template_t * gate_template,
				   char * name,
				   unsigned int obj_id) {

  lmodel_gate_t * gate;
  assert(lmodel);
  if(!lmodel) return NULL;

  // create object
  if((gate = (lmodel_gate_t *) malloc(sizeof(lmodel_gate_t))) == NULL)
    return NULL;

  memset(gate, 0, sizeof(lmodel_gate_t));

  gate->min_x = min_x;
  gate->min_y = min_y;
  gate->max_x = max_x;
  gate->max_y = max_y;
  gate->name = name;
  gate->id = obj_id ? obj_id : lmodel->object_id_counter++;
  gate->gate_template = gate_template;

  if(gate_template != NULL) gate_template->reference_counter++;

  return gate;
}

/**
 * Adds a gate placement into logic model. 
 */
ret_t lmodel_add_gate(logic_model_t * const lmodel, int layer,
		      lmodel_gate_t * gate) {

  ret_t ret;
  quadtree_object_t * obj;
  CHECK(lmodel, layer);
  assert(gate);
  if(!gate) return RET_INV_PTR;

  obj = quadtree_object_create(LM_TYPE_GATE, (void *) gate, 
			       gate->min_x,
			       gate->min_y,
			       gate->max_x,
			       gate->max_y);

  assert(obj != NULL);
  if(obj == NULL) return RET_ERR;
  
  // add object to quadtree
  if(!quadtree_insert(lmodel->root[layer], obj)) {
    lmodel_destroy_gate(gate);
    quadtree_object_destroy(obj);
    return RET_ERR;
  }
  
  if(RET_IS_NOT_OK(ret = lmodel_update_gate_ports(gate))) return ret;

  // add to gate list
  return lmodel_add_gate_to_gate_set(lmodel, gate);

}



ret_t cb_remove_refs(quadtree_t * qtree, lmodel_gate_template_t * const tmpl) {

  quadtree_object_t * object;
  ret_t ret;

  assert(qtree);
  assert(tmpl);
  if(!qtree || !tmpl) return RET_INV_PTR;

  object = qtree->objects;
  
  while(object != NULL) {
    if(object->object_type == LM_TYPE_GATE) {

      lmodel_gate_t * gate = (lmodel_gate_t *) object->object;
      if(gate->gate_template == tmpl) {
	gate->gate_template = NULL;
	
	if(gate->ports) if(RET_IS_NOT_OK(ret = lmodel_destroy_gate_ports(gate->ports))) return ret;
	gate->ports = NULL;
      }

    }
    object = object->next;
  }
  return RET_OK;
}

/** 
 * If one removes gate templates, it is possible that
 * there are still references to the template. There references
 * must be removed.
 *
 * TODO: reimplement it
 */
ret_t lmodel_remove_refs_to_gate_template(logic_model_t * const lmodel, lmodel_gate_template_t * const tmpl) {
  int layer;
  quadtree_traverse_func_t cb_func = (quadtree_traverse_func_t) &cb_remove_refs;  
  assert(lmodel);
  assert(tmpl);
  if(!lmodel || !tmpl || !lmodel->root) return RET_INV_PTR;

  for(layer = 0; layer < lmodel->num_layers; layer++) {
    assert(lmodel->root[layer]);
    if(!lmodel->root[layer]) return RET_INV_PTR;
    quadtree_traverse_complete(lmodel->root[layer], cb_func, tmpl);
  }


  return RET_OK;
}


lmodel_gate_template_t * lmodel_create_gate_template() {
  lmodel_gate_template_t * tmpl;

  if((tmpl = (lmodel_gate_template_t *)malloc(sizeof(lmodel_gate_template_t))) == NULL) return NULL;

  memset(tmpl, 0, sizeof(lmodel_gate_template_t));
  return tmpl;
}

ret_t lmodel_gate_template_set_master_region(lmodel_gate_template_t * const tmpl,
					     unsigned int min_x, unsigned int min_y, 
					     unsigned int max_x, unsigned int max_y) {

  
  assert(tmpl);
  if(!tmpl) return RET_INV_PTR;

  tmpl->master_image_min_x = MIN(min_x, max_x);
  tmpl->master_image_min_y = MIN(min_y, max_y);
  tmpl->master_image_max_x = MAX(min_x, max_x);
  tmpl->master_image_max_y = MAX(min_y, max_y);
  
  return RET_OK;
}

ret_t lmodel_gate_template_set_text(lmodel_gate_template_t * const tmpl, 
				    const char * const short_name,
				    const char * const description) {

  assert(tmpl);
  assert(short_name);
  assert(description);
  if(!tmpl || !short_name || !description) return RET_INV_PTR;

  if(tmpl->short_name) free(tmpl->short_name);
  if(tmpl->description) free(tmpl->description);

  tmpl->short_name = strdup(short_name);
  tmpl->description = strdup(description);

  return RET_OK;
}
				    
ret_t lmodel_add_gate_to_gate_set(logic_model_t * const lmodel, 
				  lmodel_gate_t * const gate) {
  
  lmodel_gate_set_t * gset;
  assert(lmodel != NULL);
  assert(gate != NULL);
  if(lmodel == NULL || gate == NULL) return RET_INV_PTR;
  
  if((gset = (lmodel_gate_set_t *) malloc(sizeof(lmodel_gate_set_t))) == NULL) 
    return RET_MALLOC_FAILED;
  memset(gset, 0, sizeof(lmodel_gate_set_t));

  gset->next = lmodel->gate_set;
  gset->gate = gate;
  lmodel->gate_set = gset;
  return RET_OK;
}

/**
 * Remove a gate (only) from a gate list. This function does not remove any references.
 *
 * @see lmodel_remove_object_by_ptr()
 */
ret_t lmodel_remove_gate_from_gate_set(logic_model_t * const lmodel, lmodel_gate_t * const gate) {
  assert(lmodel != NULL);
  assert(gate != NULL);
  if(lmodel == NULL || gate == NULL) return RET_INV_PTR;
  lmodel_gate_set_t * ptr = lmodel->gate_set;
  lmodel_gate_set_t * ptr_last = NULL;
  while(ptr != NULL) {
    if(ptr->gate == gate) {
      if(ptr_last != NULL) ptr_last->next = ptr->next;
      else lmodel->gate_set = ptr->next;
      free(ptr);
      return RET_OK;
    }
    ptr_last = ptr;
    ptr = ptr->next;
  }
  return RET_OK;
}


/**
 * Remove gate instances from logic model that reference a template of type tmpl and destroy the gates.
 */
ret_t lmodel_destroy_gates_by_template_type(logic_model_t * const lmodel, 
					    const lmodel_gate_template_t * const tmpl,					    
					    GS_DESTROY_MODE mode) {
  ret_t ret;
  assert(lmodel != NULL);
  assert(tmpl != NULL);
  if(lmodel == NULL || tmpl == NULL) return RET_INV_PTR;
  
  int layer = lmodel_get_layer_num_by_type(lmodel, LM_LAYER_TYPE_LOGIC);

  lmodel_gate_set_t * gset_ptr = lmodel->gate_set;
  while(gset_ptr != NULL) {
    if(gset_ptr->gate != NULL && gset_ptr->gate->gate_template == tmpl) {
      int destroy = 1;
      if(mode == DESTROY_WO_MASTER && lmodel_gate_is_master(gset_ptr->gate)) destroy = 0;

      // implicit removal from gate set
      if(destroy == 1 && RET_IS_NOT_OK(ret = lmodel_remove_object_by_ptr(lmodel, layer, gset_ptr->gate, 
									 LM_TYPE_GATE))) return ret;
    }
    gset_ptr = gset_ptr->next;
  }
  return RET_OK;
}


/** Add a gate template to a gate template set. 
 * @see lmodel_add_gate_template()
 */
ret_t lmodel_add_gate_template_to_gate_template_set(lmodel_gate_template_set_t * const gate_template_set, 
						    lmodel_gate_template_t * const tmpl, 
						    unsigned int obj_id) {
  
  lmodel_gate_template_set_t * gset;
  lmodel_gate_template_set_t * ptr = gate_template_set;
  unsigned int max_id = 0;
  assert(tmpl);
  assert(gate_template_set);
  if(!gate_template_set || !tmpl) return RET_INV_PTR;
  
  if((gset = (lmodel_gate_template_set_t *) malloc(sizeof(lmodel_gate_template_set_t))) == NULL) 
    return RET_MALLOC_FAILED;
  memset(gset, 0, sizeof(lmodel_gate_template_set_t));

  gset->gate = tmpl;

  // goto tail
  while(ptr->next != NULL) {
    if(ptr->gate && ptr->gate->id > max_id) max_id = ptr->gate->id;
    ptr = ptr->next;
  }

  if(tmpl->id == 0) {
    tmpl->id = obj_id ? obj_id : MAX(ptr->gate ? ptr->gate->id : 0, max_id) + 1;
  }

  // insert
  ptr->next = gset;
  return RET_OK;

}

lmodel_gate_template_t * lmodel_get_gate_template_by_id(logic_model_t * const lmodel, unsigned int obj_id) {

  lmodel_gate_template_set_t * gtmpl_set;
  assert(lmodel);
  if(!lmodel) return NULL;

  gtmpl_set = lmodel->gate_template_set;
  while(gtmpl_set != NULL) {
    if(gtmpl_set->gate && gtmpl_set->gate->id == obj_id) return gtmpl_set->gate;
    gtmpl_set = gtmpl_set->next;
  }
  return NULL;
}

/** Add a gate template to the logic model. 
 * @see lmodel_add_gate_template_to_gate_template_set()
 */
ret_t lmodel_add_gate_template(logic_model_t * const lmodel, 
			       lmodel_gate_template_t * const tmpl, 
			       unsigned int obj_id) {


  assert(lmodel);
  assert(tmpl);
  if(!lmodel || !tmpl) return RET_INV_PTR;
  
  if(lmodel->gate_template_set == NULL) {

    lmodel->gate_template_set = lmodel_create_gate_template_set(tmpl);
    assert(lmodel->gate_template_set != NULL);
    if(lmodel->gate_template_set == NULL) return RET_MALLOC_FAILED;
      
    return RET_OK;
  }
  else return lmodel_add_gate_template_to_gate_template_set(lmodel->gate_template_set, tmpl, obj_id);
}

/** create a gate template set and add a template 
 */
lmodel_gate_template_set_t * lmodel_create_gate_template_set(lmodel_gate_template_t * const tmpl) {

  lmodel_gate_template_set_t * gset = NULL;

  if((gset = (lmodel_gate_template_set_t *) malloc(sizeof(lmodel_gate_template_set_t))) == NULL) 
    return NULL;
  memset(gset, 0, sizeof(lmodel_gate_template_set_t));

  gset->gate = tmpl;

  return gset;
}

/** 
 * For some applications we need the linear function, that describes the line of a wire.
 * A linear function is given by f(x) = mx + n. The function calulates m and n.
 * If the line is nearly parallel with the y axis, there is no function, that describes the line.
 * In that case the function return RET_MATH_ERR.
 * @returns This function returns RET_MATH_ERR, if it is impossible to derive a linear function. This
 *  means, that the lines is nearly parallel to the y axis. On success RET_OK is returned. If you pass
 *  a NULL pointer RET_INV_PTR is returned;
 */
ret_t get_line_function_for_wire(lmodel_wire_t * wire, double * m, double * n) {
  assert(wire);
  assert(m);
  assert(n);
  if(!wire || !m | !n) return RET_INV_PTR;

  double d_y = ((double)wire->to_y - (double)wire->from_y);
  double d_x = ((double)wire->to_x - (double)wire->from_x);
    
  if(fabs(d_x) < EPSILON) return RET_MATH_ERR; 
  else {
    *m =  d_y / d_x;
    *n = (double)(wire->from_y) - (double)(wire->from_x) * *m;
    
  }
  return RET_OK;
}

/**
 * If you redefine the master for a gate, it is possible that your new master is not oriented the same way
 * as your old master. But the new master needs to be oriented "normal". 
 *
 * The coordinates for a template port are relative to the upper left corner of a normal oriented template.
 * @param tmpl is the template
 * @param trans The transformation that must be applied to the coordinates of all ports.
 */

ret_t lmodel_adjust_templates_port_locations(lmodel_gate_template * const tmpl, 
					     LM_TEMPLATE_ORIENTATION trans) {
  assert(tmpl != NULL);
  if(tmpl == NULL) return RET_INV_PTR;

  unsigned int height = tmpl->master_image_max_y - tmpl->master_image_min_y;
  unsigned int width = tmpl->master_image_max_x - tmpl->master_image_min_x;

  lmodel_gate_template_port_t * port_ptr = tmpl->ports;
  while(port_ptr != NULL) {

    switch(trans) {
    case LM_TEMPLATE_ORIENTATION_UNDEFINED:
      return RET_ERR;
    case LM_TEMPLATE_ORIENTATION_NORMAL:
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN:
      port_ptr->relative_y_coord = height - port_ptr->relative_y_coord;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT:
      port_ptr->relative_x_coord= width - port_ptr->relative_x_coord;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH:
      port_ptr->relative_x_coord = width - port_ptr->relative_x_coord;
      port_ptr->relative_y_coord = height - port_ptr->relative_y_coord;
      break;
    }
    

    port_ptr = port_ptr->next;
  }
  
  return RET_OK;
}

/**
 * Check, if at (x,y) is a gate port.
 * @returns The function returns a pointer to the gate port. If there are invalid params or no gate or no gate port, the function returns NULL.
 * @param gate A gate with an assigned master template. The gate is somewhere placed on a layer.
 * @param x,y The x coordinate relative to the layer origin. If values are larger than layer's width or height, nothing will be found.
 */
lmodel_gate_port_t * get_gate_port_at(const lmodel_gate_t * const gate, unsigned int x, unsigned int y) {
  assert(gate != NULL);
  if(gate == NULL) return NULL;

  if(x >= gate->min_x && x < gate->max_x &&
     y >= gate->min_y && y < gate->max_y) {

    unsigned int x_rel_to_gate = x - gate->min_x;
    unsigned int y_rel_to_gate = y - gate->min_y;

    unsigned int height = gate->max_y - gate->min_y;
    unsigned int width = gate->max_x - gate->min_x;

    //debug(TM, "there is a gate. is there a gate port?");
    switch(gate->template_orientation) {
    case LM_TEMPLATE_ORIENTATION_UNDEFINED:
      return NULL;
    case LM_TEMPLATE_ORIENTATION_NORMAL:
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN:
      y_rel_to_gate = height - y_rel_to_gate;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT:
      x_rel_to_gate = width - x_rel_to_gate;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH:
      x_rel_to_gate = width - x_rel_to_gate;
      y_rel_to_gate = height - y_rel_to_gate;
      break;
    }

    lmodel_gate_port_t * ports = gate->ports;
    while(ports != NULL) {
    
      lmodel_gate_template_port_t * tmpl_port = ports->tmpl_port;
      if(tmpl_port != NULL) {
	unsigned int 
	  _x = tmpl_port->relative_x_coord, 
	  _y = tmpl_port->relative_y_coord;
      
	//debug(TM, "check port %s", ports->tmpl_port->port_name);
	
	double delta_x = (double)x_rel_to_gate - (double)_x; 
	double delta_y = (double)y_rel_to_gate - (double)_y; 
	
	if(sqrt( delta_x * delta_x + delta_y * delta_y) < (tmpl_port->diameter << 1) ) {
	  assert(ports->tmpl_port != NULL);
	  
	  //debug(TM, "port clicked %s", ports->tmpl_port->port_name);
	  return ports;
	}
      }
      ports = ports->next;
    }

  }
  return NULL;
}

int cb_is_object_at_point(quadtree_object_t * qobj, lmodel_is_object_at_t * data_ptr) {
  //debug(TM, "checking");
  assert(qobj);
  assert(qobj->object);
  assert(data_ptr);
  if(qobj->object_type == LM_TYPE_GATE) {

    lmodel_gate_port_t * port = get_gate_port_at((const lmodel_gate_t * const)qobj->object, data_ptr->x, data_ptr->y);
    if(port == NULL) {
      data_ptr->object = qobj->object;
      data_ptr->object_type = LM_TYPE_GATE;
    }
    else {
      data_ptr->object = port;
      data_ptr->object_type = LM_TYPE_GATE_PORT;
    }
    return 1;
  }
  else if(qobj->object_type == LM_TYPE_WIRE) {
    debug(TM, "\twire");
    /*
      How to check if a point is on a line:
      y = m*x + n
      m = dy / dx
      n = y0 - m*x0
      y' = m*x' + n

      |y' - y| < epsilon?
    */
    lmodel_wire_t * wire = (lmodel_wire_t *) qobj->object;

    double d_y = ((double)wire->to_y - (double)wire->from_y);
    double d_x = ((double)wire->to_x - (double)wire->from_x);
 
    // Check if it is a vertical line (dy ~~ 0). If it is true, the bounding box describes the line.
    // And we already know, that (x,y) is in the bounding box
    if(fabs(d_x) < EPSILON) {
      debug(TM, "wire %d is vertical", wire->id);
      data_ptr->object = wire;
      data_ptr->object_type = LM_TYPE_WIRE;
      return 1; 
    }
      
     double m =  d_y / d_x;
     double n = (double)(wire->from_y) - (double)(wire->from_x) * m;
     double y_dash = m * (double) data_ptr->x + n;

     if(fabs(y_dash - data_ptr->y) <= 2 * wire->diameter) {
       data_ptr->object = wire;
       data_ptr->object_type = LM_TYPE_WIRE;
       return 1;
     }
     debug(TM, "\t-> no");
     return 0;
  }
  else if(qobj->object_type == LM_TYPE_VIA) {
    data_ptr->object = qobj->object;
    data_ptr->object_type = LM_TYPE_VIA;
    return 1;
  }
  return 0;
}

/** This function gets an object, that is placed at (x,y). If there is more than one object, the first
    is returned.
 */
ret_t lmodel_get_object(const logic_model_t * const lmodel, int layer, unsigned int real_x, unsigned int real_y,
			LM_OBJECT_TYPE * result_object_type, void ** result_object_ptr) {

  assert(result_object_type);
  assert(result_object_ptr);
  if(!result_object_type || !result_object_ptr) return RET_INV_PTR;
  CHECK(lmodel, layer);

  quadobject_traverse_func_t cb_func = (quadobject_traverse_func_t) &cb_is_object_at_point;
  lmodel_is_object_at_t data = {real_x, real_y, LM_TYPE_UNDEF, NULL};
  
  quadtree_get_object_at(lmodel->root[layer], real_x, real_y, cb_func, &data);
  
  *result_object_ptr = data.object;
  *result_object_type = data.object_type;
  return RET_OK;    
}


ret_t cb_is_gate_in_region(quadtree_t * qtree, lmodel_is_gate_in_region_t * data_ptr) {
  assert(data_ptr);
  assert(qtree);
  if(!qtree || !data_ptr) return RET_INV_PTR;

  quadtree_object_t * ptr = qtree->objects;
  lmodel_is_gate_in_region_t * params = (lmodel_is_gate_in_region_t *) data_ptr;

  while(ptr) {
    //debug(TM, "there is an object. check for collision");

    if(ptr->object_type == LM_TYPE_GATE) {
      lmodel_gate_t * gate = (lmodel_gate_t *)ptr->object;
      
      //debug(TM, "it's a gate with id=%d", gate->id); //(390,4489)

      if(data_ptr->object == NULL) { // not found
	/*	if(gate->id == 268) {
	  debug(TM, "check qt obj 268 %d, %d | %d, %d", gate->min_x, gate->max_x, gate->min_y, gate->max_y);
	  debug(TM, "  agains region  %d, %d | %d, %d", params->from_x, params->to_x, params->from_y, params->to_y);
	  } */
	if( //(gate->min_x == params->from_x && gate->min_y == params->from_y)
	    //||
	    !(gate->min_x > params->to_x ||
	      gate->max_x < params->from_x ||
	      gate->min_y > params->to_y ||
	      gate->max_y < params->from_y)) {
	  //debug(TM, "Gate with id = %d found!", gate->id);
	  
	  data_ptr->object = gate;
	  return RET_OK;
	}
	//else debug(TM, "No");
      }
      //else debug(TM, "already found object with id = %d", data_ptr->object->id);
    }

    ptr = ptr->next;
  }
  return RET_OK;
}


ret_t lmodel_get_gate_in_region(const logic_model_t * const lmodel, int layer, 
				unsigned int from_real_x, unsigned int from_real_y,
				unsigned int to_real_x, unsigned int to_real_y,
				lmodel_gate_t ** result_object_ptr) {

  ret_t ret;
  assert(result_object_ptr);
  if(!result_object_ptr) return RET_INV_PTR;
  CHECK(lmodel, layer);

  quadtree_traverse_func_t cb_func = (quadtree_traverse_func_t) &cb_is_gate_in_region;
  lmodel_is_gate_in_region_t data = {from_real_x, from_real_y, to_real_x, to_real_y, NULL};

  if(RET_IS_NOT_OK(ret = quadtree_traverse_complete_within_region(lmodel->root[layer], 
								  from_real_x, from_real_y, 
								  to_real_x, to_real_y, cb_func, &data)))
     return ret;

  *result_object_ptr = data.object;
  return RET_OK;    

}

ret_t lmodel_object_to_string(const logic_model_t * const lmodel, int layer, 
			      unsigned int real_x, unsigned int real_y, 
			      char * const msg, unsigned int len) {

  void * ptr = NULL;
  ret_t ret;
  CHECK(lmodel, layer);
  LM_OBJECT_TYPE object_type;

  if(len == 0) return RET_ERR;
  else msg[0] = '\0';

  if(RET_IS_NOT_OK(ret = lmodel_get_object(lmodel, layer, real_x, real_y, &object_type, &ptr)))
    return ret;

  if(ptr != NULL)
    return lmodel_get_printable_string_for_obj(object_type, ptr, msg, len);
  else
    return RET_OK;
}

ret_t lmodel_get_printable_string_for_obj(LM_OBJECT_TYPE object_type, void * obj_ptr, char * const msg, unsigned int len) {
  lmodel_gate_t * g;
  lmodel_gate_port_t * gp;
  lmodel_wire_t * w;
  lmodel_via_t * v;

  assert(obj_ptr != NULL);
  assert(msg != NULL);
  if(obj_ptr == NULL || msg == NULL) return RET_INV_PTR;

  switch(object_type) {
  case LM_TYPE_GATE:
    g = (lmodel_gate_t *) obj_ptr;
    if(g->gate_template == NULL || g->gate_template->short_name == NULL) {
      if(g->name == NULL || !strcmp(g->name, "")) snprintf(msg, len, "gate (%d)", g->id);
      else snprintf(msg, len, "gate %s (%d)", g->name, g->id);
    }
    else {
      if(g->name == NULL || !strcmp(g->name, "")) snprintf(msg, len, "%s (%d)", g->gate_template->short_name, g->id);
      else snprintf(msg, len, "%s : %s (%d)", g->gate_template->short_name, g->name, g->id);
    }
    break;
  case LM_TYPE_GATE_PORT:
    gp = (lmodel_gate_port_t *) obj_ptr;

    if(gp->tmpl_port) {
      g = gp->gate;
      if(g->name == NULL || !strcmp(g->name, "")) 
	snprintf(msg, len, "%s (%d) : %s", g->gate_template->short_name, g->id, gp->tmpl_port->port_name);
      else snprintf(msg, len, "%s : %s (%d) : %s", g->gate_template->short_name, g->name, g->id, gp->tmpl_port->port_name);
    }
    
    break;

  case LM_TYPE_WIRE:
    w = (lmodel_wire_t *) obj_ptr;
    if(w->name == NULL) snprintf(msg, len, "wire (%d)", w->id);
    else snprintf(msg, len, "%s (%d)", w->name, w->id);
    break;

  case LM_TYPE_VIA:
    v = (lmodel_via_t *) obj_ptr;
    if(v->name == NULL) snprintf(msg, len, "via (%d)", v->id);
    else snprintf(msg, len, "%s (%d)", v->name, v->id);

    break;
  case LM_TYPE_UNDEF:
  default:
    snprintf(msg, len, "undefined object");
  }
    
  return RET_OK;
}

int lmodel_get_select_state(LM_OBJECT_TYPE object_type, void * obj_ptr) {
  switch(object_type) {
  case LM_TYPE_GATE:
    return ((lmodel_gate_t *) obj_ptr)->is_selected;
    break;
  case LM_TYPE_GATE_PORT:
    return ((lmodel_gate_port_t *) obj_ptr)->is_selected;
    break;
  case LM_TYPE_WIRE:
    return ((lmodel_wire_t *) obj_ptr)->is_selected;
    break;
  case LM_TYPE_VIA:
    return ((lmodel_via_t *) obj_ptr)->is_selected;
    break;
  default:
    return 0;
  }
}

ret_t lmodel_set_select_state(LM_OBJECT_TYPE object_type, void * obj_ptr, int state) {
  assert(obj_ptr);
  if(!obj_ptr) return RET_INV_PTR;

  lmodel_connection_t * adj_objects = NULL;

  switch(object_type) {
  case LM_TYPE_GATE:
    ((lmodel_gate_t *) obj_ptr)->is_selected = state;
    adj_objects = NULL;
    break;
  case LM_TYPE_GATE_PORT:
    ((lmodel_gate_port_t *) obj_ptr)->is_selected = state;
    adj_objects = ((lmodel_gate_port_t *) obj_ptr)->connections;
    break;
  case LM_TYPE_WIRE:
    ((lmodel_wire_t *) obj_ptr)->is_selected = state;
    adj_objects = ((lmodel_wire_t *) obj_ptr)->connections;
    break;
  case LM_TYPE_VIA:
    ((lmodel_via_t *) obj_ptr)->is_selected = state;
    adj_objects = ((lmodel_via_t *) obj_ptr)->connections;
    break;
  default:
    return RET_ERR;
  }

  
  while(adj_objects) {
    /// XXX
    if(state == SELECT_STATE_NOT) {
      if(lmodel_get_select_state(adj_objects->object_type, adj_objects->obj_ptr) != state) 
	lmodel_set_select_state(adj_objects->object_type, adj_objects->obj_ptr, state);
    }
    else {
      if(lmodel_get_select_state(adj_objects->object_type, adj_objects->obj_ptr) == SELECT_STATE_NOT) 
	lmodel_set_select_state(adj_objects->object_type, adj_objects->obj_ptr, SELECT_STATE_ADJ);
    }

    adj_objects = adj_objects->next;
  }
  return RET_OK;
}


ret_t lmodel_set_gate_name(lmodel_gate_t * gate, const char * const new_name) {
  assert(gate);
  if(!gate) return RET_INV_PTR;

  if(gate->name) free(gate->name);
  gate->name = strdup(new_name);

  return RET_OK;
}

ret_t lmodel_set_wire_name(lmodel_wire_t * wire, const char * const new_name) {
  assert(wire);
  if(!wire) return RET_INV_PTR;

  if(wire->name) free(wire->name);
  wire->name = strdup(new_name);

  return RET_OK;
}

ret_t lmodel_set_via_name(lmodel_via_t * via, const char * const new_name) {
  assert(via);
  if(!via) return RET_INV_PTR;

  if(via->name) free(via->name);
  via->name = strdup(new_name);

  return RET_OK;

}

/**
 * Set a name for an object from logic model.
 */
ret_t lmodel_set_name(LM_OBJECT_TYPE object_type, void * obj_ptr, const char * const new_name) {
  
  switch(object_type) {
  case LM_TYPE_GATE: 
    return lmodel_set_gate_name((lmodel_gate_t *)obj_ptr, new_name);
    break;
  case LM_TYPE_WIRE: 
    return lmodel_set_wire_name((lmodel_wire_t *)obj_ptr, new_name);
    break;
  case LM_TYPE_VIA: 
    return lmodel_set_via_name((lmodel_via_t *)obj_ptr, new_name);
    break;
  default:
    return RET_ERR;
  }
  return RET_ERR;
}

char * lmodel_get_name(LM_OBJECT_TYPE object_type, void * obj_ptr) {
  
  switch(object_type) {
  case LM_TYPE_GATE: 
    return ((lmodel_gate_t *)obj_ptr)->name;
    break;
  case LM_TYPE_WIRE: 
    return ((lmodel_wire_t *)obj_ptr)->name;
    break;
  case LM_TYPE_VIA: 
    return ((lmodel_via_t *)obj_ptr)->name;
    break;
  default:
    return (char *)"";
  }

  return (char *)"";
}


ret_t lmodel_reset_gate_shape(lmodel_gate_t * gate) {
  assert(gate);
  assert(gate->gate_template);
  if(!gate || !gate->gate_template) return RET_INV_PTR;

  unsigned int master_width = gate->gate_template->master_image_max_x - gate->gate_template->master_image_min_x;
  unsigned int master_height = gate->gate_template->master_image_max_y - gate->gate_template->master_image_min_y;

  gate->max_x = gate->min_x + master_width;
  gate->max_y = gate->min_y + master_height;
  return RET_OK;
}

ret_t lmodel_set_template_for_gate(logic_model_t * lmodel, lmodel_gate_t * gate, lmodel_gate_template_t * tmpl) {
  assert(lmodel);
  assert(gate);
  assert(tmpl);
  if(!lmodel || !gate || !tmpl) return RET_INV_PTR;

  /*
  if(gate->gate_template &&
     gate->min_x == gate->gate_template->master_image_min_x &&
     gate->min_y == gate->gate_template->master_image_min_y &&
     gate->max_x == gate->gate_template->master_image_max_x &&
     gate->max_y == gate->gate_template->master_image_max_y) {

    // gate defines the master template and now there is a new template type
    gate->gate_template->master_image_min_x = 0;
    gate->gate_template->master_image_min_y = 0;
    gate->gate_template->master_image_max_x = 0;
    gate->gate_template->master_image_max_y = 0;
  }
  */

  if(gate->gate_template != NULL) {
    // there is a referenced template
    assert(gate->gate_template->reference_counter > 0);
    gate->gate_template->reference_counter--;
  }
  gate->gate_template = tmpl;

  if(gate->gate_template != NULL) gate->gate_template->reference_counter++;

  return lmodel_update_gate_ports(gate);
}

/** Get the template for a gate */
lmodel_gate_template_t * lmodel_get_template_for_gate(lmodel_gate_t * gate) {
  assert(gate);

  if(!gate) return NULL;
  else return gate->gate_template;
}

/**
 * Check, if a gate is a master gate. This means, if the image is used as template
 * @return 1, if yes, else 0
 */
int lmodel_gate_is_master(const lmodel_gate_t * const gate) {
  assert(gate != NULL);
  if(gate == NULL) return false;

  if(gate->gate_template != NULL &&
     gate->min_x == gate->gate_template->master_image_min_x &&
     gate->min_y == gate->gate_template->master_image_min_y &&
     gate->max_x == gate->gate_template->master_image_max_x &&
     gate->max_y == gate->gate_template->master_image_max_y) return 1;
  else return 0;
  
}

/**
 * Set gates orientation realtive to the template.
 * You can't redefine master gates orientation.
 */
ret_t lmodel_set_gate_orientation(lmodel_gate_t * gate, LM_TEMPLATE_ORIENTATION orientation) {
  assert(gate);
  if(!gate) return RET_INV_PTR;

  if(lmodel_gate_is_master(gate)) {

    // gate defines a template
    if(gate->template_orientation == LM_TEMPLATE_ORIENTATION_UNDEFINED) {
      gate->template_orientation = orientation;
    }
    else if(gate->gate_template != NULL && 
	    orientation == LM_TEMPLATE_ORIENTATION_NORMAL &&
	    gate->gate_template->reference_counter == 1) {
      gate->template_orientation = LM_TEMPLATE_ORIENTATION_NORMAL;
    }
    else
      return RET_ERR;
  }
  else { 
    gate->template_orientation = orientation;
  }
  return RET_OK;
}

LM_TEMPLATE_ORIENTATION lmodel_get_gate_orientation(lmodel_gate_t * gate) {
  assert(gate != NULL);
  if(gate == NULL) return LM_TEMPLATE_ORIENTATION_UNDEFINED;
  return gate->template_orientation;
}

LM_TEMPLATE_ORIENTATION lmodel_apply_transformation_chaining(LM_TEMPLATE_ORIENTATION trans1, 
							     LM_TEMPLATE_ORIENTATION trans2) {
  switch(trans1) {
	  
  case LM_TEMPLATE_ORIENTATION_UNDEFINED:
    return LM_TEMPLATE_ORIENTATION_UNDEFINED;
    break;
	  
  case LM_TEMPLATE_ORIENTATION_NORMAL:
    return trans2;
    break;
	  
  case LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN:

    switch(trans2) {
    case LM_TEMPLATE_ORIENTATION_UNDEFINED:
    case LM_TEMPLATE_ORIENTATION_NORMAL: 
      return trans1;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN:
      return LM_TEMPLATE_ORIENTATION_NORMAL;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT:
      return LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH:
      return LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT;
      break;
    }
    break;

  case LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT:
    switch(trans2) {
    case LM_TEMPLATE_ORIENTATION_UNDEFINED:
    case LM_TEMPLATE_ORIENTATION_NORMAL: 
      return trans1;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN:
      return LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT:
      return LM_TEMPLATE_ORIENTATION_NORMAL;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH:
      return LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN;
      break;
    }
    break;

  case LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH:
    switch(trans2) {
    case LM_TEMPLATE_ORIENTATION_UNDEFINED:
    case LM_TEMPLATE_ORIENTATION_NORMAL: 
      return trans1;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN:
      return LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT:
      return LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH:
      return LM_TEMPLATE_ORIENTATION_NORMAL;
      break;
    }
    break;
  default:
    return LM_TEMPLATE_ORIENTATION_UNDEFINED;
  }

  return LM_TEMPLATE_ORIENTATION_UNDEFINED;
}

/**
 * Each gate "instance" is oriented relative to the master. If you set another gate instance
 * as master and this instance is, let's say flipped up-down relative to the old master, every
 * gate's orientation must be readjusted.
 * @param lmodel The logic model.
 * @param except_for_gate The new master gate. It's orientation is untouched.
 * @param transformation Transformation, that is applied to every gates orientation.
 */
ret_t lmodel_adjust_gate_orientation_for_all_gates(logic_model_t * lmodel, lmodel_gate_t * except_for_gate, 
						   LM_TEMPLATE_ORIENTATION transformation) {

  int layer = -1;
  assert(lmodel);
  assert(except_for_gate);
  assert(lmodel->gate_set);

  if(lmodel == NULL || except_for_gate == NULL || lmodel->gate_set == NULL) return RET_INV_PTR;
  layer = lmodel_get_layer_num_by_type(lmodel, LM_LAYER_TYPE_LOGIC);
  if(layer == -1) return RET_ERR;

  lmodel_gate_set_t * gset_ptr = lmodel->gate_set;
  while(gset_ptr != NULL) {

    lmodel_gate_t * gate = gset_ptr->gate;

    if(gate != except_for_gate) {

      if(gate->gate_template != NULL)
	gate->template_orientation = lmodel_apply_transformation_chaining(gate->template_orientation, transformation);
      
    }
    
    gset_ptr = gset_ptr->next;
  }

  return RET_OK;
}


/** 
 * Remove a gate template from logic model and free corresponding memory.
 */
ret_t lmodel_remove_gate_template(logic_model_t * const lmodel, lmodel_gate_template_t * const tmpl) {
  ret_t ret;
  assert(lmodel);
  assert(tmpl);
  assert(lmodel->gate_template_set);
  if(!lmodel || !tmpl || !lmodel->gate_template_set) return RET_INV_PTR;

  if(RET_IS_NOT_OK(ret = lmodel_remove_refs_to_gate_template(lmodel, tmpl))) return ret;
  
  lmodel_gate_template_set_t * ptr = lmodel->gate_template_set, * ptr_next;
  
  if(ptr->gate == tmpl) {
    // successor?
    lmodel->gate_template_set = ptr->next ? ptr->next : NULL;

    if(RET_IS_NOT_OK(ret = lmodel_destroy_gate_template(ptr->gate))) return ret;
    free(ptr);
    return RET_OK;
  }

  while(ptr->next) {

    if(ptr->next->gate == tmpl) {

      ptr_next = ptr->next->next;

      if(RET_IS_NOT_OK(ret = lmodel_destroy_gate_template(ptr->next->gate))) return ret;
      free(ptr->next);

      ptr->next = ptr_next;
      return RET_OK;
    }
    
    ptr = ptr->next;
  }
  return RET_OK;
}

/**
 * Get a center location for an object.
 * @todo: rename it to lmodel_get_location_for_object()
 */

ret_t lmodel_get_view_for_object(const logic_model_t * const lmodel, 
				 LM_OBJECT_TYPE object_type, 
				 const object_ptr_t * const obj_ptr,
				 unsigned int * center_x, 
				 unsigned int * center_y, 
				 unsigned int * layer) {

  assert(lmodel != NULL);
  assert(obj_ptr != NULL);
  assert(center_x != NULL);
  assert(center_y != NULL);
  assert(layer != NULL);

  unsigned int object_id, sub_id = 0;

  switch(object_type) {
  case LM_TYPE_GATE:
    object_id = ((lmodel_gate_t *) obj_ptr)->id;
    break;
  case LM_TYPE_GATE_PORT:
    object_id = ((lmodel_gate_port_t *) obj_ptr)->gate->id;
    sub_id = ((lmodel_gate_port_t *) obj_ptr)->port_id;
    break;
  case LM_TYPE_WIRE:
    object_id = ((lmodel_wire_t *) obj_ptr)->id;
    break;
  case LM_TYPE_VIA:
    object_id = ((lmodel_via_t *) obj_ptr)->id;
    break;
  case LM_TYPE_UNDEF:
  default:
    return RET_ERR;
  }


  if(RET_IS_OK(lmodel_get_object_by_id(lmodel,
				       object_id, sub_id,
				       NULL, NULL, layer))) {

    lmodel_gate_t * g;
    lmodel_gate_port_t * gp;
    lmodel_wire_t * w;
    lmodel_via_t * v;

    switch(object_type) {
    case LM_TYPE_GATE:
      g = (lmodel_gate_t *) obj_ptr;
      *center_x = g->min_x + (g->max_x - g->min_x) / 2;
      *center_y = g->min_y + (g->max_y - g->min_y) / 2;
      break;
    case LM_TYPE_GATE_PORT:
      gp = (lmodel_gate_port_t *) obj_ptr;
      g = gp->gate;
      *center_x = g->min_x + (g->max_x - g->min_x) / 2;
      *center_y = g->min_y + (g->max_y - g->min_y) / 2;
      break;
    case LM_TYPE_WIRE:
      w = (lmodel_wire_t *) obj_ptr;
      *center_x = MIN(w->from_x, w->to_x) + (MAX(w->from_x, w->to_x) - MIN(w->from_x, w->to_x)) / 2;
      *center_y = MIN(w->from_y, w->to_y) + (MAX(w->from_y, w->to_y) - MIN(w->from_y, w->to_y)) / 2;
      break;
    case LM_TYPE_VIA:
      v = (lmodel_via_t *) obj_ptr;
      *center_x = v->x;
      *center_y = v->y;
      break;
    case LM_TYPE_UNDEF:
    default:
      return RET_ERR;
    }
    return RET_OK;
    
  }

  return RET_ERR;
}

/**
 * Set frame and fill color for a gate template.
 */
ret_t lmodel_gate_template_set_color(lmodel_gate_template_t * gate_template, 
				     color_t fill_color, color_t frame_color) {
  assert(gate_template != NULL);
  if(gate_template == NULL) return RET_INV_PTR;

  gate_template->fill_color = fill_color;
  gate_template->frame_color = frame_color;
  return RET_OK;
}

/**
 * Get fill and frame color for a gate template.
 *
 * @param gate_template The gate template.
 * @param fill_color Pointer to a variable for the fill color.
 * @param frame_color Pointer to a variable for the frame color
 */
ret_t lmodel_gate_template_get_color(lmodel_gate_template_t * gate_template, 
				     color_t * fill_color, color_t * frame_color) {

  assert(gate_template != NULL);
  assert(fill_color != NULL);
  assert(frame_color != NULL);

  if(gate_template == NULL || fill_color == NULL || frame_color == NULL) return RET_INV_PTR;

  *fill_color = gate_template->fill_color;
  *frame_color = gate_template->frame_color;

  return RET_OK;

}


ret_t lmodel_apply_colors_to_ports(logic_model_t * lmodel, const port_color_manager_t * const pcm) {

  assert(lmodel != NULL);
  assert(pcm != NULL);
  if(lmodel == NULL || pcm == NULL) return RET_INV_PTR;

  lmodel_gate_template_set_t * tmpl_ptr = lmodel->gate_template_set;
  lmodel_gate_template_port_t * port_ptr = NULL;

  if(tmpl_ptr == NULL) return RET_OK;

  // reset all colors to 0
  while(tmpl_ptr != NULL) {
    if(tmpl_ptr->gate == NULL) return RET_INV_PTR;
    port_ptr = tmpl_ptr->gate->ports;
    while(port_ptr != NULL) {
      port_ptr->color = 0;
      port_ptr = port_ptr->next;
    }
    tmpl_ptr = tmpl_ptr->next;
  }


  tmpl_ptr = lmodel->gate_template_set;
  while(tmpl_ptr != NULL) {
    if(tmpl_ptr->gate == NULL) return RET_INV_PTR;
    port_ptr = tmpl_ptr->gate->ports;
    while(port_ptr != NULL) {
      port_ptr->color = pcm_get_color_for_port(pcm, port_ptr->port_name);
      port_ptr = port_ptr->next;
    }
    tmpl_ptr = tmpl_ptr->next;
  }

  return RET_OK;
}
