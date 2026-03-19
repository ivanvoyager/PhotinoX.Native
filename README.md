# PhotinoX.Native

[![NuGet Version](https://img.shields.io/nuget/v/PhotinoX.Native.svg)](https://www.nuget.org/packages/PhotinoX.Native)
[![Build (Windows)](https://github.com/ivanvoyager/PhotinoX.Native/actions/workflows/photinox-native-win.yml/badge.svg)](https://github.com/ivanvoyager/PhotinoX.Native/actions/workflows/photinox-native-win.yml)
[![Build (Unix)](https://github.com/ivanvoyager/PhotinoX.Native/actions/workflows/build-native-unix.yml/badge.svg)](https://github.com/ivanvoyager/PhotinoX.Native/actions/workflows/build-native-unix.yml)
[![License](https://img.shields.io/github/license/ivanvoyager/PhotinoX.Native?label=license)](https://github.com/ivanvoyager/PhotinoX.Native/blob/master/LICENSE)
[![NuGet Downloads](https://img.shields.io/nuget/dt/PhotinoX.Native.svg)](https://www.nuget.org/packages/PhotinoX.Native)

**PhotinoX.Native** is an independent fork of [`tryphotino/photino.Native`](https://github.com/tryphotino/photino.Native) licensed under **Apache‑2.0**.  
This project is **not affiliated** with the original Photino organization.

The goal of this fork is to maintain and improve the native cross‑platform binaries for:
- **Windows x64 / ARM64**
- **Linux x64 / ARM64**
- **macOS x64 / ARM64 (Universal)**

PhotinoX.Native provides a lightweight native window host using the OS’s built‑in WebView stack:

- **Windows:** WebView2
- **macOS:** WKWebView
- **Linux:** WebKitGTK 4.1

## Runtime support (RID matrix)

Native binaries included in this package:

| OS      | Architecture | RID              | Files                                       |
|---------|--------------|------------------|---------------------------------------------|
| Windows | x64          | `win-x64`        | `PhotinoX.Native.dll`, `WebView2Loader.dll` |
| Windows | ARM64        | `win-arm64`      | `PhotinoX.Native.dll`, `WebView2Loader.dll` |
| Linux   | x64          | `linux-x64`      | `PhotinoX.Native.so`                        |
| Linux   | ARM64        | `linux-arm64`    | `PhotinoX.Native.so`                        |
| macOS   | x64          | `osx-x64`        | `PhotinoX.Native.dylib` (universal)         |
| macOS   | ARM64        | `osx-arm64`      | `PhotinoX.Native.dylib` (universal)         |

All files follow the standard NuGet `runtimes/<rid>/native/` layout.

This package is intended for developers building modern desktop apps with  
web‑based UI frameworks (Blazor, React, Vue, Angular, etc.) on top of  
native OS windows with minimal dependencies.

> If you are looking for the main project, see:  
> https://github.com/ivanvoyager/PhotinoX

### Photino (upstream) vs PhotinoX (fork)

| Aspect | Photino (upstream) | PhotinoX (fork) |
|---|---|---|
| **Concept / architecture** | Lightweight alternative to Electron: native window + system WebViews (Windows: WebView2, macOS: WKWebView, Linux: WebKitGTK). | Same architecture; fork focuses on keeping native binaries and packaging up‑to‑date. |
| **Native layer** | `Photino.Native` is the C++/Obj‑C++ wrapper around the OS web view. | Same layer and API surface; maintained builds and fixes in the fork. |
| **Linux dependency (WebKitGTK)** | Migrated to WebKitGTK 4.1 in early 2025 (makefile updated before the 4.0.22 release). | Uses WebKitGTK 4.1 consistently across CI/scripts. |
| **Docs vs. reality (Linux)** | Public Photino.Native docs still mention `libwebkit2gtk-4.0-dev` and Azure Pipelines; the page wasn’t updated after the switch in code. | README/notes match current toolchains and 4.1 (no Azure Pipelines references). |
| **Release activity** | Latest public upstream release: 4.0.22 (Jan 23, 2025). | Fork publishes its own PhotinoX.Native package with current artifacts. |
| **RID packaging** | Uses standard `runtimes/<rid>/native/` layout in NuGet packages. | Same standard RID layout; emphasis on keeping all target RIDs green in CI (win‑x64/arm64, linux‑x64/arm64, osx‑x64/arm64). |
| **Positioning** | Uses system web engines instead of bundling Chromium; smaller footprint vs. Electron. | Same positioning; focus on maintainable builds and predictable packaging cadence. |

### History (concise, factual)

**Photino (upstream).** Designed as a lightweight alternative to Electron: a native window hosts the OS’s built‑in web view (WebView2 on Windows, WKWebView on macOS, WebKitGTK on Linux), reducing both application size and memory footprint compared to Chromium‑bundled approaches.

**Native core.** The cross‑platform native layer is `Photino.Native` (C++/Objective‑C++), which wraps the system web view and exposes a minimal interface for higher‑level runtimes.

**Linux dependency update.** In January 2025 upstream switched Linux builds from WebKitGTK 4.0 to 4.1 and shortly released 4.0.22. The public documentation was not updated and still references `libwebkit2gtk-4.0-dev`

**Current state.** Upstream’s last public `Photino.Native` release is dated January 23, 2025 (v4.0.22). PhotinoX continues the idea with maintained native binaries and consistent RID packaging for Windows x64/ARM64, Linux x64/ARM64, and macOS x64/ARM64.

---

# Building (Windows / Linux / macOS)

The build system for PhotinoX.Native uses a combination of MSBuild (Windows)  
and the included `makefile` (Linux, macOS).

> **Toolset note:** The project targets **MSVC v145** (Visual Studio 2026).  
> The examples below show how to build with **v143** for Visual Studio 2022.  
> CI also uses **v143** for compatibility with hosted runners.

## Windows

Requirements:
- **Visual Studio 2026** (includes support for **MSVC Toolset v145**)
- Workload: **Desktop Development with C++**
- WebView2 Runtime (required by the Windows backend)
- Build configurations:
  - `Release | x64`
  - `Release | ARM64`

To build manually with MSVC Toolset v143:

```powershell
msbuild .\Photino.Native\Photino.Native.vcxproj ^
  /p:Configuration=Release ^
  /p:Platform=x64 ^
  /p:PlatformToolset=v143
```
or for ARM64:
```powershell
msbuild .\Photino.Native\Photino.Native.vcxproj ^
  /p:Configuration=Release ^
  /p:Platform=ARM64 ^
  /p:PlatformToolset=v143
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

## Contributing

Issues and PRs are welcome. Keep changes minimal and performance-conscious.

## License

PhotinoX.Native is licensed under **Apache‑2.0**.  