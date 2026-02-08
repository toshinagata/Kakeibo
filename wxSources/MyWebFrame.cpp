/////////////////////////////////////////////////////////////////////////////
// Purpose:     wxVueRunner Project
// Author:      Toshi Nagata
// Created:     2025/12/27
// Copyright:   (c) 2025 Toshi Nagata
// Licence:     GPL 3.0
/////////////////////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include "MyWebFrame.h"

MyWebFrame::MyWebFrame(wxString &urlString)
    : wxFrame(nullptr, wxID_ANY, "", wxDefaultPosition, wxDefaultSize)
{
  wxString appName = wxTheApp->GetAppDisplayName();
  this->SetTitle(appName);
  m_webview = wxWebView::New(this, -1, "", wxDefaultPosition, wxSize(720, 520), wxWebViewBackendDefault, 0, "");
  wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
  sizer->Add(m_webview, 1, wxEXPAND);
  this->SetSizerAndFit(sizer);

  Bind(wxEVT_CLOSE_WINDOW, &MyWebFrame::OnClose, this);

#if defined(__WXMSW__)

#endif
  
#if defined(__WXMAC__)
  auto menuBar = new wxMenuBar;
  auto menuFile = new wxMenu;
  menuFile->Append(wxID_EXIT, "Quit");
  menuBar->Append(menuFile, "File");
  SetMenuBar(menuBar);
  Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, wxID_EXIT);
#endif

#if defined(__WXMAC__)
  EnableDeveloperModeInWebView(m_webview);
#endif
  
  this->Show();
  m_webview->LoadURL(urlString);  
}

MyWebFrame::~MyWebFrame()
{
}

void
MyWebFrame::OnClose(wxCloseEvent& event)
{
    Destroy();
}
