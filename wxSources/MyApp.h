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
class MyApp : public wxApp
{
//  virtual bool Initialize(int& argc, wxChar **argv) override;
  virtual bool OnInit() override;
  virtual int OnExit() override;
  void OnIdle(wxIdleEvent &event);
  MyFrame *m_frame;
  wxProcess *m_process;
public:
  int m_port;
  wxString m_randomId;
  
  wxDECLARE_EVENT_TABLE();
};

wxDECLARE_APP(MyApp);

#endif // MYAPP_H
