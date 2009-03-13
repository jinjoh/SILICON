#ifndef __SPLASHWIN_H__
#define __SPLASHWIN_H__

#include <gtkmm.h>

class SplashWin : public Gtk::Window {
 public:
  SplashWin(int delay_msec);
  ~SplashWin();

  void delay();
 protected:
  virtual bool on_expose_event (GdkEventExpose *event);

 private:
  Glib::RefPtr<Gdk::Pixbuf>     m_image;

  unsigned int  m_image_w;
  unsigned int  m_image_h;
};


#endif
