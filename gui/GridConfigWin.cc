#include "GridConfigWin.h"
#include "lib/grid.h"

#include <gdkmm/window.h>
#include <iostream>
#include <gtkmm/stock.h>

GridConfigWin::GridConfigWin(Gtk::Window *parent,
			     unsigned int grid_offset_x, unsigned int grid_offset_y,
			     double grid_dist_x, double grid_dist_y) : 
  m_Frame_Offset("Offset"),

  // value, lower, upper, step_increment
  m_Adjustment_OffsetX(grid_offset_x, 0.0, 512.0, 1),
  m_Scale_OffsetX(m_Adjustment_OffsetX),
  m_Label_OffsetX("X"),

  m_Adjustment_OffsetY(grid_offset_y, 0.0, 512.0, 1),
  m_Scale_OffsetY(m_Adjustment_OffsetY),
  m_Label_OffsetY("Y"),

  m_Frame_Dist("Distance"),

  //m_Adjustment_DistX(grid_dist_x, 10.0, 11.0, 0.01, 1, 0),
  //m_Scale_DistX(m_Adjustment_DistX),
  m_Label_DistX("X"),

  //m_Adjustment_DistY(grid_dist_y, 1.0, 50.0, 0.01),
  //m_Scale_DistY(m_Adjustment_DistY),
  m_Label_DistY("Y"),

  m_Button_Ok("Ok") {

  set_title("Grid configuration");
  set_default_size(400, 200);
  set_border_width(5);
  set_transient_for(*parent);

  add(m_Box);

  m_Box.pack_start(m_Frame_Offset, Gtk::PACK_EXPAND_WIDGET);
  m_Frame_Offset.add(m_Box_Offset);
  m_Box_Offset.add(m_Box_OffsetX);
  m_Box_Offset.add(m_Box_OffsetY);
  m_Box_OffsetX.pack_start(m_Label_OffsetX, Gtk::PACK_SHRINK, 20);
  m_Box_OffsetX.pack_start(m_Scale_OffsetX, Gtk::PACK_EXPAND_WIDGET);
  m_Box_OffsetY.pack_start(m_Label_OffsetY, Gtk::PACK_SHRINK, 20);
  m_Box_OffsetY.pack_start(m_Scale_OffsetY, Gtk::PACK_EXPAND_WIDGET);

  m_Box.pack_start(m_Frame_Dist, Gtk::PACK_EXPAND_WIDGET);
  m_Frame_Dist.add(m_Box_Dist);
  m_Box_Dist.add(m_Box_DistX);
  m_Box_Dist.add(m_Box_DistY);
  m_Box_DistX.pack_start(m_Label_DistX, Gtk::PACK_SHRINK, 20);
  //m_Box_DistX.pack_start(m_Scale_DistX, Gtk::PACK_EXPAND_WIDGET);
  m_Box_DistX.pack_start(m_Entry_DistX, Gtk::PACK_EXPAND_WIDGET);
  m_Box_DistY.pack_start(m_Label_DistY, Gtk::PACK_SHRINK, 20);
  //m_Box_DistY.pack_start(m_Scale_DistY, Gtk::PACK_EXPAND_WIDGET);
  m_Box_DistY.pack_start(m_Entry_DistY, Gtk::PACK_EXPAND_WIDGET);

  m_Adjustment_OffsetX.signal_value_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_offset_x_changed));
  m_Adjustment_OffsetY.signal_value_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_offset_y_changed));
  //m_Adjustment_DistX.signal_value_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_dist_x_changed));
  //m_Adjustment_DistY.signal_value_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_dist_y_changed));
  m_Entry_DistX.signal_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_dist_x_changed));
  m_Entry_DistY.signal_changed().connect(sigc::mem_fun(*this, &GridConfigWin::on_dist_y_changed));

  char * tmp = (char *)alloca(50);
  snprintf(tmp, 50, "%f", grid_dist_x);
  m_Entry_DistX.set_text(tmp);

  snprintf(tmp, 50, "%f", grid_dist_y);
  m_Entry_DistY.set_text(tmp);

  m_Button_Ok.signal_clicked().connect( sigc::mem_fun(*this, &GridConfigWin::on_ok_button_clicked) );
  m_Button_Ok.set_label("Ok");
  m_Button_Ok.set_use_stock(true);
  m_Button_Ok.set_relief(Gtk::RELIEF_NORMAL);
  m_Box.pack_start(m_Button_Ok, Gtk::PACK_SHRINK);

  show_all_children();
}

GridConfigWin::~GridConfigWin() {
}


void GridConfigWin::on_offset_x_changed() {
  signal_changed_();
}

void GridConfigWin::on_offset_y_changed() {
  signal_changed_();
}

void GridConfigWin::on_dist_x_changed() {
  signal_changed_();
}

void GridConfigWin::on_dist_y_changed() {
  signal_changed_();
}

void GridConfigWin::on_ok_button_clicked() {
  hide();
}


unsigned int GridConfigWin::get_grid_offset_x() {
  return (unsigned int) m_Adjustment_OffsetX.get_value();
}

unsigned int GridConfigWin::get_grid_offset_y() {
  return (unsigned int) m_Adjustment_OffsetY.get_value();
}

double GridConfigWin::get_grid_dist_x() {
  double v = atof(m_Entry_DistX.get_text().c_str());
  if(v < 0 || v == HUGE_VAL) {
    m_Entry_DistX.set_text("0");
    v = 0;
  }
  return v;
  //return m_Adjustment_DistX.get_value();
}

double GridConfigWin::get_grid_dist_y() {
  double v = atof(m_Entry_DistY.get_text().c_str());
  if(v < 0 || v == HUGE_VAL) {
    m_Entry_DistY.set_text("0");
    v = 0;
  }
  return v;
  //return m_Adjustment_DistY.get_value();
}

sigc::signal<void>& GridConfigWin::signal_changed() {
  return signal_changed_;
}
