#
#   Makefile for wxVueRunner Project
#
#  Created by Toshi Nagata on 2025/12/28.
#  Copyright 2025 Toshi Nagata. All rights reserved.
#
#  License: GPL-3
#

PWD ?= $(shell pwd)

#  The zip command in macOS is broken. This is the path for the unbroken one.
#  (Installed by Homebrew)
ZIP = /usr/local/homebrew/opt/zip/bin/zip

#  wxWidgets base directory
#  (Depends on your installation)
WX_DIR = $(PWD)/../../wxWidgets-3.2.5
WX_VER = 3.2

#  Application name
APPNAME = $(shell echo $${PRODUCT_NAME:-wxVueRunner})

#  Object files
OBJECTS = MyApp.o MyFrame.o MyWebFrame.o mongoose.o


#  wx libraries
WXLIB_LIST = core,base,gl,adv


ifeq (,$(findstring Windows,$(OS)))
  HOST_SYS:= $(shell uname -s)
else
  HOST_SYS= Windows
endif

ifeq ($(TARGET_PLATFORM),MSW)
  TARGET_SYS= Windows
else
  ifeq ($(TARGET_PLATFORM),MAC)
    TARGET_SYS= Darwin
  else
    ifeq ($(TARGET_PLATFORM),LINUX)
      TARGET_SYS= Linux
    endif
  endif
endif

#  Target platform and cross compile flag
ifneq ($(TARGET_SYS),$(HOST_SYS))
  ifeq ($(TARGET_SYS),Windows)
    ifeq ($(TARGET_ARCH),x86_64)
      CROSS=mingw64
    else
      CROSS=mingw32
    endif
  endif
else
  CROSS=
endif

