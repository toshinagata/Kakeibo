/////////////////////////////////////////////////////////////////////////////
// Purpose:     Minumum wxWidgets Application
// Author:      Toshi Nagata
// Created:     2025/12/27
// Copyright:   (c) 2025 Toshi Nagata
// Licence:     GPL 3.0
/////////////////////////////////////////////////////////////////////////////

#ifndef MYWEBFRAME_H
#define MYWEBFRAME_H

#include <wx/frame.h>
#include <wx/webview.h>

class MyWebFrame : public wxFrame
{
public:
  MyWebFrame(wxString &urlString);
  virtual ~MyWebFrame();
  void OnClose(wxCloseEvent& event);

  wxWebView *m_webview;
};

extern "C" int EnableDeveloperModeInWebView(wxWebView *webView);

#endif // MYWEBFRAME_H
