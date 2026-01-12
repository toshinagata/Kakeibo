/////////////////////////////////////////////////////////////////////////////
// Purpose:     Minumum wxWidgets Application
// Author:      Toshi Nagata
// Created:     2025/12/27
// Copyright:   (c) 2025 Toshi Nagata
// Licence:     GPL 3.0
/////////////////////////////////////////////////////////////////////////////

#ifndef MYFRAME_H
#define MYFRAME_H

#include <wx/frame.h>
class wxStaticText;
class MyFrame : public wxFrame
{
public:
  MyFrame();
  virtual ~MyFrame();

  void SetText(wxString text);
  void OnClose(wxCloseEvent& event);
  
  wxStaticText *m_text;
};

#endif // MYFRAME_H
