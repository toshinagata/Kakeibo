/////////////////////////////////////////////////////////////////////////////
// Purpose:     wxVueRunner Project
// Author:      Toshi Nagata
// Created:     2025/12/27
// Copyright:   (c) 2025 Toshi Nagata
// Licence:     GPL 3.0
/////////////////////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include <wx/socket.h>
#include <wx/stdpaths.h>
#include <wx/process.h>
#include <wx/msgdlg.h>
#include <wx/textfile.h>
#include <wx/filename.h>
#include <wx/ffile.h>
#include <wx/dir.h>
#include <wx/arrstr.h>

#include "MyApp.h"
#include "MyFrame.h"

#include "httplib.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

//  Shared variable to show the server status
std::atomic<int> server_status(0);

static std::thread *server_thread = nullptr;
static httplib::Server *svr = nullptr;

void
handlePost(json &j, httplib::Response &res)
{
  std::string cmd = j["cmd"];
  std::string ret;
  std::string type = "text/plain";

  //  std::string and wxString
  //  The strings in json and in res are std::string encoded in UTF-8.
  //  On UNIX-like systems like macOS and linux, the default encoding for wxString
  //  is UTF-8, so std::string and wxString can be seemlessly converted.
  //  On Windows, the default encoding for wxString is not UTF-8, so we must
  //  explicitly specify the encoding; *wxConvFileName for filenames, and
  //  wxConvUTF8 for other strings.
  
  if (j["id"] != wxGetApp().m_randomId.ToStdString()) {
    res.status = httplib::StatusCode::Unauthorized_401;
    return;
  }
  if (cmd == "homeDir") {
    ret = wxGetHomeDir().ToStdString(*wxConvFileName);
  } else if (cmd == "isAvailable") {
    ret = "ok";
  } else if (cmd == "join") {
    std::string dirPath = j["dirPath"];
    std::string file = j["file"];
    std::string pathSep = wxString(wxFileName::GetPathSeparator()).ToStdString(*wxConvFileName);
    ret = dirPath + pathSep + file;
  } else if (cmd == "mkdir") {
    std::string path = j["path"];
    json options = j["options"];
    bool recursive = false;
    if (options.contains("recursive")) {
      recursive = options["recursive"];
    }
    wxString wpath(path.c_str(), *wxConvFileName);
    bool b = wxFileName::Mkdir(wpath, wxS_DIR_DEFAULT, (recursive ? wxPATH_MKDIR_FULL : 0));
    ret = (b ? "ok" : "");
  } else if (cmd == "exists") {
    std::string path = j["path"];
    wxString wpath(path.c_str(), *wxConvFileName);
    bool b = wxFileName::Exists(wpath);
    ret = (b ? "ok" : "");
  } else if (cmd == "create") {
    std::string path = j["path"];
    wxString wpath(path.c_str(), *wxConvFileName);
    wxFile file(wpath, wxFile::write);
    ret = (file.IsOpened() ? "ok" : "");
  } else if (cmd == "rename") {
    std::string oldPath = j["oldPath"];
    std::string newPath = j["newPath"];
    wxString woldPath(oldPath.c_str(), *wxConvFileName);
    wxString wnewPath(newPath.c_str(), *wxConvFileName);
    bool b = ::wxRenameFile(woldPath, wnewPath);
    ret = (b ? "ok" : "");
  } else if (cmd == "remove") {
    std::string path = j["path"];
    wxString wpath(path.c_str(), *wxConvFileName);
    bool b = ::wxRemoveFile(wpath);
    ret = (b ? "ok" : "");
  } else if (cmd == "readTextFile") {
    std::string path = j["path"];
    wxString wpath(path.c_str(), *wxConvFileName);
    wxString contents;
    wxFFile file(wpath, "rt");
    if (file.IsOpened()) {
      bool b = file.ReadAll(&contents, wxConvUTF8);
      ret = (b ? contents.ToStdString(wxConvUTF8) : "");
    } else {
      ret = "";
    }
  } else if (cmd == "writeTextFile") {
    std::string path = j["path"];
    std::string text = j["text"];
    wxString wpath(path.c_str(), *wxConvFileName);
    wxString wxtext(text.c_str(), wxConvUTF8);
    wxFFile file(wpath, "wt");
    if (file.IsOpened()) {
      bool b = file.Write(wxtext, wxConvUTF8);
      ret = (b ? "ok" : "");
    } else {
      ret = "";
    }
  } else if (cmd == "readDir") {
    std::string path = j["path"];
    wxString wpath(path.c_str(), *wxConvFileName);
    wxDir dir(wpath);
    if (dir.IsOpened()) {
      wxString s = wxT("[");
      wxString fname;
      bool b = dir.GetFirst(&fname, wxEmptyString, wxDIR_FILES | wxDIR_DIRS | wxDIR_NO_FOLLOW);
      int count = 0;
      while (b) {
        fname.Replace(wxT("\""), wxT("\\\""));
        fname = wxT("\"") + fname + wxT("\"");
        if (count > 0) {
          s += wxT(",");
        }
        s += fname;
        count++;
        b = dir.GetNext(&fname);
      }
      s += wxT("]");
      ret = s.ToStdString(wxConvUTF8);
    } else {
      ret = "[]";
    }
  } else if (cmd == "terminate") {
    server_status = 4;  //  terminate is requested by the client
    //  Queue an idle event for the main thread (to notify this thread ended)
    wxIdleEvent* evt = new wxIdleEvent;
    wxTheApp->QueueEvent(evt);
//    svr->stop();
  }
  res.set_content(ret, type);
}

