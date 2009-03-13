#include <gdkmm/window.h>
#include <gtkmm/stock.h>
#include <libglademm.h>

#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <iostream>
#include <unistd.h>

#include "MainWin.h"
#include "ObjectMatchingWin.h"
#include "NewProjectWin.h"
#include "GridConfigWin.h"
#include "GateListWin.h"
#include "GateConfigWin.h"
#include "GateSelectWin.h"
#include "PortSelectWin.h"
#include "InProgressWin.h"
#include "SplashWin.h"
#include "SetNameWin.h"
#include "SetOrientationWin.h"
#include "ProjectSettingsWin.h"
#include "gui_globals.h"

#include "lib/project.h"
#include "lib/logic_model.h"
#include "lib/alignment_marker.h"
#include "lib/plugins.h"

#define TM "MainWin.cc"

#define ZOOM_STEP 1.8

MainWin::MainWin() : 
  m_VAdjustment(0.0, 0.0, 101.0, 0.1, 1.0, 1.0), // value, lower, upper, step_increment, page_increment, page_size
  m_HAdjustment(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
  m_VScrollbar(m_VAdjustment),
  m_HScrollbar(m_HAdjustment) {


  if((plugin_func_table = plugins_init(getenv("DEGATE_PLUGINS"))) == NULL) {
    debug(TM, "Can't load plugins");
    exit(1);
  }

  // setup window
  set_default_size(800, 600);
  char path[PATH_MAX];
  snprintf(path, PATH_MAX, "%s/icons/degate_logo.png", getenv("DEGATE_HOME"));
  set_icon_from_file(path);

  add(m_Box);

  initialize_menu();
  set_widget_sensitivity(false);

  initialize_image_window();

  // setup statusbar
  m_statusbar.push("");
  m_Box.pack_start(m_statusbar, Gtk::PACK_SHRINK);

  // setup popup menu 
  Gtk::Menu::MenuList& menulist = m_Menu_Popup.items();
  //menulist.push_back( Gtk::Menu_Helpers::MenuElem("_Lock", sigc::mem_fun(*this, &MainWin::on_popup_menu_lock_region) ) );
  menulist.push_back( Gtk::Menu_Helpers::MenuElem("Set marker M1 up", 
						  sigc::bind<MARKER_TYPE>(sigc::mem_fun(*this, &MainWin::on_popup_menu_set_alignment_marker),
						  MARKER_TYPE_M1_UP) ));

  menulist.push_back( Gtk::Menu_Helpers::MenuElem("Set marker M2 up", 
						  sigc::bind<MARKER_TYPE>(sigc::mem_fun(*this, &MainWin::on_popup_menu_set_alignment_marker),
						  MARKER_TYPE_M2_UP) ));

  menulist.push_back( Gtk::Menu_Helpers::MenuElem("Set marker M1 down", 
						  sigc::bind<MARKER_TYPE>(sigc::mem_fun(*this, &MainWin::on_popup_menu_set_alignment_marker),
						  MARKER_TYPE_M1_DOWN) ));

  menulist.push_back( Gtk::Menu_Helpers::MenuElem("Set marker M2 down", 
						  sigc::bind<MARKER_TYPE>(sigc::mem_fun(*this, &MainWin::on_popup_menu_set_alignment_marker),
						  MARKER_TYPE_M2_DOWN) ));

  menulist.push_back( Gtk::Menu_Helpers::MenuElem("Set _name for object", 
						  sigc::mem_fun(*this, &MainWin::on_popup_menu_set_name) ));

  menulist.push_back( Gtk::Menu_Helpers::MenuElem("Set port on gate", 
						  sigc::mem_fun(*this, &MainWin::on_popup_menu_set_port) ));

  m_Menu_Popup.accelerate(*this);

  show_all_children();

  main_project = NULL;
  set_project_changed_state(false);
  update_title();
  shift_key_pressed = false;
  imgWin.set_shift_key_state(false);


  if(getuid() == 0) {
    warning_dialog("Security warning", 
		   "You started degate as superuser. I don't cause harm to you. "
		   "But you should think about it. You should know: \"All your base are belong to us\". "
		   "I will not drop privileges and I hope you know, what you do.");
  }
}

MainWin::~MainWin() {
}


void MainWin::update_title() {
  if(!main_project) {
    set_title("degate");
  }
  else {
    char _title[1000];
    snprintf(_title, sizeof(_title), "degate -- [%s%s] [%d/%d]", 
	     main_project->project_dir,
	     project_changed_flag == true ? "*" : "",
	     main_project->current_layer, main_project->num_layers -1);
    set_title(_title);
  }

}

void MainWin::project_changed() {
  set_project_changed_state(true);
}

void MainWin::set_project_changed_state(bool new_state) {
  project_changed_flag = new_state;
  update_title();
}

void MainWin::initialize_menu() {
  // Create actions for menus and toolbars:
  m_refActionGroup = Gtk::ActionGroup::create();

  //Project
  m_refActionGroup->add(Gtk::Action::create("ProjectMenu", "Project"));

  m_refActionGroup->add(Gtk::Action::create("ProjectNew",
					    Gtk::Stock::NEW, "_New", "Create a new project"),
			sigc::mem_fun(*this, &MainWin::on_menu_project_new));

  m_refActionGroup->add(Gtk::Action::create("ProjectOpen",
					    Gtk::Stock::OPEN, "_Open", "Open an existing project"),
			sigc::mem_fun(*this, &MainWin::on_menu_project_open));

  m_refActionGroup->add(Gtk::Action::create("ProjectClose",
					    Gtk::Stock::CLOSE, "_Close", "Close a project"),
			sigc::mem_fun(*this, &MainWin::on_menu_project_close));

  m_refActionGroup->add(Gtk::Action::create("ProjectSave",
					    Gtk::Stock::SAVE, "_Save", "Save a project"),
			sigc::mem_fun(*this, &MainWin::on_menu_project_save));

  m_refActionGroup->add(Gtk::Action::create("ProjectSettings",
					    "Project settings", "Project settings"),
			sigc::mem_fun(*this, &MainWin::on_menu_project_settings));

  m_refActionGroup->add(Gtk::Action::create("ProjectExportArchive",
					    "Create _archive", "Create an archive"),
			sigc::mem_fun(*this, &MainWin::on_menu_project_export_archive));

  m_refActionGroup->add(Gtk::Action::create("ExportViewAsGraphics",
					    "Export current view as XPM-graphics", "Export view as graphics"),
			sigc::mem_fun(*this, &MainWin::on_menu_project_export_view));

  m_refActionGroup->add(Gtk::Action::create("ExportLayerAsGraphics",
					    "Export current layer as XPM-graphics", "Export layer as graphics"),
			sigc::mem_fun(*this, &MainWin::on_menu_project_export_layer));
  
  m_refActionGroup->add(Gtk::Action::create("ProjectQuit", Gtk::Stock::QUIT),
			sigc::mem_fun(*this, &MainWin::on_menu_project_quit));

  // View
  m_refActionGroup->add(Gtk::Action::create("ViewMenu", "View"));

  m_refActionGroup->add(Gtk::Action::create("ViewZoomIn",
					    Gtk::Stock::ZOOM_IN, "Zoom in", "Zoom in"),
			Gtk::AccelKey("plus"),
			sigc::mem_fun(*this, &MainWin::on_menu_view_zoom_in));

  m_refActionGroup->add(Gtk::Action::create("ViewZoomOut",
					    Gtk::Stock::ZOOM_OUT, "Zoom out", "Zoom out"),
			Gtk::AccelKey("minus"),
			sigc::mem_fun(*this, &MainWin::on_menu_view_zoom_out));

  m_refActionGroup->add(Gtk::Action::create("ViewNextLayer",
					    Gtk::Stock::MEDIA_NEXT, "Next Layer", "Next Layer"),
			Gtk::AccelKey("<control>2"),
			sigc::mem_fun(*this, &MainWin::on_menu_view_next_layer));

  m_refActionGroup->add(Gtk::Action::create("ViewPrevLayer",
					    Gtk::Stock::MEDIA_PREVIOUS, "Previous Layer", "Previous Layer"),
			Gtk::AccelKey("<control>1"),
			sigc::mem_fun(*this, &MainWin::on_menu_view_prev_layer));

  m_refActionGroup->add(Gtk::Action::create("ViewGridConfig", "Grid configuration", "Grid configuration"),
			Gtk::AccelKey("<control>G"),
			sigc::mem_fun(*this, &MainWin::on_menu_view_grid_config));

  /*  m_refActionGroup->add(Gtk::Action::create("ViewDistanceToColor", "Define color for similarity filter", 
					    "Define color for similarity filter"),
			sigc::mem_fun(*this, &MainWin::on_menu_view_distance_to_color));
  */

  // Edit menu:

  // Choices menu, to demonstrate Radio items
  m_refActionGroup->add( Gtk::Action::create("ToolsMenu", "Tools") );
  Gtk::RadioAction::Group group_tools;
  
  m_refChoice_Select = Gtk::RadioAction::create(group_tools, "ToolSelect", "Select");
  m_refActionGroup->add(m_refChoice_Select, sigc::mem_fun(*this, &MainWin::on_menu_tools_select) );

  m_refChoice_Select = Gtk::RadioAction::create(group_tools, "ToolMove", "Move");
  m_refActionGroup->add(m_refChoice_Select, sigc::mem_fun(*this, &MainWin::on_menu_tools_move) );

  m_refChoice_Wire = Gtk::RadioAction::create(group_tools, "ToolWire", "Wire");
  m_refActionGroup->add(m_refChoice_Wire, sigc::mem_fun(*this, &MainWin::on_menu_tools_wire) );

  m_refChoice_via_up = Gtk::RadioAction::create(group_tools, "ToolViaUp", "Via up");
  m_refActionGroup->add(m_refChoice_via_up, sigc::mem_fun(*this, &MainWin::on_menu_tools_via_up) );

  m_refChoice_via_down = Gtk::RadioAction::create(group_tools, "ToolViaDown", "Via down");
  m_refActionGroup->add(m_refChoice_via_down, sigc::mem_fun(*this, &MainWin::on_menu_tools_via_down) );

  // Layer Menu
  m_refActionGroup->add( Gtk::Action::create("LayerMenu", "Layer"));

  m_refActionGroup->add(Gtk::Action::create("LayerImportBackground",
					    "_Import background patches", 
					    "Import background for current layer"),
			sigc::mem_fun(*this, &MainWin::on_menu_layer_import_background));

  m_refActionGroup->add(Gtk::Action::create("LayerClearBackgroundImage",
					    Gtk::Stock::CLEAR, "Clear background image", 
					    "Clear background image for current layer"),
			sigc::mem_fun(*this, &MainWin::on_menu_layer_clear_background_image));

  m_refActionGroup->add(Gtk::Action::create("LayerClearLogicModel",
					    Gtk::Stock::CLEAR, "Clear logic model", 
					    "Clear logic model for current layer"),
			sigc::mem_fun(*this, &MainWin::on_menu_layer_clear_logic_model));

  m_refActionGroup->add(Gtk::Action::create("LayerClearLogicModelInSelection",
					    Gtk::Stock::CLEAR, "Remove selected objects", 
					    "Remove selected Object"),
			Gtk::AccelKey("<control>C"),
			sigc::mem_fun(*this, &MainWin::on_menu_layer_clear_logic_model_in_selection));

  m_refActionGroup->add(Gtk::Action::create("LayerAlignment",
					    "Align layers", 
					    "Align layers"),
			sigc::mem_fun(*this, &MainWin::on_menu_layer_align));

  Gtk::RadioAction::Group group_layer_type;
  m_refActionGroup->add(Gtk::Action::create("LayerType", "Layer type"));
  
  m_refChoice_TransistorLayer = Gtk::RadioAction::create(group_layer_type, "LayerTypeTransistor", "Transistor layer");
  m_refActionGroup->add(m_refChoice_TransistorLayer);
  sig_conn_rbg_transistor = m_refChoice_TransistorLayer->signal_activate().connect(sigc::mem_fun(*this, &MainWin::on_menu_layer_set_transistor));
  
  m_refChoice_LogicLayer = Gtk::RadioAction::create(group_layer_type, "LayerTypeLogic", "Logic layer");
  //m_refActionGroup->add(m_refChoice_LogicLayer, sigc::mem_fun(*this, &MainWin::on_menu_layer_set_logic));
  m_refActionGroup->add(m_refChoice_LogicLayer);
  sig_conn_rbg_logic = m_refChoice_LogicLayer->signal_activate().connect(sigc::mem_fun(*this, &MainWin::on_menu_layer_set_logic), true); 

  m_refChoice_MetalLayer = Gtk::RadioAction::create(group_layer_type, "LayerTypeMetal", "Metal layer");
  //m_refActionGroup->add(m_refChoice_MetalLayer, sigc::mem_fun(*this, &MainWin::on_menu_layer_set_metal));
  m_refActionGroup->add(m_refChoice_MetalLayer);
  sig_conn_rbg_metal = m_refChoice_MetalLayer->signal_activate().connect(sigc::mem_fun(*this, &MainWin::on_menu_layer_set_metal));

  // Gate menu
  m_refActionGroup->add( Gtk::Action::create("GateMenu", "Gate"));
  m_refActionGroup->add(Gtk::Action::create("GateCreateBySelection",
					    "Create gate by selection", 
					    "Create a new gate"),
			sigc::mem_fun(*this, &MainWin::on_menu_gate_create_by_selection));

  m_refActionGroup->add(Gtk::Action::create("GateList",
					    "List gates", 
					    "List available gate"),
			sigc::mem_fun(*this, &MainWin::on_menu_gate_list));

  m_refActionGroup->add(Gtk::Action::create("GateSet",
					    "Set gate for selection", 
					    "Set gate for selection"),
			sigc::mem_fun(*this, &MainWin::on_menu_gate_set));

  m_refActionGroup->add(Gtk::Action::create("GateOrientation",
					    "Set gate orientation", 
					    "Set gate orientation"),
			sigc::mem_fun(*this, &MainWin::on_menu_gate_orientation));

  m_refActionGroup->add(Gtk::Action::create("GateSetAsMaster",
					    "Set gate as master", 
					    "Set gate as master"),
			sigc::mem_fun(*this, &MainWin::on_menu_gate_set_as_master));

  // Algorithms menu
  m_refActionGroup->add(Gtk::Action::create("AlgorithmsMenu", "Algorithms"));


  // Help menu:
  m_refActionGroup->add( Gtk::Action::create("HelpMenu", "Help") );
  m_refActionGroup->add( Gtk::Action::create("HelpAbout", Gtk::Stock::ABOUT, "About"),
			 sigc::mem_fun(*this, &MainWin::on_menu_help_about) );
  
  m_refUIManager = Gtk::UIManager::create();
  m_refUIManager->insert_action_group(m_refActionGroup);
  
  add_accel_group(m_refUIManager->get_accel_group());
  
  // Layout the actions in a menubar and toolbar:
  Glib::ustring ui_info = 
        "<ui>"
        "  <menubar name='MenuBar'>"
        "    <menu action='ProjectMenu'>"
        "      <menuitem action='ProjectNew'/>"
        "      <menuitem action='ProjectOpen'/>"
        "      <menuitem action='ProjectClose'/>"
        "      <menuitem action='ProjectSave'/>"
        "      <menuitem action='ProjectExportArchive'/>"
        "      <separator/>"
        "      <menuitem action='ProjectSettings'/>"
        "      <separator/>"
        "      <menuitem action='ExportViewAsGraphics'/>"
        "      <menuitem action='ExportLayerAsGraphics'/>"
        "      <separator/>"
        "      <menuitem action='ProjectQuit'/>"
        "    </menu>"
        "    <menu action='ViewMenu'>"
        "      <menuitem action='ViewZoomIn'/>"
        "      <menuitem action='ViewZoomOut'/>"
        "      <separator/>"
        "      <menuitem action='ViewNextLayer'/>"
        "      <menuitem action='ViewPrevLayer'/>"
        "      <separator/>"
        "      <menuitem action='ViewGridConfig'/>"  
        "    </menu>"
        "    <menu action='ToolsMenu'>"
        "      <menuitem action='ToolSelect'/>"
        "      <menuitem action='ToolMove'/>"
        "      <menuitem action='ToolWire'/>"
        "      <menuitem action='ToolViaUp'/>"
        "      <menuitem action='ToolViaDown'/>"
        "    </menu>"

        "    <menu action='LayerMenu'>"
        "      <menuitem action='LayerImportBackground'/>"
        "      <menuitem action='LayerClearBackgroundImage'/>"
        "      <separator/>"
        "      <menuitem action='LayerClearLogicModel'/>"
        "      <menuitem action='LayerClearLogicModelInSelection'/>"
        "      <separator/>"
        "      <menuitem action='LayerAlignment'/>"
        "      <separator/>"
        "      <menu action='LayerType'>"
        "        <menuitem action='LayerTypeTransistor'/>"
        "        <menuitem action='LayerTypeLogic'/>"
        "        <menuitem action='LayerTypeMetal'/>"
        "      </menu>"
        "    </menu>"
        "    <menu action='GateMenu'>"
        "      <menuitem action='GateCreateBySelection'/>"
        "      <menuitem action='GateSet'/>"
        "      <menuitem action='GateOrientation'/>"
        "      <menuitem action='GateSetAsMaster'/>"
        "      <separator/>"
        "      <menuitem action='GateList'/>"
        "    </menu>"
        "    <menu action='AlgorithmsMenu'/>"
        "    <menu action='HelpMenu'>"
        "      <menuitem action='HelpAbout'/>"
        "    </menu>"
        "  </menubar>"
        "  <toolbar  name='ToolBar'>"
        "    <toolitem action='ProjectQuit'/>"
        "    <separator/>"
        "    <toolitem action='ViewZoomIn'/>"
        "    <toolitem action='ViewZoomOut'/>"
        "    <separator/>"
        "    <toolitem action='ToolSelect'/>"
        "    <toolitem action='ToolMove'/>"
        "    <toolitem action='ToolWire'/>"
        "    <toolitem action='ToolViaUp'/>"
        "    <toolitem action='ToolViaDown'/>"
        "  </toolbar>"
    "</ui>";

#ifdef GLIBMM_EXCEPTIONS_ENABLED
  try {
    m_refUIManager->add_ui_from_string(ui_info);
  }
  catch(const Glib::Error& ex) {
    std::cerr << "building menus failed: " <<  ex.what();
  }
  #else
  std::auto_ptr<Glib::Error> ex;
  m_refUIManager->add_ui_from_string(ui_info, ex);
  if(ex.get()) {
    std::cerr << "building menus failed: " <<  ex->what();
  }
#endif //GLIBMM_EXCEPTIONS_ENABLED

  // Get the menubar and toolbar widgets, and add them to a container widget:
  Gtk::Widget* pMenubar = m_refUIManager->get_widget("/MenuBar");
  if(pMenubar) m_Box.pack_start(*pMenubar, Gtk::PACK_SHRINK);

  Gtk::Widget* pToolbar = m_refUIManager->get_widget("/ToolBar") ;
  if(pToolbar) {
    m_Box.pack_start(*pToolbar, Gtk::PACK_SHRINK);

    Gtk::ToolButton* pToolbarItem;
    pToolbarItem = dynamic_cast<Gtk::ToolButton*>(m_refUIManager->get_widget("/ToolBar/ToolSelect"));
    pToolbarItem->set_stock_id(Gtk::Stock::COLOR_PICKER);
    /*
    Gtk::Image * m_ImageSelect = Gtk::manage(new Gtk::Image("gui/icons/icon_select.xpm"));
    pToolbarItem->set_icon_widget(*m_ImageSelect);
    pToolbarItem->set_label("foo"); */

    pToolbarItem = dynamic_cast<Gtk::ToolButton*>(m_refUIManager->get_widget("/ToolBar/ToolMove"));
    pToolbarItem->set_stock_id(Gtk::Stock::SELECT_COLOR);

    pToolbarItem = dynamic_cast<Gtk::ToolButton*>(m_refUIManager->get_widget("/ToolBar/ToolWire"));
    pToolbarItem->set_stock_id(Gtk::Stock::REMOVE);

    pToolbarItem = dynamic_cast<Gtk::ToolButton*>(m_refUIManager->get_widget("/ToolBar/ToolViaUp"));
    pToolbarItem->set_stock_id(Gtk::Stock::YES);

    pToolbarItem = dynamic_cast<Gtk::ToolButton*>(m_refUIManager->get_widget("/ToolBar/ToolViaDown"));
    pToolbarItem->set_stock_id(Gtk::Stock::NO);

    /*
    Glib::RefPtr<Gdk::Bitmap> _image1_mask; 
    Glib::RefPtr<Gdk::Pixmap> _image1_pixmap = 
      Gdk::Pixmap::create_from_xpm(get_default_colormap(), _image1_mask, myImage_xpm); 
    Gtk::Image *image1 = manage(new class Gtk::Image(_image1_pixmap, _image1_mask)); 
    toolbar1->tools().push_back (Gtk::Toolbar_Helpers::ButtonElem("Button Text", *image1, 
								  sigc::mem_fun(*this, &MainWin::on_menu_tools_wire),
								  "tooltip text")); 

    */
    // m_refChoice_Wire = Gtk::RadioAction::create(group_tools, "ToolWire", "Wire");
    // m_refActionGroup->add(m_refChoice_Wire, sigc::mem_fun(*this, &MainWin::on_menu_tools_wire) );
  }

  set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerClearLogicModelInSelection", false);
  set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerAlignment", false);

  set_menu_item_sensitivity("/MenuBar/GateMenu/GateCreateBySelection", false);
  set_menu_item_sensitivity("/MenuBar/GateMenu/GateSet", false);
  set_menu_item_sensitivity("/MenuBar/GateMenu/GateOrientation", false);
  set_menu_item_sensitivity("/MenuBar/GateMenu/GateSetAsMaster", false);
  set_menu_item_sensitivity("/MenuBar/GateMenu/GateList", false);

  initialize_menu_render_funcs();
  initialize_menu_algorithm_funcs();

  signal_key_press_event().connect(sigc::mem_fun(*this,&MainWin::on_key_press_event_received), false);
  signal_key_release_event().connect(sigc::mem_fun(*this,&MainWin::on_key_release_event_received), false);

}

void MainWin::initialize_menu_algorithm_funcs() {

  Gtk::MenuItem * pMenuItem = dynamic_cast<Gtk::MenuItem*>(m_refUIManager->get_widget("/MenuBar/AlgorithmsMenu"));
  assert(pMenuItem != NULL);

  if(pMenuItem) {

    Gtk::Menu* pMenu = pMenuItem->get_submenu(); 
    assert(pMenu != NULL);

    if(pMenu) {
      (pMenu->items()).remove(pMenu->items()[1]); // XXX

      // create

      int i = 0;
      const char * name;
      while((name = plugin_get_func_description(plugin_func_table, i)) != NULL) {
	Gtk::MenuItem *menuItem = new Gtk::MenuItem(name);
	menuItem->show();
	menuItem->signal_activate().connect(sigc::bind<int>(sigc::mem_fun(*this, &MainWin::on_algorithms_func_clicked), i));
    
	pMenu->add(*menuItem);
	pMenu->show();
	pMenuItem->show();

	i++;
      }
    }
  }
}

void MainWin::initialize_menu_render_funcs() {
  Gtk::MenuItem * pMenuItem = dynamic_cast<Gtk::MenuItem*>(m_refUIManager->get_widget("/MenuBar/ViewMenu/ViewPrevLayer"));
  if(pMenuItem) {
    Gtk::Menu * pMenu = dynamic_cast<Gtk::Menu*>(pMenuItem->get_parent());
    assert(pMenu != NULL);

    if(pMenu) {

      Gtk::SeparatorMenuItem * sepMenuItem = new Gtk::SeparatorMenuItem();
      pMenu->append(*sepMenuItem);
      sepMenuItem->show();

      std::list<Glib::ustring> func_names = imgWin.get_render_func_names();
      std::list<Glib::ustring>::iterator i;
      int slot_pos = 0;
      for(i = func_names.begin(); i != func_names.end(); i++, slot_pos++) {
	bool rf_initial_state = imgWin.get_renderer_func_enabled(slot_pos);
	Gtk::CheckMenuItem *menuItem = new Gtk::CheckMenuItem(*i, true);
	menuItem->set_active(rf_initial_state);
	pMenu->append(*menuItem);
	menuItem->show();
	menuItem->signal_toggled().connect(sigc::bind<int>(sigc::mem_fun(*this, &MainWin::on_view_info_layer_toggled), slot_pos));
      }

      
    }
  }

}


void MainWin::on_view_info_layer_toggled(int slot_pos) {
  imgWin.toggle_render_info_layer(slot_pos);
  imgWin.update_screen();
}

void MainWin::initialize_image_window() {
  m_VScrollbar.set_update_policy(Gtk::UPDATE_CONTINUOUS);
  m_HScrollbar.set_update_policy(Gtk::UPDATE_CONTINUOUS);
  
  m_VAdjustment.signal_value_changed().connect(sigc::mem_fun(*this, &MainWin::on_v_adjustment_changed));
  m_HAdjustment.signal_value_changed().connect(sigc::mem_fun(*this, &MainWin::on_h_adjustment_changed));

  imgWin.signal_drag_motion().connect(sigc::mem_fun(*this, &MainWin::on_drag_motion));
  imgWin.signal_button_press_event().connect(sigc::mem_fun(*this, &MainWin::on_imgwin_clicked));
  imgWin.signal_wire_tool_released().connect(sigc::mem_fun(*this, &MainWin::on_wire_tool_release));
  imgWin.signal_selection_activated().connect(sigc::mem_fun(*this, &MainWin::on_selection_activated));
  imgWin.signal_selection_revoked().connect(sigc::mem_fun(*this, &MainWin::on_selection_revoked));
  imgWin.signal_mouse_scroll_up().connect(sigc::mem_fun(*this, &MainWin::on_mouse_scroll_up));
  imgWin.signal_mouse_scroll_down().connect(sigc::mem_fun(*this, &MainWin::on_mouse_scroll_down));
  imgWin.signal_adjust_scrollbars().connect(sigc::mem_fun(*this, &MainWin::adjust_scrollbars));

  m_displayBox.pack_start(imgWin, Gtk::PACK_EXPAND_WIDGET);
  m_displayBox.pack_start(m_VScrollbar, Gtk::PACK_SHRINK);

  m_Box.pack_start(m_displayBox, Gtk::PACK_EXPAND_WIDGET);

  m_Box.pack_start(m_HScrollbar, Gtk::PACK_SHRINK);

  tool = TOOL_SELECT;
  imgWin.set_tool(tool);
}

void MainWin::set_widget_sensitivity(bool state) {

  set_toolbar_item_sensitivity("/ToolBar/ViewZoomIn", state);
  set_toolbar_item_sensitivity("/ToolBar/ViewZoomOut", state);

  set_toolbar_item_sensitivity("/ToolBar/ToolSelect", state);
  set_toolbar_item_sensitivity("/ToolBar/ToolMove", state);
  set_toolbar_item_sensitivity("/ToolBar/ToolWire", state);
  set_toolbar_item_sensitivity("/ToolBar/ToolViaUp", state);
  set_toolbar_item_sensitivity("/ToolBar/ToolViaDown", state);

  set_menu_item_sensitivity("/MenuBar/ProjectMenu/ProjectClose", state);
  set_menu_item_sensitivity("/MenuBar/ProjectMenu/ProjectSave", state);
  set_menu_item_sensitivity("/MenuBar/ProjectMenu/ProjectExportArchive", state);
  set_menu_item_sensitivity("/MenuBar/ProjectMenu/ProjectSettings", state);
  set_menu_item_sensitivity("/MenuBar/ProjectMenu/ExportViewAsGraphics", state);
  set_menu_item_sensitivity("/MenuBar/ProjectMenu/ExportLayerAsGraphics", state);

  set_menu_item_sensitivity("/MenuBar/ViewMenu/ViewZoomIn", state);
  set_menu_item_sensitivity("/MenuBar/ViewMenu/ViewZoomOut", state);
  set_menu_item_sensitivity("/MenuBar/ViewMenu/ViewNextLayer", state);
  set_menu_item_sensitivity("/MenuBar/ViewMenu/ViewPrevLayer", state);

  set_menu_item_sensitivity("/MenuBar/ToolsMenu/ToolSelect", state);
  set_menu_item_sensitivity("/MenuBar/ToolsMenu/ToolMove", state);
  set_menu_item_sensitivity("/MenuBar/ToolsMenu/ToolWire", state);
  set_menu_item_sensitivity("/MenuBar/ToolsMenu/ToolViaUp", state);
  set_menu_item_sensitivity("/MenuBar/ToolsMenu/ToolViaDown", state);

  set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerImportBackground", state);
  set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerClearBackgroundImage", state);
  set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerClearLogicModel", state);
  set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerType", state);


  if(state == false)
    set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerAlignment", false);
  else 
    if(main_project)
      set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerAlignment", 
				amset_complete(main_project->alignment_marker_set) ? true : false);

  set_menu_item_sensitivity("/MenuBar/GateMenu/GateList", state);
}

