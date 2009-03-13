#ifndef __GRIDCONFIGWIN_H__
#define __GRIDCONFIGWIN_H__

#include <gtkmm.h>

#include "lib/project.h"

class GridConfigWin : public Gtk::Window {
 public:
  GridConfigWin(Gtk::Window *parent,
		unsigned int grid_offset_x, unsigned int grid_offset_y,
		double grid_dist_x, double grid_dist_y);
  virtual ~GridConfigWin();
        
  sigc::signal<void>& signal_changed();

  unsigned int get_grid_offset_x();
  unsigned int get_grid_offset_y();
  double get_grid_dist_x();
  double get_grid_dist_y();
  
  private:

  sigc::signal<void>  signal_changed_;

  // Child widgets

  Gtk::VBox m_Box;

  Gtk::Frame m_Frame_Offset;
  Gtk::VBox  m_Box_Offset;
  Gtk::Adjustment m_Adjustment_OffsetX;
  Gtk::HScale m_Scale_OffsetX;
  Gtk::Label m_Label_OffsetX;
  Gtk::HBox m_Box_OffsetX;

  Gtk::Adjustment m_Adjustment_OffsetY;
  Gtk::HScale m_Scale_OffsetY;
  Gtk::Label m_Label_OffsetY;
  Gtk::HBox m_Box_OffsetY;

  Gtk::Frame m_Frame_Dist;
  Gtk::VBox  m_Box_Dist;
  Gtk::Entry m_Entry_DistX;
  //Gtk::Adjustment m_Adjustment_DistX;
  //Gtk::HScale m_Scale_DistX;
  Gtk::Label m_Label_DistX;
  Gtk::HBox m_Box_DistX;

  //Gtk::Adjustment m_Adjustment_DistY;
  //Gtk::HScale m_Scale_DistY;
  Gtk::Entry m_Entry_DistY;
  Gtk::Label m_Label_DistY;
  Gtk::HBox m_Box_DistY;

  Gtk::Button m_Button_Ok;

  // Signal handlers:
  virtual void on_offset_x_changed();
  virtual void on_offset_y_changed();
  virtual void on_dist_x_changed();
  virtual void on_dist_y_changed();

  virtual void on_ok_button_clicked();
};

#endif
