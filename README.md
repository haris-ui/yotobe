# Yotobe

A lightweight, dedicated desktop client for YouTube built with modern C++20, Qt 6 (Qt WebEngine / Chromium core), and CMake.

Yotobe provides a distraction-free YouTube viewing experience with network-level ad and tracking filtering, cosmetic element cleaning, multi-tab video browsing, full Google sign-in compatibility, and an integrated video downloader.

Developed by **Muhammad Haris Zubair**.

---

## Key Features

- **Multi-Tab Browsing**: Open multiple videos or channels simultaneously in tabs with persistent shared login sessions across tabs.
- **Native Network-Level Ad Filtering**: Sub-millisecond URL interception and blocking of ad servers, telemetry, and tracking endpoints before requests touch the network.
- **Cosmetic DOM Cleaning**: Automated suppression of promotional banners, companion ad panels, and feed promotions without causing grid shifts or layout gaps.
- **Full Google Sign-In Support**: Native handling of Google authentication and OAuth redirects with browser fingerprint preservation and client hint protection.
- **Integrated Video Downloader**: Queue video downloads in various resolutions (1080p, 720p, audio-only MP3) powered by `yt-dlp`.
- **Keyboard-First Controls**: Efficient desktop hotkeys for tab management, navigation, and fullscreen playback.
- **Dark & Light Themes**: Styled with a YouTube-inspired dark aesthetic and unified 32px control heights.
- **Privacy Controls**: Single-click cache and cookie management to wipe local browsing data.

---

## Architecture Overview

```
                      Yotobe Application
                             │
       ┌─────────────────────┼─────────────────────┐
       ▼                     ▼                     ▼
   UI Shell               Browser               Engine
  (Tabs & Toolbar)     (WebEngine / Tabs)     (Filters & DL)
       │                     │                     │
       └─────────────────────┼─────────────────────┘
                             │
                             ▼
                          YouTube
```

### Module Structure
- `src/ui/`: `MainWindow`, `SplashScreen`, `SettingsDialog`, `DownloadDialog`
- `src/browser/`: `BrowserView` (Chromium web engine wrapper), `NavigationManager`, `UrlPolicy`
- `src/filtering/`: `RuleParser`, `RuleMatcher`, `UrlFilterInterceptor`, `FilterManager`, `FilterStatistics`
- `src/cosmetic/`: `CosmeticFilterManager` (DOM script injection)
- `src/downloader/`: `VideoDownloader`, `DownloadItem` (yt-dlp integration)
- `src/settings/`: `SettingsManager` (persistent configuration)
- `resources/`: Embedded SVG icons, app icon, default filter rules, and scripts

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl + T` | Open new YouTube tab |
| `Ctrl + W` | Close current tab |
| `Ctrl + Tab` | Next tab |
| `Ctrl + Shift + Tab` | Previous tab |
| `Ctrl + 1` to `9` | Jump directly to tab 1 through 9 |
| `Ctrl + L` | Focus search / URL address bar |
| `Alt + Left` | Navigate back |
| `Alt + Right` | Navigate forward |
| `F5` / `Ctrl + R` | Reload active page |
| `F11` | Toggle fullscreen mode |
| `Escape` | Exit fullscreen mode |

---

## Building from Source

### Prerequisites
1. **Windows 10 / 11 (64-bit)**
2. **Visual Studio 2022** (Build Tools or Community Edition with "Desktop development with C++" workload)
3. **Qt 6.8+ (MSVC 2022 64-bit)** with `qtwebengine` and `qtpositioning` modules installed (e.g., `C:\Qt\6.8.2\msvc2022_64`)
4. **CMake** (3.20 or newer)
5. **Ninja** build system

### Compilation Steps

1. Open PowerShell and initialize MSVC 64-bit developer tools:
   ```cmd
   call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
   ```

2. Clone the repository:
   ```bash
   git clone https://github.com/haris-ui/yotobe.git
   cd yotobe
   ```

3. Configure with CMake:
   ```powershell
   cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH="C:/Qt/6.8.2/msvc2022_64" -DCMAKE_BUILD_TYPE=Release
   ```

4. Build the executable:
   ```powershell
   cmake --build build --config Release
   ```

5. Run unit tests:
   ```powershell
   .\build\TestRuleMatcher.exe
   ```

6. Launch Yotobe:
   ```powershell
   .\build\Yotobe.exe
   ```

---

## Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository on GitHub.
2. Create a feature branch:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. Commit your changes with clear, descriptive commit messages:
   ```bash
   git commit -m "Add feature description"
   ```
4. Push to your branch:
   ```bash
   git push origin feature/your-feature-name
   ```
5. Open a Pull Request on GitHub.

### Guidelines
- Adhere to modern C++20 conventions.
- Maintain existing architecture patterns (separate filtering, navigation, and UI concerns).
- Keep UI styling consistent with the YouTube dark palette.

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