void MainWin::set_menu_item_sensitivity(const Glib::ustring& widget_path, bool state) {
  Gtk::MenuItem * pItem = dynamic_cast<Gtk::MenuItem*>(m_refUIManager->get_widget(widget_path));
#ifdef DEBUG
  if(!pItem) std::cout << "widget lookup failed for path: " << widget_path << std::endl;
#endif
  assert(pItem != NULL);
  if(pItem) pItem->set_sensitive(state);
}

void MainWin::set_toolbar_item_sensitivity(const Glib::ustring& widget_path, bool state) {
  Gtk::Widget * pItem = m_refUIManager->get_widget(widget_path);
#ifdef DEBUG
  if(!pItem) std::cout << "widget lookup failed for path: " << widget_path << std::endl;
#endif
  assert(pItem != NULL);
  if(pItem) pItem->set_sensitive(state);
}

bool MainWin::on_drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time) {
  return true;
}

void MainWin::on_menu_project_quit() {
  on_menu_project_close();
  hide(); //Closes the main window to stop the Gtk::Main::run().
}

void MainWin::on_menu_project_new() {

  if(main_project) on_menu_project_close();

  NewProjectWin npw_dialog(this);
  Gtk::Main::run(npw_dialog);
  unsigned int width = npw_dialog.get_width();
  unsigned int height = npw_dialog.get_height();
  unsigned int layers = npw_dialog.get_layers();

  if(width == 0 || height == 0 || layers == 0) {
    Gtk::MessageDialog dialog(*this, "Invalid value", true, Gtk::MESSAGE_ERROR);
    dialog.set_title("The values you entered are invalid.");
    dialog.run();
  }
  else {
    Gtk::FileChooserDialog dialog("Please choose a project folder", Gtk::FILE_CHOOSER_ACTION_CREATE_FOLDER);
    dialog.set_transient_for(*this);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button("Select", Gtk::RESPONSE_OK);
    
    int result = dialog.run();
    Glib::ustring project_dir = dialog.get_filename();
    dialog.hide();

    // create the project

    switch(result) {
    case(Gtk::RESPONSE_OK):
      if((project_init_directory(project_dir.c_str(), 0) == 0) ||
	 ((main_project = project_create(project_dir.c_str(), width, height, layers)) == NULL) ||
	 (project_map_background_memfiles(main_project) == 0)) {

	Gtk::MessageDialog dialog(*this, "Error: Can't create new project", true, Gtk::MESSAGE_ERROR);
	dialog.set_title("Initialization of the new project failed.");
	dialog.run();
      }
      else {
	update_gui_for_loaded_project();
	set_project_changed_state(false);
      }
      on_menu_project_save();

      break;
    case(Gtk::RESPONSE_CANCEL):
      break;
    }

  }

}

