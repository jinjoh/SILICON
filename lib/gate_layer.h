#ifndef __GATE_LAYER_H__
#define __GATE_LAYER_H__

#define MAX_PORTS 256

typedef struct {
  int id;
  char * img;
  char * short_name;
  char * description;
  unsigned int width, height;
  unsigned int num_ports_in, num_ports_out;
  char * in_ports[MAX_PORTS];
  char * out_ports[MAX_PORTS];
} gate_template_t;



gate_template_t * glayer_load_template( unsigned int id,
					const char * const filename, 
					const char * const short_name,
					const char * const description,
					unsigned int num_ports_in, unsigned int num_ports_out);

int glayer_write_template_image(gate_template_t * tmpl, const char * const filename);

#define MAX_ENTRIES_IN_GATE_SET 1000

struct gate_set {
  gate_template_t * set[MAX_ENTRIES_IN_GATE_SET];
  unsigned int num;
};

typedef struct gate_set gate_set_t;

gate_set_t * glayer_create_set();
void glayer_destroy_set(gate_set_t * set);
void glayer_add_template(gate_set_t * set, gate_template_t * tmpl);

gate_set_t * glayer_load_templates(const char * const project_dir, const char * const filename);

#endif