void
runServer(int port, std::string rootDir)
{
  try {
    auto ret = svr->set_mount_point("/", rootDir);
    if (!ret) {
      printf("return value %d\n", (int)ret);
    }
    svr->Post("/@vueRunner/", [](const httplib::Request &req, httplib::Response &res) {
      json j = json::parse(req.body);
      handlePost(j, res);
    });
    server_status = 1;
    svr->listen("127.0.0.1", port);
    server_status = 2;  //  Normal end of this thread
  } catch (...) {
    server_status = 3;  //  Abnormal end of this thread
  }
}

void
terminateServer()
{
  //  Initialize an httplib client
  wxString urlStr = wxString::Format(wxT("http://127.0.0.1:%d"), wxGetApp().m_port);
  httplib::Client cli(urlStr.utf8_string());
  httplib::Headers headers = {};
  std::string body = "{\"id\":\"" + wxGetApp().m_randomId.ToStdString() + "\"}";
  //  Post terminate command
  auto res = cli.Post("/@vueRunner/terminate", headers, body, "application/json");
}

FILE *fp = NULL;
void
writeLog(wxString str)
{
  if (fp == NULL) {
    wxString home = wxGetHomeDir();
    fp = fopen((home + wxT("/wxvuerunner.log")).utf8_str(), "wt");
  }
  if (fp != NULL) {
    fprintf(fp, "%s\n", (const char *)str.utf8_str());
    fflush(fp);
  }
}

#if 0
#define write_log(s) writeLog(s)
#else
#define write_log(s)
#endif

#if defined(__WXMAC__)
void
GetVersionNumbersFromInfoPlist(wxString path, int &major, int &minor)
{
  wxTextFile tfile;
  wxString infoPath = path + "/Contents/Info.plist";
  major = 0;
  minor = 0;
  if (!wxFile::Exists(infoPath)) {
    return;
  }
  tfile.Open(infoPath);
  write_log(wxT("Opening " + infoPath));
  if (tfile.IsOpened()) {
    wxString line = tfile.GetFirstLine();
    write_log(wxT("Reading " + infoPath));
    while (!tfile.Eof()) {
      if (line.Find("CFBundleShortVersionString") != wxNOT_FOUND) {
        write_log("CFBundleShortVersionString found: " + line);
        line = tfile.GetNextLine();
        long pos = line.Find("<string>");
        write_log("Next line: " + line);
        if (pos != wxNOT_FOUND) {
          wxString line1 = line.Mid(pos + 8);
          pos = line1.Find(".");
          if (pos == wxNOT_FOUND) {
            line1.ToInt(&major);
            minor = 0;
          } else {
            wxString line2 = line1.Mid(0, pos);
            line2.ToInt(&major);
            line1 = line1.Mid(pos);
            pos = line1.Find(".");
            if (pos != wxNOT_FOUND) {
              line1 = line1.Mid(0, pos);
            }
            line1.ToInt(&minor);
          }
          write_log(wxString::Format(wxT("major = %d, minor = %d"), major, minor));
          break;
        }
      }
      line = tfile.GetNextLine();
    }
    tfile.Close();
    write_log(wxT("Closing " + infoPath));
  }
}
#endif