void MainWin::on_menu_project_close() {
  if(main_project) {

    if(project_changed_flag == true) {

      Gtk::MessageDialog dialog(*this, "Project data was modified. Should it be saved?", 
				true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
      dialog.set_title("Warning");      
      if(dialog.run() == Gtk::RESPONSE_YES) {
	on_menu_project_save();
      }
    }

    imgWin.set_render_logic_model(NULL);
    imgWin.set_render_background_images(NULL);
    imgWin.set_current_layer(-1);
    
    project_destroy(main_project);
    main_project = NULL;
    imgWin.update_screen();    

    update_title();
    set_widget_sensitivity(false);
  }
}


void MainWin::on_menu_project_settings() {
  if(main_project) {
    ProjectSettingsWin psWin(this, main_project);
    if(psWin.run()) {
      project_changed();
    }
  }
}

void MainWin::on_menu_project_export_archive() {
  if(main_project) {
    if(project_changed_flag == true) {
      Gtk::MessageDialog dialog_ask(*this, "Project data was modified. Should it be saved?", 
				    true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
      dialog_ask.set_title("Warning");      
      if(dialog_ask.run() == Gtk::RESPONSE_YES) {
	on_menu_project_save();
      }
      else return;
    }
    Gtk::FileChooserDialog dialog("Export project as archive", Gtk::FILE_CHOOSER_ACTION_SAVE );
    dialog.set_transient_for(*this);

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button("Select", Gtk::RESPONSE_OK);

    time_t tim = time(NULL);
    tm *now = localtime(&tim);
    char filename_sugg[PATH_MAX];
    char * name = rindex(main_project->project_dir, '/');
    if(name && name + 1 < main_project->project_dir + strlen(main_project->project_dir)) name++;
    
    snprintf(filename_sugg, PATH_MAX, "%4d-%02d-%02d_%02d%02d_%s.zip", 
	     now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min,
	     name);
    
    //dialog.unselect_all();
    //dialog.select_filename(filename_sugg);
    dialog.set_current_name(filename_sugg);
    int result = dialog.run();
    dialog.hide();

    switch(result) {
    case Gtk::RESPONSE_OK:

      ipWin = new InProgressWin(this, "Exporting", "Please wait while exporting project.");
      ipWin->show();

      signal_export_finished_.connect(sigc::mem_fun(*this, &MainWin::on_export_finished));
      thread = Glib::Thread::create(sigc::bind<const char * const, const char * const>(sigc::mem_fun(*this, &MainWin::project_export_thread), 
										       main_project->project_dir, dialog.get_filename().c_str() ), false);


      break;
    case Gtk::RESPONSE_CANCEL:
      break;
    }
    
  }
}

void MainWin::project_export_thread(const char * const project_dir, const char * const dst_file) {

  pid_t pid = fork();
  if(pid == 0) {
    // child
    if(execlp("zip", "-j", "-r", dst_file, project_dir, NULL) == -1) {
      debug(TM, "exec failed");
    }
    debug(TM, "sth. failed");
    exit(1);
  }
  else if(pid < 0) {
    // fork failed
    debug(TM, "fork() failed");
    signal_export_finished_(false);
  }
  else {
    // parent
    int exit_code;
    if(waitpid(pid, &exit_code, 0) != pid) {
      debug(TM, "failed");
      signal_export_finished_(false);
    }
    else {
      signal_export_finished_(WEXITSTATUS(exit_code) == 0 ? true : false);
    }
  }
}


void MainWin::on_export_finished(bool success) {
  if(ipWin) {
    ipWin->close();
    delete ipWin;
    ipWin = NULL;
  }
  if(success == false) {
    error_dialog("Error", "Export failed. Maybe you have no zip utility installed?");
  }
}

void MainWin::on_menu_project_save() {
  if(main_project) {
    if(!project_save(main_project, imgWin.get_render_params())) {
      Gtk::MessageDialog dialog(*this, "Can't save project.", true, Gtk::MESSAGE_ERROR);
      dialog.set_title("Error");
      dialog.run();
    }
    project_changed_flag = false;
    update_title();
  }
}

void MainWin::on_menu_project_open() {

  if(main_project) on_menu_project_close();

  Gtk::FileChooserDialog dialog("Please choose a project folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  dialog.set_transient_for(*this);

  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button("Select", Gtk::RESPONSE_OK);

  int result = dialog.run();
  Glib::ustring project_dir = dialog.get_filename();
  dialog.hide();


  switch(result) {
  case Gtk::RESPONSE_OK:

    ipWin = new InProgressWin(this, "Opening Project", "Please wait while opening project.");
    ipWin->show();

    signal_project_open_finished_.connect(sigc::mem_fun(*this, &MainWin::on_project_load_finished));
    thread = Glib::Thread::create(sigc::bind<const Glib::ustring>(sigc::mem_fun(*this, &MainWin::project_open_thread), project_dir), false);

    break;
  case Gtk::RESPONSE_CANCEL:
    break;
  }
}

// in GUI-thread
void MainWin::on_project_load_finished() {

  //thread->join();
  
  if(ipWin) {
    ipWin->close();
    delete ipWin;
    ipWin = NULL;
  }
  
  if(!main_project) {
    Gtk::MessageDialog err_dialog(*this, "Can't open project", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
    err_dialog.run();
  }
  else {
    set_layer_type_in_menu(lmodel_get_layer_type(main_project->lmodel, main_project->current_layer));
    update_gui_for_loaded_project();
  }
}

void MainWin::project_open_thread(Glib::ustring project_dir) {
  main_project = project_load(project_dir.c_str(), imgWin.get_render_params());
  signal_project_open_finished_();
}

void MainWin::on_menu_project_export_view() {
  if(main_project) {
    Gtk::FileChooserDialog dialog("Please choose a file name", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button("Export", Gtk::RESPONSE_OK);
    int result = dialog.run();
    Glib::ustring filename = dialog.get_filename();
    dialog.hide();

    if(result == Gtk::RESPONSE_OK)
      if(!imgWin.render_to_file(filename.c_str(), imgWin.get_min_x(),
				imgWin.get_min_y(),
				imgWin.get_max_x(),
				imgWin.get_max_y())) {
	Gtk::MessageDialog err_dialog(*this, "Can't export graphics", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	err_dialog.run();
      }
  }   
}

void MainWin::on_menu_project_export_layer() {
  if(main_project) {
    Gtk::FileChooserDialog dialog("Please choose a file name", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button("Export", Gtk::RESPONSE_OK);
    int result = dialog.run();
    Glib::ustring filename = dialog.get_filename();
    dialog.hide();

    if(result == Gtk::RESPONSE_OK)
      if(!imgWin.render_to_file(filename.c_str(), 0, 0, main_project->width, main_project->height)) {
	Gtk::MessageDialog err_dialog(*this, "Can't export graphics", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	err_dialog.run();
      }
  }   
}

void MainWin::update_gui_for_loaded_project() {

  if(main_project) {
    
    imgWin.set_view(0, 0, imgWin.get_width(), imgWin.get_height());
    
    imgWin.set_render_logic_model(main_project->lmodel);
    imgWin.set_render_background_images(main_project->bg_images);
    imgWin.reset_selection();
    imgWin.set_current_layer(0);
    
    set_widget_sensitivity(true);
    update_title();

    adjust_scrollbars();

    render_params_t * render_params = imgWin.get_render_params();
    render_params->alignment_marker_set = main_project->alignment_marker_set;

    imgWin.update_screen();  
  }
}


void MainWin::on_menu_view_next_layer() {
  if(main_project->num_layers == 0) return;

  if(main_project->current_layer < main_project->num_layers - 1)
    main_project->current_layer++;
  else 
    main_project->current_layer = 0;

  imgWin.set_current_layer(main_project->current_layer);
  
  set_layer_type_in_menu(lmodel_get_layer_type(main_project->lmodel, main_project->current_layer));

  selected_objects.erase(selected_objects.begin(), selected_objects.end());
  set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerClearLogicModelInSelection", false);

  update_title();
  imgWin.update_screen();
}

void MainWin::on_menu_view_prev_layer() {
  if(main_project->num_layers == 0) return;

  if(main_project->current_layer == 0)
    main_project->current_layer = main_project->num_layers - 1;
  else 
    main_project->current_layer--;
  imgWin.set_current_layer(main_project->current_layer);
  set_layer_type_in_menu(lmodel_get_layer_type(main_project->lmodel, main_project->current_layer));

  selected_objects.erase(selected_objects.begin(), selected_objects.end());
  set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerClearLogicModelInSelection", false);

  update_title();
  imgWin.update_screen();
}

void MainWin::on_menu_view_zoom_in() {
  if(!main_project) return;
  unsigned int delta_x = imgWin.get_max_x() - imgWin.get_min_x();
  unsigned int delta_y = imgWin.get_max_y() - imgWin.get_min_y();

  unsigned int center_x = imgWin.get_min_x() + (delta_x >> 1);
  unsigned int center_y = imgWin.get_min_y() + (delta_y >> 1);

  zoom_in(imgWin.get_real_width() > main_project->width ? main_project->width /2: center_x, 
	  imgWin.get_real_height() > main_project->height ? main_project->height / 2 : center_y);
}

void MainWin::zoom_in(unsigned int center_x, unsigned int center_y) {

  if(!main_project) return;
  double delta_x = imgWin.get_max_x() - imgWin.get_min_x();
  double delta_y = imgWin.get_max_y() - imgWin.get_min_y();

  if(delta_x < 100 || delta_y < 100) return;

  double factor = 1/ZOOM_STEP;
  double min_x = (double)center_x - factor * (delta_x/2.0);
  double min_y = (double)center_y - factor * (delta_y/2.0);
  if(min_x < 0) min_x = 0;
  if(min_y < 0) min_y = 0;

  imgWin.set_view(min_x, min_y, 
		  min_x + factor * delta_x,
		  min_y + factor * delta_y);

  adjust_scrollbars();

  imgWin.update_screen();
}

void MainWin::on_menu_view_zoom_out() {
  if(!main_project) return;

  unsigned int delta_x = imgWin.get_max_x() - imgWin.get_min_x();
  unsigned int delta_y = imgWin.get_max_y() - imgWin.get_min_y();

  unsigned int center_x = imgWin.get_min_x() + (delta_x >> 1);
  unsigned int center_y = imgWin.get_min_y() + (delta_y >> 1);

  zoom_out(imgWin.get_real_width() > main_project->width ? main_project->width /2: center_x, 
	   imgWin.get_real_height() > main_project->height ? main_project->height / 2 : center_y);

}

void MainWin::zoom_out(unsigned int center_x, unsigned int center_y) {

  if(!main_project) return;

  double delta_x = imgWin.get_max_x() - imgWin.get_min_x();
  double delta_y = imgWin.get_max_y() - imgWin.get_min_y();

  unsigned int max_edge_length = MAX(main_project->width, main_project->height);

  if(delta_x <= max_edge_length || delta_y < max_edge_length) {
    
    double factor = ZOOM_STEP;
    double min_x = (double)center_x - factor * (delta_x/2.0);
    double min_y = (double)center_y - factor * (delta_y/2.0);
    if(min_x < 0) min_x = 0;
    if(min_y < 0) min_y = 0;
    
    imgWin.set_view(min_x, min_y, 
		    min_x + factor * delta_x,
		    min_y + factor * delta_y);

    adjust_scrollbars();
   
    imgWin.update_screen();
  }
}

void MainWin::adjust_scrollbars() {

  m_VAdjustment.set_lower(0);
  m_HAdjustment.set_lower(0);

  m_HAdjustment.set_upper(main_project ? main_project->width: 0);
  m_VAdjustment.set_upper(main_project ? main_project->height: 0);

  //m_VAdjustment.set_page_size(main_project ? main_project->width: 0);
  //m_HAdjustment.set_page_size(main_project ? main_project->height: 0);

  int delta_x = imgWin.get_max_x() - imgWin.get_min_x();
  int delta_y = imgWin.get_max_y() - imgWin.get_min_y();

  m_HAdjustment.set_value(main_project ? imgWin.get_min_x() : 0);
  m_VAdjustment.set_value(main_project ? imgWin.get_min_y() : 0);

  m_VAdjustment.set_page_size(delta_y);
  m_HAdjustment.set_page_size(delta_x);

  m_VAdjustment.set_step_increment(delta_y);
  m_HAdjustment.set_step_increment(delta_x);

  m_VAdjustment.set_page_increment(delta_y);
  m_HAdjustment.set_page_increment(delta_x);
}

void MainWin::on_v_adjustment_changed() {
  unsigned int val = (unsigned int) m_VAdjustment.get_value();

  imgWin.set_view(imgWin.get_min_x(), val, 
		  imgWin.get_max_x(), val + imgWin.get_real_height());
  imgWin.update_screen();
}

void MainWin::on_h_adjustment_changed() {
  unsigned int val = (unsigned int)m_HAdjustment.get_value();

  imgWin.set_view(val, imgWin.get_min_y(),
		  val + imgWin.get_real_width(), imgWin.get_max_y());

  imgWin.update_screen();
}

void MainWin::on_menu_others() {
}

void MainWin::on_menu_tools_select() {
  Glib::RefPtr<Gdk::Window> window = imgWin.get_window();
  window->set_cursor(Gdk::Cursor(Gdk::ARROW));
  tool = TOOL_SELECT;
  imgWin.set_tool(tool);
}

void MainWin::on_menu_tools_move() {
  Glib::RefPtr<Gdk::Window> window = imgWin.get_window();
  window->set_cursor(Gdk::Cursor(Gdk::HAND1));
  tool = TOOL_MOVE;
  imgWin.set_tool(tool);
}

void MainWin::on_menu_tools_wire() {
  Glib::RefPtr<Gdk::Window> window = imgWin.get_window();
  window->set_cursor(Gdk::Cursor(Gdk::CROSSHAIR));
  tool = TOOL_WIRE;
  imgWin.set_tool(tool);
}

void MainWin::on_menu_tools_via_up() {
  Glib::RefPtr<Gdk::Window> window = imgWin.get_window();
  window->set_cursor(Gdk::Cursor(Gdk::DOTBOX));
  tool = TOOL_VIA_UP;
  imgWin.set_tool(tool);
}

void MainWin::on_menu_tools_via_down() {
  Glib::RefPtr<Gdk::Window> window = imgWin.get_window();
  window->set_cursor(Gdk::Cursor(Gdk::DOTBOX));
  tool = TOOL_VIA_DOWN;
  imgWin.set_tool(tool);
}

void MainWin::on_algorithm_finished(int slot_pos, plugin_params_t * plugin_params) {
  debug(TM, "Algorithm finished. Is there a dialog to raise?");
  if(ipWin) {
    ipWin->close();
    delete ipWin;
    ipWin = NULL;
  }

  imgWin.update_screen();

  plugin_calc_slot(plugin_func_table, slot_pos, PLUGIN_FUNC_AFTER_DIALOG, plugin_params, this);

  if(RET_IS_NOT_OK(plugin_calc_slot(plugin_func_table, slot_pos, PLUGIN_FUNC_SHUTDOWN, plugin_params, this))) {
    error_dialog("Plugin Error", "Can't run shutdown function");
    return;
  }

  if(plugin_params) free(plugin_params);
  //signal_algorithm_finished_.disconnect();
  delete signal_algorithm_finished_;

  imgWin.update_screen();
  project_changed();
}


void MainWin::algorithm_calc_thread(int slot_pos, plugin_params_t * plugin_params) {

  debug(TM, "Calculating ...");
  plugin_calc_slot(plugin_func_table, slot_pos, PLUGIN_FUNC_CALC, plugin_params, this);  
  (*signal_algorithm_finished_)();
}

void MainWin::on_algorithms_func_clicked(int slot_pos) {

  if(!main_project) {
    error_dialog("Error", "You need to open a project first.");
    return;
  }

  debug(TM, "algorithm clicked %d", slot_pos);

  plugin_params_t * pparams = (plugin_params_t * )malloc(sizeof(plugin_params_t));
  if(!pparams) return;
  memset(pparams, 0, sizeof(plugin_params_t));
  
  pparams->project = main_project;
  pparams->min_x = imgWin.get_selection_min_x();
  pparams->max_x = imgWin.get_selection_max_x();
  pparams->min_y = imgWin.get_selection_min_y();
  pparams->max_y = imgWin.get_selection_max_y();
  pparams->grid = imgWin.get_grid();
  
  debug(TM, "Call init method for plugin");
  if(RET_IS_NOT_OK(plugin_calc_slot(plugin_func_table, slot_pos, PLUGIN_FUNC_INIT, pparams, this))) {
    error_dialog("Plugin Error", "Can't initialize plugin.");
    return;
  }

  if(RET_IS_OK(plugin_calc_slot(plugin_func_table, slot_pos, PLUGIN_FUNC_BEFORE_DIALOG, pparams, this))) {


    ipWin = new InProgressWin(this, "Calculating", "Please wait while calculating.");
    ipWin->show();

    signal_algorithm_finished_ = new Glib::Dispatcher;

    signal_algorithm_finished_->connect(sigc::bind<int, plugin_params_t *>(sigc::mem_fun(*this, &MainWin::on_algorithm_finished),
									   slot_pos, pparams));
    
    thread = Glib::Thread::create(sigc::bind<int, plugin_params_t *>(sigc::mem_fun(*this, &MainWin::algorithm_calc_thread), 
								     slot_pos, pparams), false);
  }

}

void MainWin::on_menu_gate_list() {
  if(main_project) {
    GateListWin glWin(this, main_project->lmodel);
    glWin.run();
  }
}

void MainWin::on_menu_gate_orientation() {
  if(main_project && (selected_objects.size() == 1)) {
    std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it = selected_objects.begin();
    LM_OBJECT_TYPE object_type = (*it).second;
    if(object_type != LM_TYPE_GATE) return;
    
    lmodel_gate_t * gate = (lmodel_gate_t *) (*it).first;

    SetOrientationWin oWin(this, gate->template_orientation);
    LM_TEMPLATE_ORIENTATION new_ori = oWin.run();

    if(new_ori != gate->template_orientation) {
      if(RET_IS_NOT_OK(lmodel_set_gate_orientation(gate, new_ori)))
	error_dialog("Error", "Can't set orientation. Probably it is not possible, because the gate represents a master template.");
      else {
	imgWin.update_screen();
	project_changed();
      }
    }
    
  }
}

void MainWin::on_menu_gate_set_as_master() {
  if(main_project && (selected_objects.size() == 1)) {
    std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it = selected_objects.begin();
    LM_OBJECT_TYPE object_type = (*it).second;
    if(object_type != LM_TYPE_GATE) return;
    
    lmodel_gate_t * gate = (lmodel_gate_t *) (*it).first;
    gate_template_t * tmpl = lmodel_get_template_for_gate(gate);

    if(!tmpl) {
      error_dialog("Error", "The gate should be the master template, but you have not defined of which type. "
		   "Please set a gate type for the gate.");
      return;
    }

    LM_TEMPLATE_ORIENTATION orig_orient = lmodel_get_gate_orientation(gate);
    if(orig_orient == LM_TEMPLATE_ORIENTATION_UNDEFINED) {
      error_dialog("Error", "The gate orientation is undefined. Please define it. Set it at least to \"normal\".");
      return;
    }

    if(RET_IS_NOT_OK(lmodel_set_gate_orientation(gate, orig_orient))) {
      error_dialog("Error", "Can't adjust other gates orientation relative to the new master template.");
      return;
    }

    if(RET_IS_NOT_OK(lmodel_gate_template_set_master_region(tmpl, gate->min_x, gate->min_y, gate->max_x, gate->max_y))) {
      error_dialog("Error", "Cant set this gate as master template.");
      return;
    }

    imgWin.update_screen();
    project_changed();
  }
}

void MainWin::on_menu_gate_set() {
  gate_template_t * tmpl = NULL;
  if(!main_project) return;

  if(imgWin.selection_active()) {

    GateSelectWin gsWin(this, main_project->lmodel);
    tmpl = gsWin.run();

    if(tmpl) {
      int layer = lmodel_get_layer_num_by_type(main_project->lmodel, LM_LAYER_TYPE_LOGIC);
      if(layer == -1) {
	error_dialog("Error", "There is no logic layer defined. Please define layer types.");
      }
      else {
	
	lmodel_gate_t * new_gate = lmodel_create_gate(main_project->lmodel, 
						      imgWin.get_selection_min_x(), 
						      imgWin.get_selection_min_y(), 
						      imgWin.get_selection_max_x(), 
						      imgWin.get_selection_max_y(), 
						      tmpl, NULL, 0);
	assert(new_gate);
	if(RET_IS_NOT_OK(lmodel_reset_gate_shape(new_gate)))
	  error_dialog("Error", "Can't reset gate shape");

	if(RET_IS_NOT_OK(lmodel_add_gate(main_project->lmodel, layer, new_gate)))
	  error_dialog("Error", "Can't add gate to logic model.");
	else {
	  imgWin.reset_selection();
	  imgWin.update_screen();
	  project_changed();
	}
      }
    }
  }
  else if(selected_objects.size() == 1) {
    
    std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it = selected_objects.begin();
    LM_OBJECT_TYPE object_type = (*it).second;
    if(object_type != LM_TYPE_GATE) return;

    lmodel_gate_t * gate = (lmodel_gate_t *) (*it).first;
    
    GateSelectWin gsWin(this, main_project->lmodel);
    tmpl = gsWin.run();
    if(tmpl) {

      debug(TM, "new template");
      if(RET_IS_NOT_OK(lmodel_reset_gate_shape(gate)))
	error_dialog("Error", "Can't reset gate shape.");

      if(RET_IS_NOT_OK(lmodel_set_template_for_gate(main_project->lmodel, gate, tmpl))) {
	error_dialog("Error", "Can't set template.");
      }
      else {
	imgWin.update_screen();
	project_changed();
      }
    }
  }
}

void MainWin::on_menu_gate_create_by_selection() {
  if(imgWin.selection_active() && main_project) {

    gate_template_t * tmpl = lmodel_create_gate_template();
    
    lmodel_gate_template_set_master_region(tmpl,
					   imgWin.get_selection_min_x(), 
					   imgWin.get_selection_min_y(), 
					   imgWin.get_selection_max_x(), 
					   imgWin.get_selection_max_y());
    GateConfigWin gcWin(this, tmpl);
    if(gcWin.run() == true) {
      if(RET_IS_NOT_OK(lmodel_add_gate_template(main_project->lmodel, tmpl, 0))) {
	error_dialog("Error", "Can't add gate template to logic model.");
      }
      else {
	int layer = lmodel_get_layer_num_by_type(main_project->lmodel, LM_LAYER_TYPE_LOGIC);
	if(layer == -1) {
	  error_dialog("Error", "There is no logic layer defined. Please define layer types.");
	}
	else {

	  lmodel_gate_t * new_gate = lmodel_create_gate(main_project->lmodel, 
							imgWin.get_selection_min_x(), 
							imgWin.get_selection_min_y(), 
							imgWin.get_selection_max_x(), 
							imgWin.get_selection_max_y(), 
							tmpl, NULL, 0);

	  assert(new_gate);
	  if(RET_IS_NOT_OK(lmodel_set_template_for_gate(main_project->lmodel, new_gate, tmpl))) {
	    error_dialog("Error", "Can't set gates template.");
	  }
	  if(RET_IS_NOT_OK(lmodel_set_gate_orientation(new_gate, LM_TEMPLATE_ORIENTATION_NORMAL))) {
	    error_dialog("Error", "Can't set orientation.");
	  }

	  if(RET_IS_NOT_OK(lmodel_add_gate(main_project->lmodel, layer, new_gate)))
	    error_dialog("Error", "Can't add gate to logic model.");
	  else {
	    imgWin.reset_selection();
	    imgWin.update_screen();
	  }
	  project_changed();
	}
      }
    }
  }
}

void MainWin::on_menu_help_about() {
  char filename[PATH_MAX];
  snprintf(filename, PATH_MAX, "%s/icons/degate_logo.png", getenv("DEGATE_HOME"));

  Gtk::AboutDialog about_dialog;

  about_dialog.set_version("Version 1.23");
  about_dialog.set_logo(Gdk::Pixbuf::create_from_file(filename));

  about_dialog.set_comments("Martin Schobert <martin@weltregierung.de>\n"
			    "This software is released under the\nGNU General Public License Version 3.\n"
			    "2008"
			    );
  about_dialog.set_website("http://degate.zfch.de/");
  about_dialog.run();
}

void MainWin::on_wire_tool_release() {
  if(imgWin.get_wire_min_x() ==  imgWin.get_wire_max_x() &&
     imgWin.get_wire_min_y() ==  imgWin.get_wire_max_y()) return;

  lmodel_wire_t * new_wire = lmodel_create_wire(main_project->lmodel,
						imgWin.get_wire_min_x(), imgWin.get_wire_min_y(),
						imgWin.get_wire_max_x(), imgWin.get_wire_max_y(),
						main_project->wire_diameter, NULL, 0);
  assert(new_wire);

  if(!RET_IS_OK(lmodel_add_wire_with_autojoin(main_project->lmodel, main_project->current_layer, new_wire)))
    error_dialog("Error", "Can't place wire");
  else
    project_changed();

  imgWin.update_screen();
}

void MainWin::on_selection_activated() {
  if(main_project) {

    set_menu_item_sensitivity("/MenuBar/GateMenu/GateCreateBySelection", true);
    set_menu_item_sensitivity("/MenuBar/GateMenu/GateSet", selected_objects.size() == 1 || imgWin.selection_active() ? true : false);
  }
}

void MainWin::on_selection_revoked() {
  if(main_project) {

    set_menu_item_sensitivity("/MenuBar/GateMenu/GateCreateBySelection", false);
    set_menu_item_sensitivity("/MenuBar/GateMenu/GateSet", selected_objects.size() == 1 || imgWin.selection_active() ? true : false);
  }
}

void MainWin::on_mouse_scroll_down(unsigned int center_x, unsigned int center_y) {
  //zoom_in(center_x, center_y);
  on_menu_view_zoom_in();

}

void MainWin::on_mouse_scroll_up(unsigned int center_x, unsigned int center_y) {
  //zoom_out(center_x, center_y);
  on_menu_view_zoom_out();
}

bool MainWin::on_imgwin_clicked(GdkEventButton * event) {

  if(main_project && (event->type == GDK_BUTTON_PRESS)) {
    if(event->button == 3) {
      imgWin.coord_screen_to_real((unsigned int)(event->x), (unsigned int)(event->y), &last_click_on_real_x, &last_click_on_real_y);
      m_Menu_Popup.popup(event->button, event->time);
    }
    else if(event->button == 1) {
      unsigned int real_x, real_y;
      imgWin.coord_screen_to_real((unsigned int)(event->x), (unsigned int)(event->y), &real_x, &real_y);

      if(tool == TOOL_SELECT && !imgWin.selection_active()) {
	object_clicked(real_x, real_y);
      }
      else if(tool == TOOL_VIA_UP || tool == TOOL_VIA_DOWN) {

	lmodel_via_t * new_via = lmodel_create_via(main_project->lmodel,
						   real_x, real_y,	
						   tool == TOOL_VIA_UP ? LM_VIA_UP : LM_VIA_DOWN,
						   main_project->pin_diameter, NULL, 0);
	assert(new_via);
						   
	if(RET_IS_NOT_OK(lmodel_add_via_with_autojoin(main_project->lmodel, main_project->current_layer, new_via)))
	  error_dialog("Error", "Can't place a via");
	else {
	  project_changed();
	  imgWin.update_screen();
	}
      }


    }
      
  }
  return true;
}

bool MainWin::on_key_release_event_received(GdkEventKey * event) {
  if((event->keyval ==  GDK_Shift_L || event->keyval == GDK_Shift_R)) {
    shift_key_pressed = false;
    imgWin.set_shift_key_state(false);
    if(tool == TOOL_WIRE) imgWin.update_screen();
  }
  return false;
}

bool MainWin::on_key_press_event_received(GdkEventKey * event) {
  if(event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R) {
    shift_key_pressed = true;
    imgWin.set_shift_key_state(true);
    if(tool == TOOL_WIRE) imgWin.update_screen();
  }
  return false;
}

void MainWin::object_clicked(unsigned int real_x, unsigned int real_y) {
  char msg[1000];
  LM_OBJECT_TYPE object_type;
  void * obj_ptr;
  bool add_to_selection = false;

  // get info about selected object
  lmodel_object_to_string(main_project->lmodel, main_project->current_layer, real_x, real_y, msg, sizeof(msg));
  m_statusbar.push(msg);

  if(RET_IS_NOT_OK(lmodel_get_object(main_project->lmodel, main_project->current_layer, 
				     real_x, real_y, &object_type, &obj_ptr))) return;
  
  
  if(obj_ptr) {
    add_to_selection = true;
  }

  std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it;

  // try to remove a single object
  if(obj_ptr && shift_key_pressed) {
    debug(TM, "remove single object from selection");      
    it = selected_objects.find(std::pair<void *, LM_OBJECT_TYPE>(obj_ptr, object_type));
    if(it != selected_objects.end()) {
      if(RET_IS_OK(lmodel_set_select_state((*it).second, (*it).first, SELECT_STATE_NOT))) {
      }
      selected_objects.erase(*it);
      add_to_selection = false;
    }
  }

  if(shift_key_pressed == false){
    debug(TM, "remove complete selection");
      
    for(it = selected_objects.begin(); it != selected_objects.end(); it++) {
      if(RET_IS_OK(lmodel_set_select_state((*it).second, (*it).first, SELECT_STATE_NOT))) {
      }
    }
    selected_objects.erase(selected_objects.begin(), selected_objects.end());
  }
  
  
  if(add_to_selection) {
    // add to selection
    if(obj_ptr)
      if(RET_IS_OK(lmodel_set_select_state(object_type, obj_ptr, SELECT_STATE_DIRECT))) {
	std::pair<void *, LM_OBJECT_TYPE> p(obj_ptr, object_type);
	selected_objects.insert(p);
      }
  }
 
  imgWin.update_screen();

  set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerClearLogicModelInSelection",selected_objects.size() > 0 ? true : false);
  set_menu_item_sensitivity("/MenuBar/GateMenu/GateSet", selected_objects.size() == 1 || imgWin.selection_active() ? true : false);
  set_menu_item_sensitivity("/MenuBar/GateMenu/GateOrientation", selected_objects.size() == 1 ? true : false);
  set_menu_item_sensitivity("/MenuBar/GateMenu/GateSetAsMaster", selected_objects.size() == 1 ? true : false);
}

void MainWin::on_popup_menu_lock_region() {
}

void MainWin::on_popup_menu_set_port() {
  if(main_project) {
    LM_OBJECT_TYPE object_type;
    void * obj_ptr;
    unsigned int x, y;

    if(RET_IS_NOT_OK(lmodel_get_object(main_project->lmodel, main_project->current_layer, 
				       last_click_on_real_x, last_click_on_real_y, 
				       &object_type, &obj_ptr))) {
      error_dialog("Error", "Unknown error.");
      return;
    }
    if(!obj_ptr || object_type != LM_TYPE_GATE) {
      error_dialog("Error", "There is no gate");
      return;
    }
    
    lmodel_gate_t * gate = (lmodel_gate_t *) obj_ptr;
    if(!gate->gate_template) {
      error_dialog("Error", "Please define a template type for that gate.");
      return;
    }

    assert(gate->gate_template->master_image_max_x > gate->gate_template->master_image_min_x);
    assert(gate->gate_template->master_image_max_y > gate->gate_template->master_image_min_y);

    unsigned int width  = gate->gate_template->master_image_max_x - gate->gate_template->master_image_min_x;
    unsigned int height = gate->gate_template->master_image_max_y - gate->gate_template->master_image_min_y;

    if(gate->min_x <= last_click_on_real_x &&
       gate->min_y <= last_click_on_real_y) {
      x = last_click_on_real_x - gate->min_x;
      y = last_click_on_real_y - gate->min_y;
    }
    else {
      error_dialog("Error", "Can't define gate port.");
      return;
    }

    switch(gate->template_orientation) {
    case LM_TEMPLATE_ORIENTATION_UNDEFINED:
      error_dialog("Error", "Please define gate's orientation relative to the template gate.");
      return;
      break;
    case LM_TEMPLATE_ORIENTATION_NORMAL:
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_UP_DOWN:
      y = height - y;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_LEFT_RIGHT:
      x = width - x;
      break;
    case LM_TEMPLATE_ORIENTATION_FLIPPED_BOTH:
      x = width - x;
      y = height - y;
      break;
    }
    
    PortSelectWin psWin(this, gate);
    gate_template_port_t * template_port = psWin.run();
    if(template_port) {
      debug(TM, "x=%d y=%d", x, y);
      template_port->relative_x_coord = x; 
      template_port->relative_y_coord = y; 
      template_port->diameter = main_project->pin_diameter;

      project_changed();
      imgWin.update_screen();
    }
  }
}

void MainWin::on_popup_menu_set_name() {
  if(main_project) {

    std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it;
    Glib::ustring name("");

    if(selected_objects.size() == 0) {
      error_dialog("Error", "Please select one or more objects.");
      return;
    }
    else if(selected_objects.size() == 1) {
      it = selected_objects.begin();
      name = lmodel_get_name( (*it).second, (*it).first);
    }

    SetNameWin nsWin(this, name);
    name = nsWin.run();

    for(it = selected_objects.begin(); it != selected_objects.end(); it++) {
      if(RET_IS_NOT_OK(lmodel_set_name( (*it).second, (*it).first, name.c_str() ))) {
	error_dialog("Error", "Can't set name.");
	return;
      }
    }
    project_changed();
    imgWin.update_screen();

  }
}

void MainWin::on_popup_menu_set_alignment_marker(MARKER_TYPE marker_type) {
  if(main_project && main_project->alignment_marker_set) {

    // check if the marker is already stored
    if(amset_get_marker(main_project->alignment_marker_set, main_project->current_layer, 
			marker_type) != NULL) {
      Gtk::MessageDialog dialog(*this, "Marker already placed. Do you want to replace it?", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
      dialog.set_title("Warning");      
      if(dialog.run() == Gtk::RESPONSE_YES) {
	if(amset_replace_marker(main_project->alignment_marker_set, main_project->current_layer, 
				marker_type, last_click_on_real_x, last_click_on_real_y) == 0) {
	  Gtk::MessageDialog dialog(*this, "Error: Can't replace marker.", true, Gtk::MESSAGE_ERROR);
	  dialog.set_title("Error");
	  dialog.run();
	}
      }
    }
    else {
      if(amset_add_marker(main_project->alignment_marker_set, main_project->current_layer, 
			  marker_type, last_click_on_real_x, last_click_on_real_y) == 0) {
	Gtk::MessageDialog dialog(*this, "Error: Can't add marker.", true, Gtk::MESSAGE_ERROR);
	dialog.set_title("Error");
	dialog.run();
	
      }
    }
    //amset_print(main_project->alignment_marker_set);

    project_changed();

    set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerAlignment", 
			      amset_complete(main_project->alignment_marker_set) ? true : false);

    imgWin.update_screen();
  }
}


void MainWin::on_menu_view_grid_config() {
  GridConfigWin gcWin(this, 
		      imgWin.get_grid_offset_x(), imgWin.get_grid_offset_y(),
		      imgWin.get_grid_dist_x(), imgWin.get_grid_dist_y());

  gcWin.signal_changed().connect(sigc::bind<GridConfigWin *>(sigc::mem_fun(*this, &MainWin::on_grid_config_changed), &gcWin));

  Gtk::Main::run(gcWin);
 
}

void MainWin::on_grid_config_changed(GridConfigWin * gcWin) {
  if(gcWin) {
    imgWin.set_grid_offset_x(gcWin->get_grid_offset_x());
    imgWin.set_grid_offset_y(gcWin->get_grid_offset_y());
    imgWin.set_grid_dist_x(gcWin->get_grid_dist_x());
    imgWin.set_grid_dist_y(gcWin->get_grid_dist_y());
    project_changed();
    imgWin.update_screen();
  }
}

/*void MainWin::on_distance_to_col_changed(Gtk::ColorSelection * pColorSel) {
  render_params_t * render_params = imgWin.get_render_params();
  Gdk::Color col = pColorSel->get_current_color();
  render_params->distance_to_color = MERGE_CHANNELS(col.get_red() >> 8, col.get_green() >> 8, col.get_blue() >> 8, 0xff);
  imgWin.update_screen();
  }

void MainWin::on_menu_view_distance_to_color() {

  render_params_t * render_params = imgWin.get_render_params();
  Gdk::Color col;
  col.set_red(MASK_R(render_params->distance_to_color) << 8);
  col.set_green(MASK_G(render_params->distance_to_color) << 8);
  col.set_blue(MASK_B(render_params->distance_to_color) << 8);

  Gtk::ColorSelectionDialog dialog("Select a reference color to compute the color distance");
  dialog.set_transient_for(*this);
  Gtk::ColorSelection* pColorSel = dialog.get_colorsel();
  pColorSel->set_current_color(col);
  pColorSel->signal_color_changed().connect(sigc::bind<Gtk::ColorSelection *>(sigc::mem_fun(*this, &MainWin::on_distance_to_col_changed), pColorSel));
  int result = dialog.run();
  
  switch(result) {
  case(Gtk::RESPONSE_OK):
    col = pColorSel->get_current_color();
    render_params->distance_to_color = MERGE_CHANNELS(col.get_red() >> 8, col.get_green() >> 8, col.get_blue() >> 8, 0xff);
    imgWin.update_screen();
    break;
  }
 
}

*/

void MainWin::on_menu_layer_import_background() {
  Gtk::FileChooserDialog dialog("Please select background patches", Gtk::FILE_CHOOSER_ACTION_OPEN);
  dialog.set_transient_for(*this);
  //dialog.set_select_multiple(true);

  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button("Select", Gtk::RESPONSE_OK);

  int result = dialog.run();

  //Glib::SListHandle<Glib::ustring> filenames = dialog.get_filenames();
  Glib::ustring filename = dialog.get_filename();
  dialog.hide();
  
  switch(result) {
  case(Gtk::RESPONSE_OK):

    ipWin = new InProgressWin(this, "Importing", "Please wait while importing background image(s).");
    ipWin->show();
    project_changed();
    signal_bg_import_finished_.connect(sigc::mem_fun(*this, &MainWin::on_background_import_finished));
    Glib::Thread::create(sigc::bind<const Glib::ustring>(sigc::mem_fun(*this, &MainWin::background_import_thread), filename), false);

    break;
  case(Gtk::RESPONSE_CANCEL):
    break;
  }
  
}


// in GUI-thread
void MainWin::on_background_import_finished() {

  if(ipWin) {
    ipWin->close();
    delete ipWin;
    ipWin = NULL;
  }
  imgWin.update_screen();
}

void MainWin::background_import_thread(Glib::ustring bg_filename) {
  if(!gr_import_background_image(main_project->bg_images[main_project->current_layer], 0, 0, bg_filename.c_str())) {
    debug(TM, "Can't import image file");
  }

  signal_bg_import_finished_();
}


void MainWin::on_menu_layer_set_transistor() { 

  if(main_project) {
    debug(TM, "current layer is %d. layer_type is %d", 
	  main_project->current_layer, 
	  main_project->lmodel->layer_type[main_project->current_layer]);
    if(main_project->lmodel->layer_type[main_project->current_layer] == LM_LAYER_TYPE_TRANSISTOR) return;

    if(-1 != lmodel_get_layer_num_by_type(main_project->lmodel, LM_LAYER_TYPE_TRANSISTOR))
      warning_dialog("Warning", "There is already a transistor layer.");

    project_changed();
    lmodel_set_layer_type(main_project->lmodel, main_project->current_layer, LM_LAYER_TYPE_TRANSISTOR);
  }
}

void MainWin::on_menu_layer_set_logic() { 

  if(main_project) {
    if(main_project->lmodel->layer_type[main_project->current_layer] == LM_LAYER_TYPE_LOGIC) return;

    if(-1 != lmodel_get_layer_num_by_type(main_project->lmodel, LM_LAYER_TYPE_LOGIC))
      warning_dialog("Warning", "There is already a logic layer.");

    lmodel_set_layer_type(main_project->lmodel, main_project->current_layer, LM_LAYER_TYPE_LOGIC);
    project_changed();
  }
}

void MainWin::on_menu_layer_set_metal() { 
  if(main_project) {
    if(main_project->lmodel->layer_type[main_project->current_layer] == LM_LAYER_TYPE_METAL) return;

    lmodel_set_layer_type(main_project->lmodel, main_project->current_layer, LM_LAYER_TYPE_METAL);
    project_changed();
  }
}

void MainWin::set_layer_type_in_menu(LAYER_TYPE layer_type) {

  /* Hack: If you defined a signal handler for radio button activation in a group and you want to
     activate another item from the radio button group via ->activate() or ->set_active() two signals
     are emitted -- for the last activted button and for the new one.

     Here it is a problem, because we want to find out, if the user activated it 
     directly via menu or if it is activated just because it is defined as callback.
     So we undefine the callback first, to prevent it.
  */

  sig_conn_rbg_transistor.disconnect();
  sig_conn_rbg_logic.disconnect();
  sig_conn_rbg_metal.disconnect();

  switch(layer_type) {
    case LM_LAYER_TYPE_TRANSISTOR: 
      m_refChoice_TransistorLayer->set_active();
      break;
    case LM_LAYER_TYPE_LOGIC: 
      m_refChoice_LogicLayer->set_active();
      break;
    case LM_LAYER_TYPE_METAL: 
    case LM_LAYER_TYPE_UNDEF: 
      m_refChoice_MetalLayer->activate();
      break;
  }

  sig_conn_rbg_transistor = m_refChoice_TransistorLayer->signal_activate().connect(sigc::mem_fun(*this, &MainWin::on_menu_layer_set_transistor));
  sig_conn_rbg_logic = m_refChoice_LogicLayer->signal_activate().connect(sigc::mem_fun(*this, &MainWin::on_menu_layer_set_logic));
  sig_conn_rbg_metal = m_refChoice_MetalLayer->signal_activate().connect(sigc::mem_fun(*this, &MainWin::on_menu_layer_set_metal));

}


void MainWin::on_menu_layer_clear_logic_model() {
  Gtk::MessageDialog dialog(*this, "Clear logic model",
			    false, Gtk::MESSAGE_QUESTION,
			    Gtk::BUTTONS_OK_CANCEL);
  dialog.set_secondary_text("Are you sure you want to clear the complete logic model for the current layer?");
  int result = dialog.run();
  switch(result) {
  case(Gtk::RESPONSE_OK):
    if(RET_IS_NOT_OK(lmodel_clear_layer(main_project->lmodel, main_project->current_layer)))
      error_dialog("Error", "Can't clear logic model for current layer. It is possible, that the logic model for the current layer is in a bad state.");
    else {
      project_changed();
      imgWin.update_screen();
    }
    break;
  case(Gtk::RESPONSE_CANCEL):
  default:
    break;
  }

}

void MainWin::on_menu_layer_clear_logic_model_in_selection() {

  std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it;

  for(it = selected_objects.begin(); it != selected_objects.end(); it++) {

    if(RET_IS_NOT_OK(lmodel_remove_object_by_ptr(main_project->lmodel, main_project->current_layer, (*it).first, (*it).second))) {
      error_dialog("Error", "Can't remove object(s) from logic model");
    }
  }

  selected_objects.erase(selected_objects.begin(), selected_objects.end());
  set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerClearLogicModelInSelection", false);
  imgWin.update_screen(); 

  /*
  if(imgWin.selection_active()) {

    if(!lmodel_clear_area(main_project->lmodel, main_project->current_layer, 
			  imgWin.get_selection_min_x(), 
			  imgWin.get_selection_min_y(), 
			  imgWin.get_selection_max_x(), 
			  imgWin.get_selection_max_y())) {

      Gtk::MessageDialog dialog(*this, "Error: Can't clear logic model.", true, Gtk::MESSAGE_ERROR);
      dialog.set_title("Error");
      dialog.run();
    }
    imgWin.update_screen();
  }
  */
}

void MainWin::on_menu_layer_clear_background_image() {
  Gtk::MessageDialog dialog(*this, "Clear background image",
			    false /* use_markup */, Gtk::MESSAGE_QUESTION,
			    Gtk::BUTTONS_OK_CANCEL);
  dialog.set_secondary_text("Are you sure you want to clear the background image for the current layer?");
  int result = dialog.run();
  switch(result) {
  case(Gtk::RESPONSE_OK):
    if(!gr_map_clear(main_project->bg_images[main_project->current_layer]))
      error_dialog("Error", "Error: Can't clear background image for current layer.");
    else {
      project_changed();
      imgWin.update_screen();
    }
  case(Gtk::RESPONSE_CANCEL):
  default:
    break;
  }
  
}


void MainWin::on_menu_layer_align() {
  assert(main_project);
  assert(main_project->bg_images);
  assert(main_project->lmodel);
  assert(main_project->lmodel->root);

  Gtk::MessageDialog dialog(*this, 
			    "Layer alignment is only implemented for images. It is not(!) implemented " \
			    "for objects in the logic model. If you continue, it may result in a gap " \
			    "between the logic model and image data. Do you want to proceed?",
			    true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
  dialog.set_title("Warning");      
  if(dialog.run() == Gtk::RESPONSE_NO) return;

  // will be deallocted in MainWin::on_layer_alignment_finished()
  double * scaling_x = (double*) malloc(main_project->num_layers * sizeof(double));
  double * scaling_y = (double*) malloc(main_project->num_layers * sizeof(double));
  int * shift_x = (int*) malloc(main_project->num_layers * sizeof(int));
  int * shift_y = (int*) malloc(main_project->num_layers * sizeof(int));

  amset_print(main_project->alignment_marker_set);

  if(scaling_x && scaling_y && shift_x && shift_y) {

    if(amset_calc_transformation(main_project->alignment_marker_set,
				 scaling_x, scaling_y, shift_x, shift_y)) {
 
      ipWin = new InProgressWin(this, "Layer alignment", "Please wait while aligning layers.");
      ipWin->show();
      
      signal_layer_alignment_finished_.connect(
	   sigc::bind<double *, double *, int *, int * >(
		 sigc::mem_fun(*this, &MainWin::on_layer_alignment_finished),
		 scaling_x, scaling_y, shift_x, shift_y));
      // XXX signal muesste deconnected werden?
      Glib::Thread::create(
	   sigc::bind<double *, double *, int *, int * >(
		 sigc::mem_fun(
		       *this, &MainWin::layer_alignment_thread), 
		 scaling_x, scaling_y, shift_x, shift_y), false);
      
      project_changed();
    }
    else 
      error_dialog("Error", "Error: Can't calculate aligmnment.");

    #ifdef DEBUG
    for(int i = 0; i < main_project->alignment_marker_set->num_layers; i++)
      printf("layer=%d sx=%f sy=%f dx=%d dy=%d\n", i, scaling_x[i], scaling_y[i], shift_x[i], shift_y[i]);
    #endif

  }
}

void MainWin::layer_alignment_thread(double * scaling_x, double * scaling_y, int * shift_x, int * shift_y) {

  for(int i = 0; i < main_project->num_layers; i++) {

    printf("align layer i=%d\n", i);
    assert(main_project->bg_images[i]);
    assert(main_project->lmodel->root[i]);

    // transform background images
    gr_scale_and_shift_in_place(main_project->bg_images[i],
				scaling_x[i], scaling_y[i], 
				shift_x[i], shift_y[i]);
    // transform logic model images
    /* XXX
    mm_scale_and_shift_in_place(main_project->lmodel->root[i],
				scaling_x[i], scaling_y[i], 
				shift_x[i], shift_y[i]); */
  }
  signal_layer_alignment_finished_();  
}


// in GUI-thread
void MainWin::on_layer_alignment_finished(double * scaling_x, double * scaling_y, int * shift_x, int * shift_y) {
  
  if(ipWin) {
    ipWin->close();
    delete ipWin;
    ipWin = NULL;
  }
  
  if(main_project) {
#ifdef DEBUG
    amset_print(main_project->alignment_marker_set);
#endif
    amset_apply_transformation_to_markers(main_project->alignment_marker_set,
					  scaling_x, scaling_y, shift_x, shift_y);
#ifdef DEBUG
    amset_print(main_project->alignment_marker_set);
#endif
    update_gui_for_loaded_project();
    imgWin.update_screen();
  }

  if(scaling_x) free(scaling_x);
  if(scaling_y) free(scaling_y);
    
  if(shift_x) free(shift_x);
  if(shift_y) free(shift_y);

}


void MainWin::error_dialog(const char * const title, const char * const message) {

  Gtk::MessageDialog dialog(*this, message, true, Gtk::MESSAGE_ERROR);
  dialog.set_title(title);
  dialog.run();
}

void MainWin::warning_dialog(const char * const title, const char * const message) {

  Gtk::MessageDialog dialog(*this, message, true, Gtk::MESSAGE_WARNING);
  dialog.set_title(title);
  dialog.run();
}

