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
#include "PortColorsWin.h"
#include "GateConfigWin.h"
#include "GateSelectWin.h"
#include "PortSelectWin.h"
#include "InProgressWin.h"
#include "SplashWin.h"
#include "MenuManager.h"
#include "SetOrientationWin.h"
#include "ProjectSettingsWin.h"
#include "ConnectionInspectorWin.h"
#include "GenericTextInputWin.h"
#include "gui_globals.h"

#include "lib/project.h"
#include "lib/logic_model.h"
#include "lib/alignment_marker.h"
#include "lib/plugins.h"

#define ZOOM_STEP 1.3
#define ZOOM_STEP_MOUSE_SCROLL 2.0
#define ZOOM_STEP_MOUSE_SCROLL_AND_SHIFT 1.1

MainWin::MainWin() : 
  m_VAdjustment(0.0, 0.0, 101.0, 0.1, 1.0, 1.0), // value, lower, upper, step_increment, page_increment, page_size
  m_HAdjustment(0.0, 0.0, 101.0, 0.1, 1.0, 1.0),
  m_VScrollbar(m_VAdjustment),
  m_HScrollbar(m_HAdjustment) {

  if((plugin_func_table = plugins_init(getenv("DEGATE_PLUGINS"))) == NULL) {
    debug(TM, "Warning: can't load plugins");
  }

  // setup window
  set_default_size(1024, 700);
  char path[PATH_MAX];
  snprintf(path, PATH_MAX, "%s/icons/degate_logo.png", getenv("DEGATE_HOME"));
  set_icon_from_file(path);

  add(m_Box);


  // setup menu
  menu_manager = new MenuManager(this);
  Gtk::Widget* menubar = menu_manager->get_menubar();
  Gtk::Widget* toolbar = menu_manager->get_toolbar();
  assert(menubar != NULL);
  assert(toolbar != NULL);
  if(menubar != NULL && toolbar != NULL) {
    m_Box.pack_start(*menubar, Gtk::PACK_SHRINK);
    m_Box.pack_start(*toolbar, Gtk::PACK_SHRINK);
  }

  const std::vector<Glib::ustring> render_func_names = imgWin.get_renderer_func_names();
  const std::vector<bool> render_func_states = imgWin.get_renderer_func_states();

  menu_manager->initialize_menu_render_funcs(render_func_names, render_func_states);
  menu_manager->initialize_menu_algorithm_funcs(plugin_func_table);


  initialize_image_window();

  // setup statusbar
  m_statusbar.push("");
  m_Box.pack_start(m_statusbar, Gtk::PACK_SHRINK);


  show_all_children();

  main_project = NULL;
  set_project_changed_state(false);
  update_title();
  control_key_pressed = false;
  shift_key_pressed = false;
  imgWin.set_shift_key_state(false);
  
  imgWin.grab_focus();

  project_to_open = NULL;
  Glib::signal_idle().connect( sigc::mem_fun(*this, &MainWin::on_idle));


  if(getuid() == 0) {
    warning_dialog("Security warning", 
		   "You started degate as superuser. I don't cause harm to you. "
		   "But you should think about it. You should know: \"All your base are belong to us\". "
		   "I will not drop privileges and I hope you know, what you do.");
  }


  signal_key_press_event().connect(sigc::mem_fun(*this,&MainWin::on_key_press_event_received), false);
  signal_key_release_event().connect(sigc::mem_fun(*this,&MainWin::on_key_release_event_received), false);
  signal_hide().connect(sigc::mem_fun(*this, &MainWin::on_menu_project_close), false);

}

MainWin::~MainWin() {
}


void MainWin::update_title() {
  if(main_project == NULL) {
    set_title("degate");
  }
  else {
    char _title[1000];
    assert(main_project->project_name != NULL);

    snprintf(_title, sizeof(_title), "degate -- [%s%s%s%s] [%d/%d]", 
	     strlen(main_project->project_name) > 0 ? main_project->project_name : "",
	     strlen(main_project->project_name) > 0 ? ": " : "",
	     main_project->project_dir,
	     project_changed_flag == true ? "*" : "",
	     main_project->current_layer, main_project->num_layers -1);
    set_title(_title);
  }

}


bool MainWin::on_idle() {
  debug(TM, "idle");
  if(project_to_open != NULL) {
    open_project(project_to_open);
    project_to_open = NULL;
  }
  return false;
}

void MainWin::project_changed() {
  set_project_changed_state(true);
}

void MainWin::set_project_changed_state(bool new_state) {
  project_changed_flag = new_state;
  update_title();
}


void MainWin::on_view_info_layer_toggled(int slot_pos) {
  if(menu_manager->toggle_info_layer(slot_pos)) {
    imgWin.toggle_render_info_layer(slot_pos);
    imgWin.update_screen();
  }
}

void MainWin::on_menu_view_toggle_all_info_layers() {
 
  const std::vector<bool> new_states = menu_manager->toggle_info_layer_visibility();
  imgWin.set_renderer_info_layer_state(new_states);
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

bool MainWin::on_drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time) {
  //debug(TM, "drag");
  return true;
}

void MainWin::add_to_recent_menu() {

  Glib::ustring str(main_project->project_dir);
  str += "/project.prj";
  
  Gtk::RecentManager::Data data;

  data.app_exec = "degate %u";
  data.app_name  ="degate";

  data.groups.push_back("degate");
  data.description ="degate project";

  data.display_name = str;
  data.mime_type = degate_mime_type;

  Glib::RefPtr<Gtk::RecentManager> recent_manager = Gtk::RecentManager::get_default();
  recent_manager->add_item ("file://" + str, data);
}

