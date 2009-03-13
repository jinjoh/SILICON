#ifndef __NEWRPROJECTWINWIN_H__
#define __NEWPROJECTWIN_H__

#include <gtkmm.h>

class NewProjectWin : public Gtk::Window {
 public:
  NewProjectWin(Gtk::Window *parent);
  virtual ~NewProjectWin();
    
  // return 0 on error
  unsigned int get_width();
  unsigned int get_height();
  unsigned int get_layers();

  private:

  // Child widgets
  Gtk::VBox m_Box;

  Gtk::Frame m_Frame_Size;
  Gtk::VBox  m_Box_Size;
  Gtk::Entry m_Entry_Width;
  Gtk::Entry m_Entry_Height;
  Gtk::Label m_Label_Width;
  Gtk::Label m_Label_Height;


  Gtk::Frame m_Frame_Layers;
  Gtk::VBox  m_Box_Layers;
  Gtk::Entry m_Entry_Layers;
  
  Gtk::Button m_Button_Ok;

  // Signal handlers:
  virtual void on_ok_button_clicked();
};

#endif
