#define MyAppName "わたしの家計簿"
#define Version "0.2.0"
[Setup]
AppName = {#MyAppName}
AppVerName = {#MyAppName} {#Version}
DefaultDirName = {pf}\{#MyAppName}
DefaultGroupName = {#MyAppName}
UninstallDisplayIcon = {app}\{#MyAppName}.exe
OutputBaseFileName = Setup_{#MyAppName}
ArchitecturesInstallIn64BitMode = x64
SourceDir = ..\build-win
OutputDir = ..\_latest_binaries

[Files]
; Install 64-bit version if running in 64-bit mode, 32-bit version otherwise.

; x86_64 files
Source: "build\release\{#MyAppName}\{#MyAppName}.exe"; DestDir: {app}; Check: Is64BitInstallMode
Source: "build\release\{#MyAppName}\WebView2Loader.dll"; DestDir: {app}; Check: Is64BitInstallMode

; i386 files (the first one should be marked 'solidbreak')
;Source: "..\build-win32\build\release\{#MyAppName}\{#MyAppName}.exe"; DestDir: {app}; Check: Is64BitInstallMode; Flags: solidbreak
;Source: "..\build-win32\release\{#MyAppName}\WebView2Loader.dll"; DestDir: {app}; Check: Is64BitInstallMode

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppName}.exe"
Name: "{group}\Uninstall {#MyAppName}"; Filename: {uninstallexe}
