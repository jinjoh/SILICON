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

#ifndef __MAINWIN_H__
#define __MAINWIN_H__

#include <gtkmm.h>
#include "ImageWin.h"
#include "InProgressWin.h"
#include "GridConfigWin.h"
#include "ObjectMatchingWin.h"
#include "ConnectionInspectorWin.h"

#include "lib/alignment_marker.h"
#include "lib/project.h"
#include "lib/plugins.h"
#include <set>
#include <utility>

class MainWin : public Gtk::Window  {
 public:
  MainWin();
  virtual ~MainWin();
  void open_project(Glib::ustring project_dir);
  void set_project_to_open(char * project_dir);

 protected:
  char * project_to_open;

  void zoom(unsigned int center_x, unsigned int center_y, double zoom_factor);
  void center_view(unsigned int center_x, unsigned int center_y, unsigned int layer);
  void set_layer(unsigned int layer);

  bool selected_objects_are_interconnectable();
  bool selected_objects_are_removable();

  void open_popup_menu(GdkEventButton * event);

  //Signal handlers:
  virtual void on_v_adjustment_changed();
  virtual void on_h_adjustment_changed();
  virtual void adjust_scrollbars();
  virtual bool on_drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time);

  virtual void on_menu_project_new();
  virtual void on_menu_project_open();
  virtual void on_menu_project_close();
  virtual void on_menu_project_quit();
  virtual void on_menu_project_save();
  virtual void on_menu_project_settings();
  virtual void on_menu_project_export_archive();
  virtual void on_menu_project_export_view();
  virtual void on_menu_project_export_layer();
  virtual void on_menu_project_recent_projects();
  
  virtual void on_menu_view_zoom_in();
  virtual void on_menu_view_zoom_out();
  virtual void on_menu_view_next_layer();
  virtual void on_menu_view_prev_layer();
  virtual void on_menu_view_grid_config();

  // Layer menu
  virtual void on_menu_layer_import_background();
  virtual void on_menu_layer_set_transistor();
  virtual void on_menu_layer_set_logic();
  virtual void on_menu_layer_set_metal();
  virtual void on_menu_layer_clear_background_image();
  virtual void on_menu_layer_align();

  // Logic menu
  virtual void on_menu_logic_interconnect();
  virtual void on_menu_logic_isolate();
  virtual void on_menu_logic_clear_logic_model();
  virtual void on_menu_logic_clear_logic_model_in_selection();
  virtual void on_menu_logic_connection_inspector();

  // Gate menu
  virtual void on_menu_gate_create_by_selection();
  virtual void on_menu_gate_list();
  virtual void on_menu_gate_set();
  virtual void on_menu_gate_orientation();
  virtual void on_menu_gate_set_as_master();
  virtual void on_menu_gate_remove_gate_by_type();
  virtual void on_menu_gate_remove_gate_by_type_wo_master();
  virtual void on_menu_gate_port_colors();
  
  // Tools menu
  virtual void on_menu_tools_select();
  virtual void on_menu_tools_move();
  virtual void on_menu_tools_wire();
  virtual void on_menu_tools_via_up();
  virtual void on_menu_tools_via_down();

  // Help menu
  virtual void on_menu_help_about();
  virtual void on_menu_others();

  virtual bool on_imgwin_clicked(GdkEventButton * event);
  void object_clicked(unsigned int real_x, unsigned int real_y);
  
  virtual void on_wire_tool_release();
  virtual void on_selection_activated(); // should be renamed to area selection
  virtual void on_selection_revoked();
  virtual void on_mouse_scroll_up(unsigned int center_x, unsigned int center_y);
  virtual void on_mouse_scroll_down(unsigned int center_x, unsigned int center_y);

  virtual void on_goto_object(LM_OBJECT_TYPE object_type, object_ptr_t * obj_ptr);

  bool on_key_press_event_received(GdkEventKey * event);
  bool on_key_release_event_received(GdkEventKey * event);

  //virtual bool on_expose_event(GdkEventExpose * event);
  bool on_idle();
  
  //Child widgets:

  Gtk::VBox m_Box;
  Glib::RefPtr<Gtk::UIManager> m_refUIManager;
  Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;
  Glib::RefPtr<Gtk::RadioAction> 
    m_refChoice_Select, m_refChoice_Move, m_refChoice_Wire, 
    m_refChoice_via_up, m_refChoice_via_down;
  Glib::RefPtr<Gtk::RadioAction> m_refChoice_TransistorLayer, m_refChoice_LogicLayer, m_refChoice_MetalLayer;

  // There is a point where we need to unbind signals. Therefore we store this.
  sigc::connection sig_conn_rbg_transistor;
  sigc::connection sig_conn_rbg_logic;
  sigc::connection sig_conn_rbg_metal;

  Gtk::Statusbar m_statusbar;
  ImageWin imgWin;
  InProgressWin * ipWin;
  ConnectionInspectorWin * ciWin;
  GridConfigWin * gcWin;

  Gtk::HBox m_displayBox;
  Gtk::Adjustment m_VAdjustment;
  Gtk::Adjustment m_HAdjustment;
  Gtk::VScrollbar m_VScrollbar;
  Gtk::HScrollbar m_HScrollbar;
  Gtk::Menu m_Menu_Popup;
  

  project_t * main_project;
  plugin_func_table_t * plugin_func_table;
  ret_t plugin_func_ret_status;

 private:
  bool shift_key_pressed;
  bool project_changed_flag;
  Glib::Thread * thread;
  TOOL tool;
  std::set<std::pair<void *, LM_OBJECT_TYPE> > selected_objects;

  unsigned int last_click_on_real_x, last_click_on_real_y;

  void project_changed();
  void set_project_changed_state(bool new_state);

  void on_popup_menu_lock_region();
  void on_popup_menu_set_alignment_marker(MARKER_TYPE);
  void on_popup_menu_set_name();
  void on_popup_menu_set_port();

  void update_title();
  void add_to_recent_menu();

  void set_menu_item_sensitivity(const Glib::ustring& widget_path, bool state);
  void set_toolbar_item_sensitivity(const Glib::ustring& widget_path, bool state);
  void set_widget_sensitivity(bool state);

  void project_open_thread(Glib::ustring project_dir);
  void background_import_thread(Glib::ustring bg_filename);
  void layer_alignment_thread(double * scaling_x, double * scaling_y, int * shift_x, int * shift_y);
  void algorithm_calc_thread(int slot_pos, plugin_params_t * plugin_params);
  void project_export_thread(const char * const project_dir, const char * const dst_file);

  void on_project_load_finished();
  void on_background_import_finished();
  void on_layer_alignment_finished(double * scaling_x, double * scaling_y, int * shift_x, int * shift_y);
  void on_algorithm_finished(int slot_pos, plugin_params_t * plugin_params);
  void on_export_finished(bool success);
  
  Glib::Dispatcher signal_project_open_finished_;
  Glib::Dispatcher signal_bg_import_finished_;
  Glib::Dispatcher signal_layer_alignment_finished_;
  Glib::Dispatcher * signal_algorithm_finished_;
  sigc::signal<void, bool> signal_export_finished_;

  void update_gui_for_loaded_project();

  void initialize_menu();
  void initialize_image_window();
  void initialize_menu_render_funcs();
  void initialize_menu_algorithm_funcs();
  void set_image_for_toolbar_widget(Glib::ustring toolbar_widget_path, Glib::ustring file_name);

  void on_algorithms_func_clicked(int pos);

  void on_view_info_layer_toggled(int slot_pos);
  void on_grid_config_changed();

  void set_layer_type_in_menu(LAYER_TYPE layer_type);

  void error_dialog(const char * const title, const char * const message);
  void warning_dialog(const char * const title, const char * const message);

  void remove_gate_by_type(GS_DESTROY_MODE destory_mode);


  void update_gui_on_selection_change();
  void clear_selection();
  
};

#endif