wxBEGIN_EVENT_TABLE(MyApp, wxApp)
  EVT_IDLE(MyApp::OnIdle)
wxEND_EVENT_TABLE()

bool
MyApp::OnInit()
{
  //  Disable any wxLog functionality (otherwise ::exit() may crash)
  wxLog::EnableLogging(false);

  //  Examine which port is open
  int port = 8081;
  {
    wxIPV4address addr;
    wxSocketClient *sock = new wxSocketClient();
    sock->SetTimeout(1);
    addr.Hostname("127.0.0.1");
    while (1) {
      addr.Service(port);
      printf("Trying to connect to 127.0.0.1:%d...\n", port);
      write_log(wxString::Format(wxT("Trying to connect to 127.0.0.1:%d..."), port));
      if (sock->Connect(addr, true)) {
        sock->Close();
        port += 1;
        continue;
      }
      printf("Looks like port %d is not in use.\n", port);
      write_log(wxString::Format(wxT("Looks like port %d is not in use."), port));
      break;
    }
    m_port = port;
  }
  //  Create random id.
  {
    char s[16];
    std::mt19937 mt{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(0, 61);
    for (int i = 0; i < 15; i++) {
      int r = dist(mt);
      if (r < 26) {
        s[i] = 'A' + r;
      } else if (r < 52) {
        s[i] = 'a' + (r - 26);
      } else {
        s[i] = '0' + r - 52;
      }
    }
    s[15] = 0;
    m_randomId = wxString(s);
  }
  //  Determine the root directory.
  wxString distDir = wxStandardPaths::Get().GetResourcesDir() + wxT("/dist");
  
  //  Run the server in a separate thread.
  svr = new httplib::Server();
  server_thread = new std::thread(runServer, port, distDir.utf8_string());
  
  //  Run the browser
  //  TODO: Use wxWebView in the current process
  wxString urlStr;
  urlStr.Printf("http://127.0.0.1:%d/?id=%s", port, m_randomId.utf8_str());

  //  Wait until the port is available
  {
    wxIPV4address addr;
    wxSocketClient *sock = new wxSocketClient();
    sock->SetTimeout(10);
    addr.Hostname("127.0.0.1");
    addr.Service(port);
    while (1) {
      if (sock->Connect(addr, true)) {
        sock->Close();
        break;
      }
    }
  }

  wxString invoke;
  wxString browser;

#if defined(__WXMAC__)
  //  Check the Safari version. If less than 15, then avoid it
  {
    int major, minor;
    GetVersionNumbersFromInfoPlist("/Applications/Firefox.app", major, minor);
    if (major >= 115) {
      invoke = "open -a 'Firefox' -g " + urlStr;
      browser = "Firefox";
    } else {
      GetVersionNumbersFromInfoPlist("/Applications/Google Chrome.app", major, minor);
      if (major >= 116) {
        invoke = "open -a 'Google Chrome' -g " + urlStr;
        browser = "Google Chrome";
      } else {
        GetVersionNumbersFromInfoPlist("/Applications/Safari.app", major, minor);
        if (major >= 15) {
          invoke = "open -a 'Safari' -g " + urlStr;
          browser = "Safari";
        } else {
          wxMessageBox("Safari (15以上), Firefox (115以上) または Chrome (116以上) をインストールしてください。", "ブラウザのバージョン", wxOK);
          terminateServer();
          wxExit();
        }
      }
    }
    write_log(wxT("Invoking " + invoke));
    wxExecute(invoke);
    wxExecute(wxT("osascript -e 'tell application \"") + browser + wxT("\" to activate'"));
  }
#else
  wxLaunchDefaultBrowser(urlStr);
  browser = wxT("デフォルトブラウザ");
#endif
  
  m_frame = new MyFrame;
  m_frame->SetText(wxString::Format(browser + wxT("上, 127.0.0.1:%d\n で動作しています。"), port));
  m_frame->Show();

  return true;
}

int
MyApp::OnExit()
{
  if (svr != nullptr && (server_status == 1 || server_status == 4)) {
    server_status = 0;
    svr->stop();
  }
  if (server_thread->joinable())
    server_thread->join();
  return wxApp::OnExit();
}
wxIMPLEMENT_APP(MyApp);

void
MyApp::OnIdle(wxIdleEvent &WXUNUSED(event))
{
  if (svr != nullptr && server_status == 4) {
    server_status = 0;
    svr->stop();
    wxExit();
  }
}
