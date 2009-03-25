#ifndef __IMAGEWIN_H__
#define __IMAGEWIN_H__

#include <gtkmm/drawingarea.h>
#include <gtkmm/tooltips.h>
#include "lib/renderer.h"
#include "lib/logic_model.h"
#include "lib/graphics.h"
#include <list>
#include "gui_globals.h"

class ImageWin : public Gtk::DrawingArea {
 public:
  ImageWin();
  virtual ~ImageWin();
  
  void set_min_x(unsigned int val);
  void set_min_y(unsigned int val);
  void set_max_x(unsigned int val);
  void set_max_y(unsigned int val);

  void set_view(unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y);

  unsigned int get_min_x();
  unsigned int get_min_y();
  unsigned int get_max_x();
  unsigned int get_max_y();

  unsigned int get_width();
  unsigned int get_height();
  unsigned int get_real_width();
  unsigned int get_real_height();

  void coord_screen_to_real(unsigned int screen_x, unsigned int screen_y, 
			    unsigned int * real_x, unsigned int * real_y);
  void coord_real_to_screen(unsigned int real_x, unsigned int real_y, 
			    unsigned int * screen_x, unsigned int * screen_y);

  double get_scaling_x();
  double get_scaling_y();

  void update_screen();

  void zoom_out();
  void zoom_in();

  void set_tool(TOOL tool);

  bool selection_active();
  unsigned int get_selection_min_x();
  unsigned int get_selection_min_y();
  unsigned int get_selection_max_x();
  unsigned int get_selection_max_y();

  unsigned int get_wire_min_x();
  unsigned int get_wire_min_y();
  unsigned int get_wire_max_x();
  unsigned int get_wire_max_y();

  // to access rendering params
  void set_render_logic_model(logic_model_t  * lmodel);
  void set_render_background_images(image_t ** bg_images);
  void set_current_layer(int layer);
  void toggle_render_info_layer(int slot_pos);
  bool get_renderer_func_enabled(int slot_pos);
  std::list<Glib::ustring> get_render_func_names();

  unsigned int get_grid_offset_x();
  unsigned int get_grid_offset_y();
  double get_grid_dist_x();
  double get_grid_dist_y();
  void set_grid_offset_x(unsigned int val);
  void set_grid_offset_y(unsigned int val);
  void set_grid_dist_x(double val);
  void set_grid_dist_y(double val);
  grid_t * get_grid();
  render_params_t * get_render_params();


  sigc::signal<void>& signal_wire_tool_released();
  sigc::signal<void>& signal_selection_activated();
  sigc::signal<void>& signal_selection_revoked();
  sigc::signal<void, unsigned int, unsigned int>& signal_mouse_scroll_down();
  sigc::signal<void, unsigned int, unsigned int>& signal_mouse_scroll_up();
  sigc::signal<void>& signal_adjust_scrollbars();

  bool render_to_file(const char * const filename, unsigned int min_x, unsigned int min_y, unsigned int max_x, unsigned int max_y);

  void reset_selection();

  void set_shift_key_state(bool state);

 protected:

  virtual bool on_expose_event(GdkEventExpose * event);
  virtual bool on_drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time);
  virtual bool on_button_press_event(GdkEventButton * event);
  virtual bool on_button_release_event(GdkEventButton * event);
  virtual bool on_drawarea_scroll(GdkEventScroll * event);
  virtual bool on_motion_notify_event(GdkEventMotion * event);
  virtual bool on_scroll_event (GdkEventScroll* ev);

  virtual bool on_key_press_event(GdkEventKey * event);

 private:

  sigc::signal<void>  signal_wire_tool_released_;
  sigc::signal<void>  signal_selection_activated_;
  sigc::signal<void>  signal_selection_revoked_;
  sigc::signal<void, unsigned int, unsigned int>  signal_mouse_scroll_down_;
  sigc::signal<void, unsigned int, unsigned int>  signal_mouse_scroll_up_;
  sigc::signal<void>  signal_adjust_scrollbars_;

  image_t * rendering_buffer;
  image_t * rendering_buffer_backup;
  renderer_t * renderer;
  render_params_t render_params;
  int current_layer;
  bool shift_key_pressed;

  unsigned int curr_width;
  unsigned int curr_height;

  TOOL tool;
  
  // screen coords
  unsigned int last_click_on_x, last_click_on_y;
  bool mouse_button_pressed;
  bool drag_mode;

  // real coords
  unsigned int selection_x_start, selection_y_start, selection_x_end, selection_y_end;
  unsigned int line_x_start, line_y_start, line_x_end, line_y_end;
  
  // real coords
  unsigned int min_x, max_x, min_y, max_y;


  unsigned int object_selection_x, object_selection_y;
  bool object_selection_active;

  double zoom_level;

  bool in_selection_mode;
  bool in_line_mode;


  void resize_rendering_buffer(unsigned int new_width, unsigned int new_height);
  void draw_selection_box();
  void draw_wire();


  void draw_object_info();
  void reset_wire();
  void setup_renderer();


};

#endif
