#ifndef __GRIDCONFIGWIN_H__
#define __GRIDCONFIGWIN_H__

#include <gtkmm.h>
#include "lib/project.h"

class GridConfigWin {


 public:

  GridConfigWin(Gtk::Window *parent, grid_t * grid);
  virtual ~GridConfigWin();
        
  sigc::signal<void>& signal_changed();

  void show();


 private:
  Gtk::Window *parent;
  grid_t orig_grid;
  grid_t * grid;

  sigc::signal<void>  signal_changed_;

  Gtk::Dialog * pDialog;

  Gtk::Button* p_ok_button;
  Gtk::Button* p_cancel_button;
  Gtk::CheckButton * p_horizontal_checkbutton;
  Gtk::CheckButton * p_vertical_checkbutton;

  
  Gtk::HScale * p_scale_offset_x;
  Gtk::Adjustment * p_adj_offset_x;
  Gtk::HScale * p_scale_offset_y;
  Gtk::Adjustment * p_adj_offset_y;

  Gtk::HScale * p_scale_dist_x;
  Gtk::Adjustment * p_adj_dist_x;
  Gtk::HScale * p_scale_dist_y;
  Gtk::Adjustment * p_adj_dist_y;

  Gtk::Entry * p_entry_dist_x;
  Gtk::Entry * p_entry_dist_y;

  // Signal handlers:

  virtual void on_offset_x_changed();
  virtual void on_offset_y_changed();
  virtual void on_dist_x_changed();
  virtual void on_dist_y_changed();
  virtual void on_entry_dist_x_changed();
  virtual void on_entry_dist_y_changed();

  virtual void on_ok_button_clicked();
  virtual void on_cancel_button_clicked();

  virtual void on_horz_checkb_clicked();
  virtual void on_vert_checkb_clicked();
};

#endif