ifeq ($(TARGET_PLATFORM),MSW)
 CPP_EXTRA_FLAGS =
 ifeq ($(CROSS),)
  ifeq ($(TARGET_ARCH),i686)
   BUILD_DIR = build-win32
   TOOL_PREFIX = i686-w64-mingw32-
   PATH_PREFIX=/mingw32/bin/
   LIB_SUFFIX = -$(WX_VER)-i686-w64-mingw32
   HOST_CC = "gcc"
  else
   BUILD_DIR = build-win
   TOOL_PREFIX = x86_64-w64-mingw32-
   PATH_PREFIX=/mingw64/bin/
   LIB_SUFFIX = -$(WX_VER)-x86_64-w64-mingw32
  endif
 else
  PATH_PREFIX=/usr/local/homebrew/bin/
  ifeq ($(TARGET_ARCH),i686)
   BUILD_DIR = build-win32
   TOOL_PREFIX = i686-w64-mingw32-
   LIB_SUFFIX = -$(WX_VER)-i686-w64-mingw32
   HOST_CC = "gcc -m32"
   WEBVIEW_LIB = $(WX_DIR)/3rdparty/webview2/build/native/x86/WebView2Loader.dll
  else
   ifeq ($(TARGET_ARCH),x86_64)
    BUILD_DIR = build-win
    TOOL_PREFIX = x86_64-w64-mingw32-
    LIB_SUFFIX = -$(WX_VER)-x86_64-w64-mingw32
    HOST_CC = "gcc"
    WEBVIEW_LIB = $(WX_DIR)/3rdparty/webview2/build/native/x64/WebView2Loader.dll
   endif
  endif
  WINE_PATH="/Applications/Wine Stable.app/Contents/Resources/wine/bin"
 endif
 WX_LIB_DIR = $(WX_DIR)/$(BUILD_DIR)/lib
 WX_ARCH_DIR = $(WX_LIB_DIR)/wx/include/$(TOOL_PREFIX)msw-unicode-static-$(WX_VER)
 WX_CPPFLAGS = -I$(PWD)/../wxSources -isystem $(WX_ARCH_DIR) -isystem $(WX_DIR)/include -D_LARGEFILE_SOURCE=unknown -D__WXMSW__
 WX_LDFLAGS = -L$(WX_LIB_DIR) -Wl,--subsystem,windows -mwindows $(WX_LIB_DIR)/libwx_mswu_gl$(LIB_SUFFIX).a -lopengl32 -lglu32 $(WX_LIB_DIR)/libwx_mswu$(LIB_SUFFIX).a -lwxregexu$(LIB_SUFFIX) -lwxexpat$(LIB_SUFFIX) -lwxtiff$(LIB_SUFFIX) -lwxjpeg$(LIB_SUFFIX) -lwxpng$(LIB_SUFFIX) -lwxzlib$(LIB_SUFFIX) -lwxscintilla$(LIB_SUFFIX) -lrpcrt4 -loleaut32 -lole32 -luuid -luxtheme -lwinspool -lwinmm -lshell32 -lshlwapi -lcomctl32 -lcomdlg32 -ladvapi32 -lversion -lwsock32 -lgdi32 -loleacc -lwinhttp -limm32 -lws2_32 $(WEBVIEW_LIB)
 LD_EXTRA_FLAGS = -static
 EXECUTABLE = _$(APPNAME).exe_
 FINAL_EXECUTABLE = $(APPNAME).exe
 EXE_SUFFIX = .exe
 PRODUCT_DIR = $(APPNAME)
 PRODUCT = $(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
endif

ifeq ($(TARGET_PLATFORM),LINUX)
 CPP_EXTRA_FLAGS = 
 ifeq ($(CROSS),)
  ifeq ($(TARGET_ARCH),i686)
   BUILD_DIR = $(shell basename `pwd`)
   TOOL_PREFIX =
   PATH_PREFIX =
   LIB_SUFFIX = -$(WX_VER)
   HOST_CC = "gcc"
   APPIMAGE_TOOL_ARCH = i686
   APPIMAGE_PRODUCT_ARCH = i386
  else
   BUILD_DIR = $(shell basename `pwd`)
   TOOL_PREFIX =
   PATH_PREFIX=
   LIB_SUFFIX = -$(WX_VER)
   APPIMAGE_TOOL_ARCH = x86_64
   APPIMAGE_PRODUCT_ARCH = x86_64
  endif
 else
  PATH_PREFIX=/usr/local/homebrew/bin/
  ifeq ($(TARGET_ARCH),i686)
   BUILD_DIR = build-win32
   TOOL_PREFIX = i686-w64-mingw32-
   LIB_SUFFIX = -$(WX_VER)-i686-w64-mingw32
   HOST_CC = "gcc -m32"
  else
   ifeq ($(TARGET_ARCH),x86_64)
    BUILD_DIR = build-win
    TOOL_PREFIX = x86_64-w64-mingw32-
    LIB_SUFFIX = -$(WX_VER)-x86_64-w64-mingw32
    HOST_CC = "gcc"
   endif
  endif
 endif
 CPP_NAME = g++
 CC_NAME = gcc
 WX_LIB_DIR = $(WX_DIR)/$(BUILD_DIR)/lib
 WX_ARCH_DIR = $(WX_LIB_DIR)/wx/include/gtk3-unicode-static-$(WX_VER)
 WX_CPPFLAGS = -I$(PWD)/../wxSources -I$(WX_ARCH_DIR) -I$(WX_DIR)/include -D_FILE_OFFSET_BITS=64 -D__WXGTK__ -pthread
 WX_LDFLAGS = -fPIC -L$(WX_LIB_DIR) -pthread $(WX_LIB_DIR)/libwx_gtk3u_gl$(LIB_SUFFIX).a -lGL -lGLU -lEGL -lwayland-egl -lwayland-client $(WX_LIB_DIR)/libwx_gtk3u$(LIB_SUFFIX).a -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0 -pthread -lglib-2.0 -lX11 -lXxf86vm -lSM -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lpangoft2-1.0 -lpango-1.0 -lgobject-2.0 -lglib-2.0 -lfontconfig -lfreetype -lpng -ljpeg -lz -lwxregexu$(LIB_SUFFIX) -lwxtiff$(LIB_SUFFIX) -lwxscintilla$(LIB_SUFFIX) -lz -lexpat -lpng -lz -ldl -lm
 LD_EXTRA_FLAGS = -L$(PWD)/build/lib -Wl,-rpath='$$ORIGIN/lib' -shared -pie -Wl,-export-dynamic
 LD_EXTRA_FLAGS_WEBVIEW_SO = -L$(PWD)/build/lib -Wl,-rpath='$$ORIGIN/..'
 EXECUTABLE = _$(APPNAME)_
 FINAL_EXECUTABLE = $(APPNAME)
 EXE_SUFFIX =
 PRODUCT_DIR = $(APPNAME)
 PRODUCT = $(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
endif

CPP_NAME ?= g++
CC_NAME ?= gcc

CPP = $(PATH_PREFIX)$(TOOL_PREFIX)$(CPP_NAME)
CC = $(PATH_PREFIX)$(TOOL_PREFIX)$(CC_NAME)
AR = $(PATH_PREFIX)$(TOOL_PREFIX)ar
RANLIB = $(PATH_PREFIX)$(TOOL_PREFIX)ranlib

ifeq ($(MAKECMDGOALS),debug)
 DEBUG = 1
endif

ifeq ($(CONFIGURATION),Debug)
 DEBUG = 1
endif

ifeq ($(DEBUG),1)
 DESTPREFIX = build/debug
 COPT = -O0 -g
else
 DESTPREFIX = build/release
 COPT = -O2 -g
endif
MAKEDIR = $(PWD)
DESTDIR = $(PWD)/$(DESTPREFIX)
CFLAGS = $(CPPFLAGS) $(COPT) $(CPP_EXTRA_FLAGS) $(WX_CPPFLAGS)
LDFLAGS = $(LD_EXTRA_FLAGS) $(WX_LDFLAGS)

export CFLAGS
export LDFLAGS
export DESTDIR
export CC
export CPP
export AR
export TARGET_PLATFORM
export TARGET_ARCH
export RANLIB
export PATH_PREFIX

release: all

debug: all

all: make_dir $(DESTPREFIX)/$(PRODUCT)

#  Create destination directory
#  DESTPREFIX is "build/debug" or "build/release"
make_dir:
	echo "Creating $(DESTPREFIX)/$(PRODUCT)"
	mkdir -p $(DESTPREFIX)
	for i in $(SUBDIRS); do mkdir -p $(DESTPREFIX)/$$i; done

#  Create resource file for windows
ifeq ($(TARGET_PLATFORM),MSW)
RESOURCE = MyApp_rc.o
$(DESTPREFIX)/$(RESOURCE) : $(PWD)/../wxSources/MyApp.rc
	$(PATH_PREFIX)$(TOOL_PREFIX)windres -i $< -o $@ -I$(WX_DIR)/include
endif

depend: cleandep $(DESTPREFIX) $(OBJECTS:%.o=$(DESTPREFIX)/%.d) $(EXTRA_OBJECTS:%.o=$(DESTPREFIX)/%.d)
	cat $(DESTPREFIX)/*.d > build/Makefile.depend

$(DESTPREFIX): make_dir

cleandep:
	rm -f build/Makefile.depend

-include build/Makefile.depend

$(DESTPREFIX)/%.d : $(PWD)/../wxSources/%.cpp
	$(CPP) -MM $< >$@ $(subst -arch ppc,,$(CFLAGS))

$(DESTPREFIX)/%.d : $(PWD)/../wxSources/%.c
	$(CC) -MM $< >$@ $(subst -arch ppc,,$(CFLAGS))

$(DESTPREFIX)/%.o : $(PWD)/../wxSources/%.cpp
	$(CPP) -c $< -o $@ $(CFLAGS)

$(DESTPREFIX)/%.o : $(PWD)/../wxSources/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

ALL_OBJECTS = $(OBJECTS) $(EXTRA_OBJECTS) $(RESOURCE)
DESTOBJECTS = $(addprefix $(DESTPREFIX)/,$(ALL_OBJECTS))
$(DESTPREFIX)/$(EXECUTABLE) : $(DESTPREFIX) $(DESTOBJECTS) $(PWD)/../Version
	sh $(PWD)/../record_build_date.sh >$(DESTPREFIX)/buildInfo.c
	$(CC) -c $(DESTPREFIX)/buildInfo.c -o $(DESTPREFIX)/buildInfo.o $(CFLAGS)
	$(CPP) -o $@ $(DESTOBJECTS) $(DESTPREFIX)/buildInfo.o $(WIN_FOPEN_O) $(CFLAGS) $(LDFLAGS) $(LUAJIT_LDFLAGS)

final_executable : $(DESTPREFIX)/$(EXECUTABLE) $(PWD)/../Tauri-Vue/dist
ifeq ($(TARGET_PLATFORM),MSW)
	rm -rf $(DESTPREFIX)/$(PRODUCT_DIR)
	mkdir -p $(DESTPREFIX)/$(PRODUCT_DIR)
	cp $(DESTPREFIX)/$(EXECUTABLE) $(DESTPREFIX)/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
	cp $(WEBVIEW_LIB) $(DESTPREFIX)/$(PRODUCT_DIR)
	cp $(PWD)/../Resources/icon.ico $(DESTPREFIX)/$(PRODUCT_DIR)
	cp -R $(PWD)/../Tauri-Vue/dist $(DESTPREFIX)/$(PRODUCT_DIR)
endif
ifeq ($(TARGET_PLATFORM),LINUX)
	rm -rf $(DESTPREFIX)/$(PRODUCT_DIR)
	mkdir -p $(DESTPREFIX)/$(PRODUCT_DIR)/lib
	cp $(DESTPREFIX)/$(EXECUTABLE) $(DESTPREFIX)/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
	cp -R $(PWD)/../Tauri-Vue/dist $(DESTPREFIX)/$(PRODUCT_DIR)
endif

$(DESTPREFIX)/$(PRODUCT) : final_executable $(WEBVIEW_SO)
ifeq ($(TARGET_PLATFORM),MSW)
ifneq ($(DEBUG),1)
	$(PATH_PREFIX)$(TOOL_PREFIX)strip $(DESTPREFIX)/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
endif
#	cp -R $(PWD)/../wxSources/lib $(DESTPREFIX)/$(PRODUCT_DIR)
ifneq ($(DEBUG),1)
	(cd build/release/$(PRODUCT_DIR) && rm -rf $(MAKEDIR)/../_latest_binaries/$(PRODUCT_NAME)_Win$(PRODUCT_SUFFIX).zip && $(ZIP) -r -UN=UTF8 $(MAKEDIR)/../_latest_binaries/$(PRODUCT_NAME)_Win$(PRODUCT_SUFFIX).zip * -x \*.DS_Store \*.svn*)
endif
endif
ifeq ($(TARGET_PLATFORM),LINUX)
ifneq ($(DEBUG),1)
	$(PATH_PREFIX)$(TOOL_PREFIX)strip $(DESTPREFIX)/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
endif
#	cp $(WEBVIEW_SO) $(DESTPREFIX)/$(PRODUCT_DIR)/lib
#	cp -R $(PWD)/../wxSources/lib $(DESTPREFIX)/$(PRODUCT_DIR)
endif

ifeq ($(TARGET_PLATFORM),MSW)
install: release

setup: build/release/$(PRODUCT_DIR)/$(FINAL_EXECUTABLE)
	mkdir -p $(PWD)/../_latest_binaries
ifneq ($(CROSS),)
	#($(WINE_PATH)/wine ../../Inno\ Setup\ 5/ISCC.exe ../wxSources/MyApp.iss || exit 1)
else
	#(/c/Program\ Files\ \(x86\)/Inno\ Setup\ 5/iscc ../wxSources/MyApp.iss || exit 1)
endif
endif

clean:
	rm -rf $(DESTPREFIX)
