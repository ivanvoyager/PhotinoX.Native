[![PhotinoX Logo](https://raw.githubusercontent.com/ivanvoyager/PhotinoX/refs/heads/master/assets/photinox-logo.png)](https://github.com/ivanvoyager/PhotinoX)

# PhotinoX.Native

[![NuGet Version](https://img.shields.io/nuget/v/PhotinoX.Native.svg)](https://www.nuget.org/packages/PhotinoX.Native)
[![Build (Windows)](https://github.com/ivanvoyager/PhotinoX.Native/actions/workflows/photinox-native-win.yml/badge.svg)](https://github.com/ivanvoyager/PhotinoX.Native/actions/workflows/photinox-native-win.yml)
[![Build (Unix)](https://github.com/ivanvoyager/PhotinoX.Native/actions/workflows/build-native-unix.yml/badge.svg)](https://github.com/ivanvoyager/PhotinoX.Native/actions/workflows/build-native-unix.yml)
[![License](https://img.shields.io/github/license/ivanvoyager/PhotinoX.Native?label=license)](https://github.com/ivanvoyager/PhotinoX.Native/blob/master/LICENSE)
[![NuGet Downloads](https://img.shields.io/nuget/dt/PhotinoX.Native.svg)](https://www.nuget.org/packages/PhotinoX.Native)

**PhotinoX.Native** is an independent fork of [`tryphotino/photino.Native`](https://github.com/tryphotino/photino.Native) licensed under **Apache‑2.0**.  
This project is **not affiliated** with the original Photino organization.

The goal of this fork is to maintain and improve the native cross-platform layer for:
- **Windows x64 / ARM64**
- **Linux x64 / ARM64**
- **macOS x64 / ARM64 (Universal)**

PhotinoX.Native provides a lightweight native window host using the OS’s built-in WebView stack, while improving native API structure, interop layout, application lifetime, message-loop ownership, and unified native window state tracking.

- **Windows:** WebView2 Runtime  
  Required component: **Microsoft.Web.WebView2** (Edge WebView2)  
  https://learn.microsoft.com/microsoft-edge/webview2/
- **macOS:** WKWebView (system WebKit)  
  https://developer.apple.com/documentation/webkit/wkwebview/
- **Linux:** WebKitGTK 4.1 (runtime + dev packages)  
  https://webkitgtk.org/

## Runtime support (RID matrix)

Binaries included in this package:

| OS      | Architecture | RID              | Files                                       |
|---------|--------------|------------------|---------------------------------------------|
| Windows | x64          | `win-x64`        | `PhotinoX.Native.dll`, `WebView2Loader.dll` |
| Windows | ARM64        | `win-arm64`      | `PhotinoX.Native.dll`, `WebView2Loader.dll` |
| Linux   | x64          | `linux-x64`      | `PhotinoX.Native.so`                        |
| Linux   | ARM64        | `linux-arm64`    | `PhotinoX.Native.so`                        |
| macOS   | x64          | `osx-x64`        | `PhotinoX.Native.dylib` (universal)         |
| macOS   | ARM64        | `osx-arm64`      | `PhotinoX.Native.dylib` (universal)         |

All files follow the standard NuGet `runtimes/<rid>/native/` layout.

## Core (ecosystem)

These repositories provide the managed .NET surface around the native binaries:

- [**PhotinoX**](https://github.com/ivanvoyager/PhotinoX) - .NET wrapper around the native layer.
- [**PhotinoX.Blazor**](https://github.com/ivanvoyager/PhotinoX.Blazor) - Blazor integration for native desktop apps.
- [**PhotinoX.Server**](https://github.com/ivanvoyager/PhotinoX.Server) - optional local static-file server for SPA/static assets.
- [**PhotinoX.Samples**](https://github.com/ivanvoyager/PhotinoX.Samples) - sample projects showcasing common scenarios.

This package is intended for developers building modern desktop apps with web‑based UI frameworks (Blazor, React, Vue, Angular, etc.) on top of native OS windows with minimal dependencies.

> If you are looking for the main project, see:  
> https://github.com/ivanvoyager/PhotinoX

### Photino (upstream) vs PhotinoX (fork)

| Aspect | Photino (upstream) | PhotinoX (fork) |
|---|---|---|
| **Concept / architecture** | Lightweight alternative to Electron: native window + system WebViews (Windows: WebView2, macOS: WKWebView, Linux: WebKitGTK). | Same lightweight architecture, with active work on native stability, packaging, interop, and long-running application scenarios. |
| **Native layer structure** | Central `Photino` class mixes public API, callbacks, platform headers, platform state, and platform-specific implementation details. | Keeps the same native foundation but separates callbacks, init parameters, options, strings, monitors, exports, and platform state into focused components, reducing cross-platform coupling. |
| **Platform isolation** | Platform-specific headers, fields, and methods are mixed directly into the main `Photino` declaration. | Uses platform state objects (`WindowsState`, `LinuxState`, `MacState`) and platform-specific implementation sections to keep Windows, Linux, and macOS concerns isolated. |
| **Native memory ownership** | Uses pointer-based string conversion and implicit ownership across several native interop paths, including custom-scheme/resource responses. | Uses explicit native allocation/free helpers, owned `PlatformString` values, and non-owning `Utf8String` ABI inputs. String conversion and response buffers crossing the managed/native boundary now have predictable native ownership to reduce leak-prone paths in long-running applications. |
| **Application / message loop model** | Uses global native wait/invoke helpers around the native window/message loop. | Introduces `PhotinoApplication` as the native application lifetime object, with explicit run/shutdown semantics, check-access support, and application-level synchronous/asynchronous dispatch. |
| **Application dispatch API** | Uses older global invoke/wait helpers without a consistent state-carrying callback ABI. | Provides application-level `Invoke` and `BeginInvoke` entry points using a state-aware callback shape (`callback + state`). This gives wrappers a predictable synchronous/asynchronous dispatch contract and allows explicit state passing without captured callbacks in dispatcher-facing paths. |
| **Window lifecycle callbacks** | Provides the original window callback set. | Extends native lifecycle/state callbacks with closed, fullscreen, unified state-change, and WebView content-loaded notifications for more complete and predictable native window and WebView state reporting. |
| **Window state model** | Window state handling is spread across platform-specific callbacks and legacy minimized/maximized/fullscreen paths, which can produce transient or duplicate restored notifications. | Uses a unified native-driven `PhotinoWindowState` model (`Normal`, `Minimized`, `Maximized`, `FullScreen`) with `StateChanged` as the primary transition event. Legacy state callbacks are derived from actual state transitions, avoiding misleading `Restored` notifications from transient resize/native messages. |
| **Startup window state** | Startup minimized/maximized/fullscreen handling is platform-specific and tied to older separate state flags. | Startup state is normalized through the unified `PhotinoWindowState` model and synchronized without user state callbacks during native window construction where the platform allows it. |
| **Windows fullscreen handling** | Fullscreen behavior follows the older platform-specific path. | Windows fullscreen is restore-aware: the previous style and placement are saved before entering fullscreen and restored when leaving fullscreen, with fullscreen transitions integrated into the unified state machine. |
| **Linux dependency (WebKitGTK)** | Migrated to WebKitGTK 4.1 in early 2025 (makefile updated before the 4.0.22 release). | Uses WebKitGTK 4.1 consistently across CI/scripts. |
| **Docs vs. reality (Linux)** | Public Photino.Native docs still mention `libwebkit2gtk-4.0-dev` and Azure Pipelines; the page wasn’t updated after the switch in code. | README/notes match current toolchains and 4.1 (no Azure Pipelines references). |
| **Release activity** | Latest public upstream release: 4.0.22 (Jan 23, 2025). | Fork publishes its own PhotinoX.Native package with current artifacts. |
| **RID packaging** | Uses standard `runtimes/<rid>/native/` layout in NuGet packages. | Same standard RID layout; emphasis on keeping all target RIDs green in CI (win‑x64/arm64, linux‑x64/arm64, osx‑x64/arm64). |

### History

Photino succeeded Steve Sanderson’s experimental [WebWindow](https://github.com/SteveSandersonMS/WebWindow) project, which explored native OS windows hosting web UI for .NET applications on Windows, macOS, and Linux. Photino continued this idea as an Electron-inspired open-source .NET project backed by the CODE Magazine team and the open-source community, while using the OS-provided WebView stack instead of bundling Chromium.

Upstream’s last public `Photino.Native` release is dated January 23, 2025 (v4.0.22). PhotinoX.Native continues the native-window model with maintained binaries, consistent RID packaging, improved native memory ownership, clearer platform isolation, Windows WebView2 integration fixes, unified native window state tracking, WebView content-loaded callbacks, state-aware application dispatch APIs, and an application-oriented native message-loop model.

---

# Building (Windows / Linux / macOS)

The build system for PhotinoX.Native uses a combination of MSBuild (Windows)  
and the included `makefile` (Linux, macOS).

> **Toolset note:** The project targets **MSVC v145** (Visual Studio 2026).  
> CI also uses **v145**.

> CI: see  
> - [`.github/workflows/photinox-native-win.yml`](https://github.com/ivanvoyager/PhotinoX.Native/blob/master/.github/workflows/photinox-native-win.yml) (build + pack + upload `win-x64`/`win-ARM64`)  
> - [`.github/workflows/build-native-unix.yml`](https://github.com/ivanvoyager/PhotinoX.Native/blob/master/.github/workflows/build-native-unix.yml) (build + pack + upload `linux-x64`/`linux-arm64` and `osx-x64`/`osx-arm64`)

## Windows

Requirements:
- **Visual Studio 2026** (includes support for **MSVC Toolset v145**)
- Workload: **Desktop Development with C++**
- WebView2 Runtime (required by the Windows backend)
- Build configurations:
  - `Release | x64`
  - `Release | ARM64`

To build manually with MSVC Toolset v145:

```powershell
msbuild .\Photino.Native\Photino.Native.vcxproj ^
  /p:Configuration=Release ^
  /p:Platform=x64 ^
  /p:PlatformToolset=v145
```
or for ARM64:
```powershell
msbuild .\Photino.Native\Photino.Native.vcxproj ^
  /p:Configuration=Release ^
  /p:Platform=ARM64 ^
  /p:PlatformToolset=v145
```

## Linux

```sh
sudo apt-get update
sudo apt-get install \
    build-essential \
    libgtk-3-dev \
    libwebkit2gtk-4.1-dev \
    libnotify-dev
make linux-x64       # or make linux-arm64
```

## macOS

Requirements:
- Xcode 15+ (macOS 14/15/16 toolchains)
- clang with Objective‑C++ support
- WKWebView available system-wide

```sh
make mac-universal
```

## Known limitations

- **Linux Wayland:** top-level window position is compositor-controlled. Move notifications and position restore are best-effort; state and size tracking remain supported.
- **Linux chromeless drag/resize:** native chromeless drag, resize, and titlebar double-click handling use GTK event-driven hit testing over configured drag and resize regions. The generic `BeginWindowDrag` / `BeginWindowResize` entry points remain no-ops on Linux because GTK/Wayland require the original trusted native pointer event.

## Contributing

Issues and PRs are welcome. Keep changes minimal and performance-conscious.

## License

PhotinoX.Native is licensed under **Apache‑2.0**.  