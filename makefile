# C++ toolchain
CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -O2
LDFLAGS ?=
SOFLAGS ?= -shared -fPIC
# Debug example:
#   make linux-x64 CXXFLAGS="-std=c++17 -Wall -O0 -g"

# Source layout
SRC=./Photino.Native
SRC_SHARED=$(SRC)/Shared
SRC_WIN=$(SRC)/Windows
SRC_MAC=$(SRC)/macOS
SRC_LIN=$(SRC)/Linux

# Output layout
DEST_PATH=./lib
DEST_PATH_X64=$(DEST_PATH)/x64
DEST_PATH_ARM64=$(DEST_PATH)/arm64
DEST_FILE=PhotinoX.Native

.PHONY: all windows mac-universal linux-x64 linux-arm64 \
		build-photino-windows build-photino-mac-universal \
		build-photino-linux-x64 build-photino-linux-arm64 \
		install-linux-dependencies clean-x64 clean-arm64

all:
	@echo "Use one of: windows | mac-universal | linux-x64 | linux-arm64"

windows: clean-x64 build-photino-windows

mac-universal: clean-x64 build-photino-mac-universal

linux-x64: clean-x64 install-linux-dependencies build-photino-linux-x64
linux-arm64: clean-arm64 install-linux-dependencies build-photino-linux-arm64

build-photino-windows:
	@echo "Windows build is not defined here. Use MSBuild/VS."

# macOS: universal dylib (x86_64 + arm64)
build-photino-mac-universal: | $(DEST_PATH_X64)
	cp $(SRC)/Exports.cpp $(SRC)/Exports.mm && \
	$(CXX) $(CXXFLAGS) $(SOFLAGS) $(LDFLAGS) \
		-arch x86_64 \
		-arch arm64 \
		-framework Cocoa \
		-framework WebKit \
		-framework UserNotifications \
		-framework Security \
		-o $(DEST_PATH_X64)/$(DEST_FILE).dylib \
		$(SRC)/Photino.Mac.AppDelegate.mm \
		$(SRC)/Photino.Mac.UiDelegate.mm \
		$(SRC)/Photino.Mac.WindowDelegate.mm \
		$(SRC)/Photino.Mac.NavigationDelegate.mm \
		$(SRC)/Photino.Mac.UrlSchemeHandler.mm \
		$(SRC)/Photino.Mac.NSWindowBorderless.mm \
		$(SRC)/Photino.Mac.Dialog.mm \
		$(SRC)/Photino.Mac.mm \
		$(SRC)/Exports.mm && \
	rm $(SRC)/Exports.mm

install-linux-dependencies:
	sudo apt-get update && \
	sudo apt-get install -y libgtk-3-dev libwebkit2gtk-4.1-dev libnotify4 libnotify-dev

# Linux x64 .so
build-photino-linux-x64: | $(DEST_PATH_X64)
	$(CXX) $(CXXFLAGS) $(SOFLAGS) $(LDFLAGS) \
		-o $(DEST_PATH_X64)/$(DEST_FILE).so \
		$(SRC)/Photino.Linux.Dialog.cpp \
		$(SRC)/Photino.Linux.cpp \
		$(SRC)/Exports.cpp \
		`pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libnotify`

# Linux arm64 .so (native on arm64 host; for cross, override CXX/PKG_CONFIG_PATH)
build-photino-linux-arm64: | $(DEST_PATH_ARM64)
	$(CXX) $(CXXFLAGS) $(SOFLAGS) $(LDFLAGS) \
		-o $(DEST_PATH_ARM64)/$(DEST_FILE).so \
		$(SRC)/Photino.Linux.Dialog.cpp \
		$(SRC)/Photino.Linux.cpp \
		$(SRC)/Exports.cpp \
		`pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libnotify`

# Ensure output directories exist before link steps
$(DEST_PATH_X64):
	mkdir -p $@

$(DEST_PATH_ARM64):
	mkdir -p $@

clean-x64:
	rm -rf $(DEST_PATH_X64)/* ; mkdir -p $(DEST_PATH_X64)

clean-arm64:
	rm -rf $(DEST_PATH_ARM64)/* ; mkdir -p $(DEST_PATH_ARM64)
