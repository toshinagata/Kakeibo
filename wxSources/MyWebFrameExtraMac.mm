/////////////////////////////////////////////////////////////////////////////
// Purpose:     wxVueRunner Project
// Author:      Toshi Nagata
// Created:     2025/12/27
// Copyright:   (c) 2025 Toshi Nagata
// Licence:     GPL 3.0
/////////////////////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include "MyWebFrame.h"
#import <WebKit/WebKit.h>

int EnableDeveloperModeInWebView(wxWebView *webView)
{
  WKWebView *wkWebView = static_cast<WKWebView *>(webView->GetNativeBackend());
  [wkWebView.configuration.preferences setValue:@YES forKey:@"developerExtrasEnabled"];
  return 0;
}
