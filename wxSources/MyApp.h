/////////////////////////////////////////////////////////////////////////////
// Purpose:     Minumum wxWidgets Application
// Author:      Toshi Nagata
// Created:     2025/12/27
// Copyright:   (c) 2025 Toshi Nagata
// Licence:     GPL 3.0
/////////////////////////////////////////////////////////////////////////////

#ifndef MYAPP_H
#define MYAPP_H

#include <wx/wx.h>

class MyFrame;
class MyWebFrame;
class MyApp : public wxApp
{
//  virtual bool Initialize(int& argc, wxChar **argv) override;
  virtual bool OnInit() override;
  virtual int OnExit() override;
  void OnCustomEvent(wxCommandEvent &event);
  void OnIdle(wxIdleEvent &event);
  wxProcess *m_process;
public:
  MyFrame *m_frame;
  MyWebFrame *m_webFrame;
  int m_port;
  bool m_useWebView;
  
  wxDECLARE_EVENT_TABLE();
};

wxDECLARE_APP(MyApp);

#endif // MYAPP_H
