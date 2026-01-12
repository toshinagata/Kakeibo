/////////////////////////////////////////////////////////////////////////////
// Purpose:     wxVueRunner Project
// Author:      Toshi Nagata
// Created:     2025/12/27
// Copyright:   (c) 2025 Toshi Nagata
// Licence:     GPL 3.0
/////////////////////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include "MyFrame.h"
#include <wx/stdpaths.h>

extern volatile char *gLastBuildString;
extern volatile char *gVersion;

MyFrame::MyFrame()
    : wxFrame(nullptr, wxID_ANY, "", wxDefaultPosition, wxSize(400, 180))
{
  //  hsizer1 --> App Icon
  //          |-> vsizer2 --> App name
  //                      |-> static text 1
  //                      |-> static text 2

  wxString appName = wxTheApp->GetAppDisplayName();
  this->SetTitle(appName);
  auto appNameText = new wxStaticText(this, -1, appName, wxDefaultPosition);
  auto font = appNameText->GetFont();
  font.MakeBold().MakeLarger();
  appNameText->SetFont(font);
  wxIcon icon(wxT("icon"), wxICON_DEFAULT_TYPE, 64, 64);
  wxBitmap *bitmap = new wxBitmap(icon);
  auto iconBitmap = new wxStaticBitmap(this, -1, *bitmap, wxDefaultPosition, wxSize(64, 64));
  wxString build_string = wxString::Format(wxT("バージョン %s\nビルド %s"), (const char *)gVersion, (const char *)gLastBuildString);
  auto build_text = new wxStaticText(this, -1, build_string, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
  m_text = new wxStaticText(this, -1, "", wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
  
  auto hsizer1 = new wxBoxSizer(wxHORIZONTAL);
  auto vsizer2 = new wxBoxSizer(wxVERTICAL);
  vsizer2->Add(appNameText, wxSizerFlags(1).Border(wxTOP, 10).Left());
  vsizer2->Add(m_text, wxSizerFlags(1).Border(wxBOTTOM, 5).Left());
  vsizer2->Add(build_text, wxSizerFlags(1).Border(wxBOTTOM, 10).Left());
  hsizer1->Add(iconBitmap, wxSizerFlags(0).Border(wxLEFT | wxRIGHT, 20).Center());
  hsizer1->Add(vsizer2, wxSizerFlags(1).Border(wxRIGHT, 10));
  this->SetSizerAndFit(hsizer1);
  Bind(wxEVT_CLOSE_WINDOW, &MyFrame::OnClose, this);

#if defined(__WXMSW__)
  this->SetBackgroundColour(*wxWHITE);
#endif
  
#if defined(__WXMAC__)
  auto menuBar = new wxMenuBar;
  auto menuFile = new wxMenu;
  menuFile->Append(wxID_EXIT, "Quit");
  menuBar->Append(menuFile, "File");
  SetMenuBar(menuBar);
  Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, wxID_EXIT);
#endif
  
  this->Show();
}


MyFrame::~MyFrame()
{
}

void
MyFrame::SetText(wxString text)
{
  wxSize size = m_text->GetSizeFromText(text);
  m_text->SetSize(size);
  m_text->SetLabel(text);
  this->GetSizer()->Fit(this);
}

void
MyFrame::OnClose(wxCloseEvent& event)
{
    Destroy();
}