void MainWin::on_menu_project_recent_projects() {
  debug(TM, "on_menu_project_recent_projects()");
  /*
  Glib::RefPtr<Gtk::RecentManager> m_refRecentManager;


  Gtk::RecentChooserDialog dialog(*this, "Recent Files", m_refRecentManager);
  dialog.add_button("Select File", Gtk::RESPONSE_OK);
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

  const int response = dialog.run();
  dialog.hide();
  if(response == Gtk::RESPONSE_OK)
    {
      std::cout << "URI selected = " << dialog.get_current_uri() << std::endl;
    }
  */
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
      if(RET_IS_NOT_OK(project_init_directory(project_dir.c_str(), 0)) ||
	 ((main_project = project_create(project_dir.c_str(), width, height, layers)) == NULL) ||
	 RET_IS_NOT_OK(project_map_background_memfiles(main_project))) {

	Gtk::MessageDialog dialog(*this, "Error: Can't create new project", true, Gtk::MESSAGE_ERROR);
	dialog.set_title("Initialization of the new project failed.");
	dialog.run();
      }
      else {

#ifdef MAP_FILES_ON_DEMAND
	if(main_project != NULL && 
	   RET_IS_NOT_OK(gr_reactivate_mapping(main_project->bg_images[main_project->current_layer]))) {
	  debug(TM, "mapping image failed");
	}
#endif

	update_gui_for_loaded_project();
	set_project_changed_state(false);
	set_layer(0);
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
    imgWin.set_render_background_images(NULL, NULL);
    imgWin.set_current_layer(-1);
    imgWin.set_grid(NULL);

    clear_selection();
    if(RET_IS_NOT_OK(project_destroy(main_project))) {
      error_dialog("Error", "Can't destroy project. This should not happen.");
    }

    main_project = NULL;
    imgWin.update_screen();

    update_title();
    
    delete ciWin; 
    ciWin = NULL;

    delete gcWin;
    gcWin = NULL;

    menu_manager->set_widget_sensitivity(false);
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
    if(RET_IS_NOT_OK(project_save(main_project))) {
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

  if(result == Gtk::RESPONSE_OK) open_project(project_dir);
}

void MainWin::set_project_to_open(char * project_dir) {
  project_to_open = project_dir;
}

void MainWin::open_project(Glib::ustring project_dir) {
  if(main_project) on_menu_project_close();

  ipWin = new InProgressWin(this, "Opening Project", "Please wait while opening project.");
  ipWin->show();

  signal_project_open_finished_.connect(sigc::mem_fun(*this, &MainWin::on_project_load_finished));
  thread = Glib::Thread::create(sigc::bind<const Glib::ustring>(sigc::mem_fun(*this, &MainWin::project_open_thread), 
								project_dir), false);
}

// in GUI-thread
void MainWin::on_project_load_finished() {

  //thread->join();
  
  if(ipWin) {
    ipWin->close();
    delete ipWin;
    ipWin = NULL;
  }
  
  if(main_project == NULL) {
    Gtk::MessageDialog err_dialog(*this, "Can't open project", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
    err_dialog.run();
  }
  else {
    menu_manager->set_layer_type_in_menu(lmodel_get_layer_type(main_project->lmodel, main_project->current_layer));
    update_gui_for_loaded_project();
    set_layer(0);
  }
}

void MainWin::project_open_thread(Glib::ustring project_dir) {
  main_project = project_load(project_dir.c_str());
#ifdef MAP_FILES_ON_DEMAND
  if(main_project != NULL && 
     RET_IS_NOT_OK(gr_reactivate_mapping(main_project->bg_images[main_project->current_layer]))) {
    debug(TM, "mapping image failed");
  }
#endif

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
    imgWin.set_render_background_images(main_project->bg_images, main_project->scaling_manager);
    imgWin.reset_selection();
    imgWin.set_current_layer(0);
    imgWin.set_grid(main_project->grid);
    
    menu_manager->set_widget_sensitivity(true);
    add_to_recent_menu();

    update_title();

    adjust_scrollbars();
    
    ciWin = new ConnectionInspectorWin(this, main_project->lmodel);
    ciWin->signal_goto_button_clicked().connect(sigc::mem_fun(*this, &MainWin::goto_object));
    
    gcWin = new GridConfigWin(this, main_project->grid);
    gcWin->signal_changed().connect(sigc::mem_fun(*this, &MainWin::on_grid_config_changed));


    render_params_t * render_params = imgWin.get_render_params();
    render_params->alignment_marker_set = main_project->alignment_marker_set;

    imgWin.update_screen();  
  }
}

void MainWin::set_layer(unsigned int layer) {
  if(main_project == NULL) return;
  if(main_project->num_layers == 0) return;

  
#ifdef MAP_FILES_ON_DEMAND
  //if((unsigned int)main_project->current_layer != layer) {
    
    if(RET_IS_NOT_OK(scalmgr_unmap_files_for_layer(main_project->scaling_manager, 
						   main_project->current_layer))) {
      debug(TM, "unmapping prescaled images faild");
      return;
    }

    if(RET_IS_NOT_OK(gr_deactivate_mapping(main_project->bg_images[main_project->current_layer]))) {
      debug(TM, "unmapping image for layer %d failed.", main_project->current_layer);
      return;
    }
    if(RET_IS_NOT_OK(gr_reactivate_mapping(main_project->bg_images[layer]))) {
      debug(TM, "mapping image for layer %d failed.", layer);
      return;
    }

    if(RET_IS_NOT_OK(scalmgr_map_files_for_layer(main_project->scaling_manager, layer))) {
      debug(TM, "mapping prescaled images faild");
      return;
    }

    //}

  
#endif

  main_project->current_layer = layer;

  imgWin.set_current_layer(main_project->current_layer);
  
  menu_manager->set_layer_type_in_menu(lmodel_get_layer_type(main_project->lmodel, main_project->current_layer));

  update_title();
  imgWin.update_screen();
}

void MainWin::goto_object(LM_OBJECT_TYPE object_type, object_ptr_t * obj_ptr) {
  assert(obj_ptr != NULL);
  if(main_project != NULL && obj_ptr != NULL) {
    unsigned int center_x, center_y, layer;

    if(RET_IS_OK(lmodel_get_view_for_object(main_project->lmodel, object_type, obj_ptr,
					    &center_x, &center_y, &layer))) {

      highlighted_objects.add(object_type, obj_ptr);

      //int old_state = lmodel_get_select_state(object_type, obj_ptr);
      //lmodel_set_select_state(object_type, obj_ptr, SELECT_STATE_DIRECT);
      center_view(center_x, center_y, layer);
      //lmodel_set_select_state(object_type, obj_ptr, old_state);

      highlighted_objects.remove(object_type, obj_ptr);

    }
    
  }
}

void MainWin::on_menu_view_next_layer() {
  if(main_project->num_layers == 0) return;

  if(main_project->current_layer < main_project->num_layers - 1)
    set_layer(main_project->current_layer + 1);
  else 
    set_layer(0);
}

void MainWin::on_menu_view_prev_layer() {
  if(main_project->num_layers == 0) return;

  if(main_project->current_layer == 0)
    set_layer(main_project->num_layers - 1);
  else 
    set_layer(main_project->current_layer - 1);
}

void MainWin::on_menu_view_zoom_in() {
  if(main_project == NULL) return;

  unsigned int center_x = imgWin.get_center_x();
  unsigned int center_y = imgWin.get_center_y();

  zoom(imgWin.get_real_width() > main_project->width ? main_project->width /2: center_x, 
       imgWin.get_real_height() > main_project->height ? main_project->height / 2 : center_y,
       1.0/ZOOM_STEP);
  
}

void MainWin::on_menu_view_zoom_out() {
  if(main_project == NULL) return;

  unsigned int center_x = imgWin.get_center_x();
  unsigned int center_y = imgWin.get_center_y();

  zoom(imgWin.get_real_width() > main_project->width ? main_project->width /2: center_x, 
       imgWin.get_real_height() > main_project->height ? main_project->height / 2 : center_y,
       ZOOM_STEP);
}



void MainWin::zoom(unsigned int center_x, unsigned int center_y, double zoom_factor) {

  if(main_project == NULL) return;

  double delta_x = imgWin.get_max_x() - imgWin.get_min_x();
  double delta_y = imgWin.get_max_y() - imgWin.get_min_y();

  unsigned int max_edge_length = MAX(main_project->width, main_project->height);

  if( ((delta_x < max_edge_length || delta_y < max_edge_length) && zoom_factor >= 1) ||
      ((delta_x > 100 || delta_y > 100) && zoom_factor <= 1)  ) {
    

    double min_x = (double)center_x - zoom_factor * (delta_x/2.0);
    double min_y = (double)center_y - zoom_factor * (delta_y/2.0);
    double max_x = (double)center_x + zoom_factor * (delta_x/2.0);
    double max_y = (double)center_y + zoom_factor * (delta_y/2.0);
    if(min_x < 0) { max_x -= min_x; min_x = 0; }
    if(min_y < 0) { max_y -= min_y; min_y = 0; }
    
    imgWin.set_view(min_x, min_y, max_x, max_y);
    adjust_scrollbars();
    imgWin.update_screen();
  }
}

void MainWin::center_view(unsigned int center_x, unsigned int center_y, unsigned int layer) {

  if(main_project == NULL) return;

  unsigned int width_half = (imgWin.get_max_x() - imgWin.get_min_x()) / 2;
  unsigned int height_half = (imgWin.get_max_y() - imgWin.get_min_y()) / 2;

  unsigned int min_x = center_x > width_half ? center_x - width_half : 0;
  unsigned int min_y = center_y > height_half ? center_y - height_half : 0;
  imgWin.set_view(min_x, min_y, min_x + (width_half << 1), min_y + (height_half << 1));
  adjust_scrollbars();
  
  set_layer(layer);
}

void MainWin::adjust_scrollbars() {

  m_VAdjustment.set_lower(0);
  m_HAdjustment.set_lower(0);

  m_HAdjustment.set_upper(main_project ? main_project->width: 0);
  m_VAdjustment.set_upper(main_project ? main_project->height: 0);

  //m_VAdjustment.set_page_size(main_project ? main_project->width: 0);
  //m_HAdjustment.set_page_size(main_project ? main_project->height: 0);

  int delta_x = (imgWin.get_max_x() - imgWin.get_min_x());
  int delta_y = (imgWin.get_max_y() - imgWin.get_min_y());

  m_VAdjustment.set_page_size(delta_y);
  m_HAdjustment.set_page_size(delta_x);

  m_VAdjustment.set_step_increment((double)delta_y * 0.1);
  m_HAdjustment.set_step_increment((double)delta_x * 0.1);

  m_VAdjustment.set_page_increment(delta_y);
  m_HAdjustment.set_page_increment(delta_x);

  m_HAdjustment.set_value(main_project ? imgWin.get_min_x() : 0);
  m_VAdjustment.set_value(main_project ? imgWin.get_min_y() : 0);

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

// here is a list og images for each mouse cursor type:
// http://www.pygtk.org/docs/pygtk/gdk-constants.html

void MainWin::on_menu_tools_select() {
  Glib::RefPtr<Gdk::Window> window = imgWin.get_window();
  window->set_cursor(Gdk::Cursor(Gdk::LEFT_PTR));
  tool = TOOL_SELECT;
  imgWin.set_tool(tool);
}

void MainWin::on_menu_tools_move() {
  Glib::RefPtr<Gdk::Window> window = imgWin.get_window();
  window->set_cursor(Gdk::Cursor(Gdk::FLEUR));
  tool = TOOL_MOVE;
  imgWin.set_tool(tool);
}

void MainWin::on_menu_tools_wire() {
  Glib::RefPtr<Gdk::Window> window = imgWin.get_window();
  window->set_cursor(Gdk::Cursor(Gdk::TCROSS));
  tool = TOOL_WIRE;
  imgWin.set_tool(tool);
}

void MainWin::on_menu_tools_via_up() {
  Glib::RefPtr<Gdk::Window> window = imgWin.get_window();
  window->set_cursor(Gdk::Cursor(Gdk::CROSS));
  tool = TOOL_VIA_UP;
  imgWin.set_tool(tool);
}

void MainWin::on_menu_tools_via_down() {
  Glib::RefPtr<Gdk::Window> window = imgWin.get_window();
  window->set_cursor(Gdk::Cursor(Gdk::CROSS));
  tool = TOOL_VIA_DOWN;
  imgWin.set_tool(tool);
}

void MainWin::on_algorithm_finished(int slot_pos, plugin_params_t * plugin_params) {


  if(ipWin) {
    ipWin->close();
    delete ipWin;
    ipWin = NULL;
  }

  imgWin.update_screen();

  if(RET_IS_NOT_OK(plugin_func_ret_status)) {
    error_dialog("Plugin Error", "The plugin returned with an error");
  }

  debug(TM, "Algorithm finished. Is there a dialog to raise?");

  plugin_calc_slot(plugin_func_table, slot_pos, PLUGIN_FUNC_AFTER_DIALOG, plugin_params, this);

  if(RET_IS_NOT_OK(plugin_calc_slot(plugin_func_table, slot_pos, PLUGIN_FUNC_SHUTDOWN, plugin_params, this))) {
    error_dialog("Plugin Error", "Can't run shutdown function");
    return;
  }

  if(plugin_params) free(plugin_params);
  //signal_algorithm_finished_.disconnect();
  delete signal_algorithm_finished_;

  project_changed();
}


void MainWin::algorithm_calc_thread(int slot_pos, plugin_params_t * plugin_params) {

  debug(TM, "Calculating ...");
  plugin_func_ret_status = plugin_calc_slot(plugin_func_table, slot_pos, PLUGIN_FUNC_CALC, plugin_params, this);  
  (*signal_algorithm_finished_)();
}

void MainWin::on_algorithms_func_clicked(int slot_pos) {

  if(main_project == NULL) {
    error_dialog("Error", "You need to open a project first.");
    return;
  }

  debug(TM, "algorithm clicked %d", slot_pos);

  plugin_params_t * pparams = (plugin_params_t * )malloc(sizeof(plugin_params_t));
  if(pparams == NULL) return;
  memset(pparams, 0, sizeof(plugin_params_t));
  
  pparams->project = main_project;
  pparams->min_x = imgWin.get_selection_min_x();
  pparams->max_x = imgWin.get_selection_max_x();
  pparams->min_y = imgWin.get_selection_min_y();
  pparams->max_y = imgWin.get_selection_max_y();
  
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


void MainWin::on_menu_goto_gate_by_name() {
  if(main_project != NULL) {
    GenericTextInputWin input(this, "Goto gate by name", "Gate name", "");
    Glib::ustring str;
    if(input.run(str)) {
    
      lmodel_gate_t * gate = lmodel_get_gate_by_name(main_project->lmodel, str.c_str());
      if(gate == NULL) error_dialog("Error", "There is no gate with that name or the name is not unique.");
      else goto_object(LM_TYPE_GATE, (object_ptr_t *)gate);
    }
  }
}

void MainWin::on_menu_goto_gate_by_id() {
  if(main_project != NULL) {
    GenericTextInputWin input(this, "Goto gate by ID", "Gate ID", "");
    Glib::ustring str;
    if(input.run(str)) {
    
      unsigned int id = atol(str.c_str());

      lmodel_gate_t * gate = lmodel_get_gate_by_id(main_project->lmodel, id);
      if(gate == NULL) error_dialog("Error", "There is no gate with that ID.");
      else goto_object(LM_TYPE_GATE, (object_ptr_t *)gate);
    }
  }
}


void MainWin::on_menu_gate_port_colors() {
  if(main_project != NULL) {
    PortColorsWin pcWin(this, main_project->lmodel, main_project->port_color_manager);
    pcWin.run();

    imgWin.update_screen();
    project_changed();
  }
}


void MainWin::on_menu_gate_list() {
  if(main_project != NULL) {
    GateListWin glWin(this, main_project->lmodel);
    glWin.run();

    imgWin.update_screen();
    project_changed();

    if(RET_IS_NOT_OK(lmodel_apply_colors_to_ports(main_project->lmodel, 
						  main_project->port_color_manager))) {

      error_dialog("Error", "Can't update port colors.");
      return;
    }

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
	error_dialog("Error", 
		     "Can't set orientation. Probably it is not possible, "
		     "because the gate represents a master template.");
      else {
	imgWin.update_screen();
	project_changed();
      }
    }
    
  }
}

void MainWin::remove_gate_by_type(GS_DESTROY_MODE destroy_mode) {
  if(main_project != NULL) {

    GateSelectWin gsWin(this, main_project->lmodel);
    lmodel_gate_template_set_t * tmpl_set = gsWin.get_multiple();
    lmodel_gate_template_set_t * set_ptr = tmpl_set;

    if(tmpl_set == NULL) return;

    Gtk::MessageDialog dialog(*this, "Are you sure you want to remove all gates by that type(s)?", 
			      true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
    dialog.set_title("Warning");
    if(dialog.run() == Gtk::RESPONSE_YES) {
      dialog.hide();

      while(set_ptr != NULL) {
	assert(set_ptr->gate != NULL);

	if(set_ptr->gate != NULL &&
	   RET_IS_NOT_OK(lmodel_destroy_gates_by_template_type(main_project->lmodel, 
							       set_ptr->gate, 
							       destroy_mode))) {
	  error_dialog("Error", "Can't remove gate.");
	  goto end;
	}

	
	Gtk::MessageDialog dialog2(*this, "Do you want to remove the gate definition, too?",
				  true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
	dialog2.set_title("Warning");
	dialog2.hide();
	if(dialog.run() == Gtk::RESPONSE_YES) {

	  if(RET_IS_NOT_OK(lmodel_remove_gate_template(main_project->lmodel, set_ptr->gate))) {
	    error_dialog("Error", "Can't remove template gate.");
	    goto end;
	  }
	}

	set_ptr = set_ptr->next;
      }

    end:

      imgWin.update_screen();
      project_changed();
    }


    if(RET_IS_NOT_OK(lmodel_destroy_gate_template_set(tmpl_set, DESTROY_CONTAINER_ONLY)))
      error_dialog("Error", "Can't destroy temporary data structure.");

  }

}

void MainWin::on_menu_gate_remove_gate_by_type_wo_master() {
  remove_gate_by_type(DESTROY_WO_MASTER);
}

void MainWin::on_menu_gate_remove_gate_by_type() {
  remove_gate_by_type(DESTROY_ALL);
}

void MainWin::on_menu_gate_set_as_master() {
  if(main_project && (selected_objects.size() == 1)) {
    std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it = selected_objects.begin();
    LM_OBJECT_TYPE object_type = (*it).second;
    if(object_type != LM_TYPE_GATE) return;
    
    lmodel_gate_t * gate = (lmodel_gate_t *) (*it).first;
    lmodel_gate_template_t * tmpl = lmodel_get_template_for_gate(gate);

    if(tmpl == NULL) {
      error_dialog("Error", "The gate should be the master template, but you have not defined of which type. "
		   "Please set a gate type for the gate.");
      return;
    }

    if(lmodel_gate_is_master(gate) == 1) {
      error_dialog("Error", "The gate is already a master.");
      return;      
    }

    LM_TEMPLATE_ORIENTATION orig_orient = lmodel_get_gate_orientation(gate);
    if(orig_orient == LM_TEMPLATE_ORIENTATION_UNDEFINED) {
      error_dialog("Error", "The gate orientation is undefined. Please define it.");
      return;
    }

    if(RET_IS_NOT_OK(lmodel_set_gate_orientation(gate, LM_TEMPLATE_ORIENTATION_NORMAL))) {
      error_dialog("Error", "Can't reset gate's orientation to normal orientation.");
      return;
    }

    if(RET_IS_NOT_OK(lmodel_adjust_gate_orientation_for_all_gates(main_project->lmodel, gate, 
								  orig_orient))) {
      error_dialog("Error", "Can't adjust other gates orientation relative to the new master template.");
      return;
    }

    if(RET_IS_NOT_OK(lmodel_gate_template_set_master_region(tmpl, gate->min_x, gate->min_y, gate->max_x, gate->max_y))) {
      error_dialog("Error", "Cant set this gate as master template.");
      return;
    }

    if(RET_IS_NOT_OK(lmodel_adjust_templates_port_locations(tmpl, orig_orient))) {
      error_dialog("Error", "Can't adjust templates port locations.");
      return;
    }

    imgWin.update_screen();
    project_changed();
  }
}

void MainWin::on_menu_gate_set() {
  lmodel_gate_template_t * tmpl = NULL;
  if(main_project == NULL) return;

  if(imgWin.selection_active()) {

    GateSelectWin gsWin(this, main_project->lmodel);
    tmpl = gsWin.get_single();

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
    tmpl = gsWin.get_single();
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

    int layer = lmodel_get_layer_num_by_type(main_project->lmodel, LM_LAYER_TYPE_LOGIC);
    if(layer == -1) {
      error_dialog("Error", "There is no logic layer defined. Please define layer types.");
      return;
    }

    lmodel_gate_template_t * tmpl = lmodel_create_gate_template();
    assert(tmpl);

    if(RET_IS_NOT_OK(lmodel_gate_template_set_master_region(tmpl,
							    imgWin.get_selection_min_x(), 
							    imgWin.get_selection_min_y(), 
							    imgWin.get_selection_max_x(), 
							    imgWin.get_selection_max_y()))) {
      error_dialog("Error", "Can't set master region.");
      return;
    }

    GateConfigWin gcWin(this, main_project->lmodel, tmpl);
    if(gcWin.run() == true) {
      if(RET_IS_NOT_OK(lmodel_add_gate_template(main_project->lmodel, tmpl, 0))) {
	error_dialog("Error", "Can't add gate template to logic model.");
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

void MainWin::on_menu_help_about() {
  char filename[PATH_MAX];
  snprintf(filename, PATH_MAX, "%s/icons/degate_logo.png", getenv("DEGATE_HOME"));

  Gtk::AboutDialog about_dialog;

  about_dialog.set_version("Version 0.0.6");
  about_dialog.set_logo(Gdk::Pixbuf::create_from_file(filename));

  about_dialog.set_comments("Martin Schobert <martin@weltregierung.de>\n"
			    "This software is released under the\nGNU General Public License Version 3.\n"
			    "2009"
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

  if(RET_IS_NOT_OK(lmodel_add_wire_with_autojoin(main_project->lmodel, main_project->current_layer, new_wire)))
    error_dialog("Error", "Can't place wire");
  else
    project_changed();

  imgWin.update_screen();
}

void MainWin::on_selection_activated() {
  if(main_project) {

    menu_manager->set_menu_item_sensitivity("/MenuBar/GateMenu/GateCreateBySelection", true);
    menu_manager->set_menu_item_sensitivity("/MenuBar/GateMenu/GateSet", true);
  }
}

void MainWin::on_selection_revoked() {
  if(main_project) {

    menu_manager->set_menu_item_sensitivity("/MenuBar/GateMenu/GateCreateBySelection", false);
    menu_manager->set_menu_item_sensitivity("/MenuBar/GateMenu/GateSet", false);
  }
}

void MainWin::on_mouse_scroll_down(unsigned int clicked_real_x, unsigned int clicked_real_y) {
  if(main_project != NULL) {

    int real_dist_to_center_x = (int)clicked_real_x - (int)imgWin.get_center_x();
    int real_dist_to_center_y = (int)clicked_real_y - (int)imgWin.get_center_y();

    double zoom_factor = shift_key_pressed == true ? ZOOM_STEP_MOUSE_SCROLL_AND_SHIFT : ZOOM_STEP_MOUSE_SCROLL;

    unsigned int new_center_x = (int)imgWin.get_center_x() + real_dist_to_center_x -
      (double)real_dist_to_center_x * zoom_factor;

    unsigned int new_center_y = (int)imgWin.get_center_y() + real_dist_to_center_y -
      (double)real_dist_to_center_y * zoom_factor;

    zoom(new_center_x, new_center_y, zoom_factor);

    //zoom_out(center_x, center_y);
  }
}

void MainWin::on_mouse_scroll_up(unsigned int clicked_real_x, unsigned int clicked_real_y) {
  if(main_project != NULL) {

    int real_dist_to_center_x = (int)clicked_real_x - (int)imgWin.get_center_x();
    int real_dist_to_center_y = (int)clicked_real_y - (int)imgWin.get_center_y();

    double zoom_factor = shift_key_pressed == true ? ZOOM_STEP_MOUSE_SCROLL_AND_SHIFT : ZOOM_STEP_MOUSE_SCROLL;

    unsigned int new_center_x = (int)imgWin.get_center_x() + real_dist_to_center_x -
      (double)real_dist_to_center_x / zoom_factor;

    unsigned int new_center_y = (int)imgWin.get_center_y() + real_dist_to_center_y -
      (double)real_dist_to_center_y / zoom_factor;

    zoom(new_center_x, new_center_y, 1.0/zoom_factor);

  }
}

bool MainWin::on_imgwin_clicked(GdkEventButton * event) {

  if(main_project && (event->type == GDK_BUTTON_PRESS)) {
    if(event->button == 3) {
      imgWin.coord_screen_to_real((unsigned int)(event->x), (unsigned int)(event->y), &last_click_on_real_x, &last_click_on_real_y);
      menu_manager->show_popup_menu(event->button, event->time);
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
  else if(event->state & GDK_CONTROL_MASK || 
	  event->keyval == GDK_Control_L || 
	  event->keyval == GDK_Control_R) {
    control_key_pressed = false;
    //debug(TM, "ctrl release");
  }

  //debug(TM, "key release: %d %d", event->state, event->keyval);
  return false;
}

bool MainWin::on_key_press_event_received(GdkEventKey * event) {
  if(event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R) {
    shift_key_pressed = true;
    imgWin.set_shift_key_state(true);
    if(tool == TOOL_WIRE) imgWin.update_screen();
  }
  //else if(event->keyval == GDK_Control_L || event->keyval == GDK_Control_R) {
  else if(!(event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_Control_L)) {
    control_key_pressed = true;
    //debug(TM, "ctrl pressed");
  }
  else if(event->state & GDK_CONTROL_MASK) {
    control_key_pressed = false;
    //debug(TM, "ctrl as modifier pressed");
  }
  //else {
  //debug(TM, "any key  pressed");
  //}

  //debug(TM, "key press: %d %d", event->state, event->keyval);
  return false;
}

void MainWin::clear_selection() {
  std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it;
  debug(TM, "remove complete selection");
      
  for(it = selected_objects.begin(); it != selected_objects.end(); it++) {
    char s[100];
    lmodel_get_printable_string_for_obj((*it).second, (*it).first, s, 100);
    debug(TM, "\tunselect %s", s);
  }

  highlighted_objects.clear();
  selected_objects.erase(selected_objects.begin(), selected_objects.end());
}

void MainWin::update_gui_on_selection_change() {
  std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it;

  if(selected_objects.size() == 1) {
    it = selected_objects.begin();
    if( (*it).second == LM_TYPE_GATE_PORT) {
      if(ciWin != NULL) ciWin->set_object( LM_TYPE_GATE_PORT, (object_ptr_t *)((*it).first));
    }
    else if( (*it).second == LM_TYPE_WIRE) {
      if(ciWin != NULL) ciWin->set_object( LM_TYPE_WIRE, (object_ptr_t *)((*it).first));
    }
    else if( (*it).second == LM_TYPE_VIA) {
      if(ciWin != NULL) ciWin->set_object( LM_TYPE_VIA, (object_ptr_t *)((*it).first));
    }
    else if( (*it).second == LM_TYPE_GATE) {

      menu_manager->set_menu_item_sensitivity("/MenuBar/GateMenu/GateOrientation", true);
      menu_manager->set_menu_item_sensitivity("/MenuBar/GateMenu/GateSetAsMaster", true);
      menu_manager->set_menu_item_sensitivity("/MenuBar/GateMenu/GateSet", true);

      if(ciWin != NULL) ciWin->set_object( LM_TYPE_GATE, (object_ptr_t *)((*it).first));
      //if(ciWin != NULL) ciWin->disable_inspection();
    }
    else {
      menu_manager->set_menu_item_sensitivity("/MenuBar/GateMenu/GateSet", imgWin.selection_active() ? true : false);
      if(ciWin != NULL) ciWin->disable_inspection();
    }
  }
  else {
    menu_manager->set_menu_item_sensitivity("/MenuBar/GateMenu/GateOrientation", false);
    menu_manager->set_menu_item_sensitivity("/MenuBar/GateMenu/GateSetAsMaster", false);
    menu_manager->set_menu_item_sensitivity("/MenuBar/GateMenu/GateSet", imgWin.selection_active() ? true : false);

    if(ciWin != NULL) {
      ciWin->disable_inspection();

    }
  }

  menu_manager->set_menu_item_sensitivity("/MenuBar/LogicMenu/LogicClearLogicModelInSelection", selected_objects_are_removable() ? true : false);
  menu_manager->set_menu_item_sensitivity("/MenuBar/LogicMenu/LogicInterconnect", 
					  selected_objects.size() >= 2 && selected_objects_are_interconnectable() ? true : false);
  menu_manager->set_menu_item_sensitivity("/MenuBar/LogicMenu/LogicIsolate", 
					  selected_objects.size() >= 1 && selected_objects_are_interconnectable() ? true : false);


}

void MainWin::object_clicked(unsigned int real_x, unsigned int real_y) {
  char msg[1000];
  LM_OBJECT_TYPE object_type;
  void * obj_ptr;
  bool add_to_selection = false;

  // get info about selected object
  lmodel_object_to_string(main_project->lmodel, main_project->current_layer, real_x, real_y, msg, sizeof(msg));

  if(RET_IS_NOT_OK(lmodel_get_object(main_project->lmodel, main_project->current_layer, 
				     real_x, real_y, &object_type, &obj_ptr))) return;
  

  // check
  if(obj_ptr == NULL) {
    int layer = lmodel_get_layer_num_by_type(main_project->lmodel, LM_LAYER_TYPE_LOGIC);
    if(layer == -1) {
      error_dialog("Error", "There is no logic layer defined. Please define layer types.");
    }
    else if(layer != main_project->current_layer) {
      if(RET_IS_NOT_OK(lmodel_get_object(main_project->lmodel, layer, 
					 real_x, real_y, &object_type, &obj_ptr))) return;  
    }
  }

  if(obj_ptr != NULL) {
    add_to_selection = true;
  }
  else {
    snprintf(msg, sizeof(msg), "%d,%d", real_x, real_y);
  }

  m_statusbar.push(msg);

  std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it;

  // try to remove a single object
  if(obj_ptr != NULL && control_key_pressed == true) {
    //debug(TM, "remove single object from selection");      
    it = selected_objects.find(std::pair<void *, LM_OBJECT_TYPE>(obj_ptr, object_type));
    if(it != selected_objects.end()) {

      selected_objects.erase(*it);
      highlighted_objects.remove(object_type, (object_ptr_t *) obj_ptr);
      
      add_to_selection = false;
    }
  }

  if(control_key_pressed == false){
    clear_selection();
    highlighted_objects.clear();
  }
  
  
  if(add_to_selection) {
    // add to selection
    if(obj_ptr) {
      std::pair<void *, LM_OBJECT_TYPE> p(obj_ptr, object_type);
      selected_objects.insert(p);
      
      highlighted_objects.add(object_type, (object_ptr_t *)obj_ptr);
    }
  }
 
  imgWin.update_screen();
  update_gui_on_selection_change();
}

bool MainWin::selected_objects_are_interconnectable() {
  std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it;

  for(it = selected_objects.begin(); it != selected_objects.end(); it++) {
    if( (*it).second == LM_TYPE_GATE) {
      return false;
    }
  }
  return true;
}

bool MainWin::selected_objects_are_removable() {
  std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it;
  if(selected_objects.size() == 0) return false;
  for(it = selected_objects.begin(); it != selected_objects.end(); it++) {
    if( (*it).second == LM_TYPE_GATE_PORT) {
      return false;
    }
  }
  return true;
}


void MainWin::on_popup_menu_set_port() {
  if(main_project) {
    LM_OBJECT_TYPE object_type = LM_TYPE_UNDEF;
    void * obj_ptr = NULL;
    unsigned int x, y;

    int layer = lmodel_get_layer_num_by_type(main_project->lmodel, LM_LAYER_TYPE_LOGIC);
    if(layer == -1) {
      error_dialog("Error", "There is no logic layer defined.");
      return;
    }

    if(RET_IS_NOT_OK(lmodel_get_object(main_project->lmodel, layer, 
				       last_click_on_real_x, last_click_on_real_y, 
				       &object_type, &obj_ptr))) {
      error_dialog("Error", "Unknown error.");
      return;
    }

    if(obj_ptr != NULL && object_type == LM_TYPE_GATE_PORT) {
      // user clicked on an alredy placed gate port -> derive gate object from it
      lmodel_gate_port_t * gate_port = (lmodel_gate_port_t *) obj_ptr;
      object_type = LM_TYPE_GATE;
      obj_ptr = gate_port->gate;
    }

    if(obj_ptr == NULL) {
      error_dialog("Error", "There is no gate. Maybe you are not working on the logic layer?");
      return;
    }

    if(object_type != LM_TYPE_GATE) {
      error_dialog("Error", "You clicked on something that is not a gate.");
      return;
    }

    assert(obj_ptr != NULL);

    lmodel_gate_t * gate = (lmodel_gate_t *) obj_ptr;
    if(gate->gate_template == NULL) {
      error_dialog("Error", "Please define a template type for that gate.");
      return;
    }

    // check, if the gate has defined ports
    if(lmodel_gate_template_get_num_ports(gate->gate_template) == 0) {
      error_dialog("Error", "Please define ports before you place them.");
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
    lmodel_gate_template_port_t * template_port = psWin.run();
    if(template_port != NULL) {
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
      char * n = lmodel_get_name( (*it).second, (*it).first);
      if(n != NULL) name = n;
    }

    GenericTextInputWin input(this, "Set name", "Please set a name", name);
    Glib::ustring str;
    if(input.run(name)) {
    
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
}

void MainWin::on_popup_menu_add_vertical_grid_line() {
  if(main_project != NULL && main_project->grid != NULL) {
    if(main_project->grid->grid_mode != USE_UNREGULAR_GRID)
      error_dialog("Error", "Please set the unregular grid mode in the grid configuration.");
    else {
      if(RET_IS_NOT_OK(grid_add_vertical_grid_line(main_project->grid, last_click_on_real_x)))
	error_dialog("Error", "Can't add grid line.");
      else {
	gcWin->update_grid_entries();
	project_changed();
	imgWin.update_screen();
      }
    }
  }
}

void MainWin::on_popup_menu_add_horizontal_grid_line() {
  if(main_project != NULL && main_project->grid != NULL) {
    if(main_project->grid->grid_mode != USE_UNREGULAR_GRID)
      error_dialog("Error", "Please set the unregular grid mode in the grid configuration.");
    else {
      if(RET_IS_NOT_OK(grid_add_horizontal_grid_line(main_project->grid, last_click_on_real_y)))
	error_dialog("Error", "Can't add grid line.");
      else {
	gcWin->update_grid_entries();
	project_changed();
	imgWin.update_screen();
      }
    }
  }
}

void MainWin::on_popup_menu_set_alignment_marker(MARKER_TYPE marker_type) {
  if(main_project && main_project->alignment_marker_set) {

    // check if the marker is already stored
    if(amset_get_marker(main_project->alignment_marker_set, main_project->current_layer, 
			marker_type) != NULL) {
      Gtk::MessageDialog dialog(*this, "Marker already placed. Do you want to replace it?", 
				true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
      dialog.set_title("Warning");      
      if(dialog.run() == Gtk::RESPONSE_YES) {
	if(RET_IS_NOT_OK(amset_replace_marker(main_project->alignment_marker_set, 
					      main_project->current_layer, 
					      marker_type, 
					      last_click_on_real_x, last_click_on_real_y)))
	  error_dialog("Error", "Error: Can't replace marker.");	
      }
    }
    else {
      if(RET_IS_NOT_OK(amset_add_marker(main_project->alignment_marker_set, 
					main_project->current_layer, 
					marker_type, 
					last_click_on_real_x, last_click_on_real_y)))
	error_dialog("Error", "Error: Can't add marker.");	      
    }
    //amset_print(main_project->alignment_marker_set);

    project_changed();

    menu_manager->set_menu_item_sensitivity("/MenuBar/LayerMenu/LayerAlignment", 
					    amset_complete(main_project->alignment_marker_set) ? true : false);

    imgWin.update_screen();
  }
}


void MainWin::on_menu_logic_auto_name_gates(AUTONAME_ORIENTATION orientation) {
  if(main_project != NULL) {

    Gtk::MessageDialog dialog(*this, "The operation may destroy previously set names. Are you sure you want name all gates?", 
			      true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
    dialog.set_title("Warning");
    if(dialog.run() == Gtk::RESPONSE_NO) return;
    dialog.hide();

    int layer = lmodel_get_layer_num_by_type(main_project->lmodel, LM_LAYER_TYPE_LOGIC);
    if(layer == -1) {
      error_dialog("Error", "There is no logic layer defined.");
      return;
    }

    ipWin = new InProgressWin(this, "Naming gates", "Please wait while generating names.");
    ipWin->show();

    signal_auto_name_finished_.connect(sigc::mem_fun(*this, &MainWin::on_auto_name_finished));
    thread = Glib::Thread::create(sigc::bind<AUTONAME_ORIENTATION>(sigc::mem_fun(*this, &MainWin::auto_name_gates_thread), 
								   orientation), false);

  }
}


void MainWin::auto_name_gates_thread(AUTONAME_ORIENTATION orientation) {
  int layer = lmodel_get_layer_num_by_type(main_project->lmodel, LM_LAYER_TYPE_LOGIC);
  ret_t ret = lmodel_autoname_gates(main_project->lmodel, layer, orientation);
  signal_auto_name_finished_(ret);
}


void MainWin::on_auto_name_finished(ret_t ret) {
  if(ipWin) {
    ipWin->close();
    delete ipWin;
    ipWin = NULL;
  }
  if(RET_IS_NOT_OK(ret)) {
    error_dialog("Error", "naming failed");
  }
  else {
    project_changed();
    //imgWin.update_screen(); // XXX results in a Fatal IO error 11 (Resource temporarily unavailable) on X server :0.0.
  }
}

void MainWin::on_menu_logic_interconnect() {
  std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it1, it2;
  if(selected_objects.size() >= 2) {

    for(it1 = selected_objects.begin(); it1 != selected_objects.end(); it1++)
      if(lmodel_object_is_connectable((*it1).second) == false) {
	error_dialog("Error", "One of the objects you selected can not have connections at all.");
	return;
      }

    //    for(it1 = selected_objects.begin(); it1 != selected_objects.end(); it1++)
    //  lmodel_set_select_state((*it1).second, (*it1).first, SELECT_STATE_NOT);
    

    it1 = selected_objects.begin();

    for(it2 = selected_objects.begin(); it2 != selected_objects.end(); it2++) {

      if(RET_IS_NOT_OK(lmodel_connect_objects((*it1).second, (*it1).first,
					      (*it2).second, (*it2).first))) {
	error_dialog("Error", "Can't connect objects.");
	  
      }
    }

    //for(it1 = selected_objects.begin(); it1 != selected_objects.end(); it1++)
    //  lmodel_set_select_state((*it1).second, (*it1).first, SELECT_STATE_DIRECT);

    project_changed();
    imgWin.update_screen();
  }
}

void MainWin::on_menu_logic_isolate() {
  std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it;
  if(selected_objects.size() >= 1) {

    for(it = selected_objects.begin(); it != selected_objects.end(); it++) {
  
      //lmodel_set_select_state((*it).second, (*it).first, SELECT_STATE_NOT);
      if(RET_IS_NOT_OK(lmodel_remove_all_connections_from_object((*it).second, (*it).first))) {
	error_dialog("Error", "Can't isolate object.");
      }
      //lmodel_set_select_state((*it).second, (*it).first, SELECT_STATE_DIRECT);
    }
    project_changed();
    imgWin.update_screen();
  }

}

void MainWin::on_menu_logic_connection_inspector() {
  if(main_project != NULL && ciWin != NULL) ciWin->show();
}


void MainWin::on_menu_view_grid_config() {

  if(main_project != NULL && gcWin != NULL) {
    gcWin->show();
  }
 
}

void MainWin::on_grid_config_changed() {
  if(gcWin) {
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
  Gtk::FileChooserDialog dialog("Please select a background image", Gtk::FILE_CHOOSER_ACTION_OPEN);
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

    ipWin = new InProgressWin(this, "Importing", 
			      "Please wait while importing background image and calculate the prescaled images.");
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
  if(RET_IS_NOT_OK(gr_import_background_image(main_project->bg_images[main_project->current_layer], 
				 0, 0, bg_filename.c_str()))) {
    debug(TM, "Can't import image file");
  }
  else {
    if(RET_IS_NOT_OK(scalmgr_recreate_scalings_for_layer(main_project->scaling_manager, 
							 main_project->current_layer)))
      debug(TM, "Can't recreate scaled images.");

#ifdef MAP_FILES_ON_DEMAND
	if(main_project != NULL && 
	   RET_IS_NOT_OK(gr_reactivate_mapping(main_project->bg_images[main_project->current_layer]))) {
	  debug(TM, "mapping image failed");
	}
#endif

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



void MainWin::on_menu_logic_clear_logic_model() {
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
      ciWin->objects_removed();
    }
    break;
  case(Gtk::RESPONSE_CANCEL):
  default:
    break;
  }

}

void MainWin::remove_objects() {

  if(main_project != NULL) {

    int layer = lmodel_get_layer_num_by_type(main_project->lmodel, LM_LAYER_TYPE_LOGIC);
    if(layer == -1) {
      error_dialog("Error", "There is no logic layer defined. Please define layer types.");
    }
    else {
      std::set< std::pair<void *, LM_OBJECT_TYPE> >::const_iterator it;

      for(it = selected_objects.begin(); it != selected_objects.end(); it++) {
	
	if(RET_IS_NOT_OK(lmodel_remove_object_by_ptr(main_project->lmodel, layer, (*it).first, (*it).second))) {
	  error_dialog("Error", "Can't remove object(s) from logic model");
	}
      }
      
      selected_objects.erase(selected_objects.begin(), selected_objects.end());
      highlighted_objects.clear();

      menu_manager->set_menu_item_sensitivity("/MenuBar/LogicMenu/LogicClearLogicModelInSelection", false);
      imgWin.update_screen(); 
      ciWin->objects_removed();
    }
  }
}

void MainWin::on_menu_layer_clear_background_image() {
  Gtk::MessageDialog dialog(*this, "Clear background image",
			    false /* use_markup */, Gtk::MESSAGE_QUESTION,
			    Gtk::BUTTONS_OK_CANCEL);
  dialog.set_secondary_text("Are you sure you want to clear the background image for the current layer?");
  int result = dialog.run();
  switch(result) {
  case(Gtk::RESPONSE_OK):
    if(RET_IS_NOT_OK(gr_map_clear(main_project->bg_images[main_project->current_layer])))
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

    if(RET_IS_OK(amset_calc_transformation(main_project->alignment_marker_set,
					   scaling_x, scaling_y, shift_x, shift_y))) {
 
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
    if(RET_IS_NOT_OK(amset_apply_transformation_to_markers(main_project->alignment_marker_set,
							   scaling_x, scaling_y, shift_x, shift_y))) {
      error_dialog("Error", "Can't apply transformation.");
    }
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

