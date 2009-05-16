#ifndef __MENUMANAGER_H__
#define __MENUMANAGER_H__

class MenuManager;
class MainWin;

#include <gdkmm/window.h>
#include <gtkmm/menu.h>
#include <gtkmm/stock.h>
#include <libglademm.h>
#include "lib/plugins.h"
#include "lib/logic_model.h"

#include "gui_globals.h"
#include "MainWin.h"


class MenuManager {

 public:

  MenuManager(MainWin * wnd);

  void show_popup_menu(guint button, guint32  activate_time);
  Gtk::Widget* get_menubar();
  Gtk::Widget* get_toolbar();

  void initialize_menu_render_funcs(const std::vector<Glib::ustring> & func_names, 
				    const std::vector<bool> & func_states);
  void initialize_menu_algorithm_funcs(plugin_func_table_t * plugin_func_table);

  void toggle_toolbar_visibility();

  void set_layer_type_in_menu(LAYER_TYPE layer_type);

  const std::vector<bool> toggle_info_layer_visibility();

  bool toggle_info_layer(int slot_pos);

  void set_menu_item_sensitivity(const Glib::ustring& widget_path, bool state);

  void set_widget_sensitivity(bool state);

 private:

  bool toolbar_visible;
  bool info_layers_visible;
  bool info_layers_checkbox_ignore_sig; // if it is true, signals emitted by render-slot-checkboxes are ignored

  MainWin * window;
  Gtk::Menu popup_menu;

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

  std::vector<std::pair<Gtk::CheckMenuItem *, bool> > slot_states;

  void create_popup_menu();
  void create_menu();

  
  void create_and_bind_project_menu();
  void create_and_bind_view_menu();
  void create_and_bind_tools_menu();
  void create_and_bind_logic_menu();
  void create_and_bind_gate_menu();
  void create_and_bind_recognition_menu();
  void create_and_bind_help_menu();

  void set_initial_sensitivity();


  void setup_menu_stucture();
  void set_toolbar_images();

  void set_image_for_toolbar_widget(Glib::ustring toolbar_widget_path, Glib::ustring image_file_name);


  void set_toolbar_item_sensitivity(const Glib::ustring& widget_path, bool state);

  bool toggle_menu_item(Glib::ustring path, bool state, Glib::ustring text1, Glib::ustring text2);

};


#endif
