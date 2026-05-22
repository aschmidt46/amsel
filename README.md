# Antons Multi-System-Emulator-Library (AMSEL)

Cross-Plattform Multi-System-Emulator in C++ (NES, DMG, CGB)


<img src="resources/kristall.png" width="48%">   <img src="resources/zelda.png" width="48%">

# Unterstützte Systeme und Aufbau

## Systeme
-   Nintendo Entertainment System (nur NTSC)
-   Nintendo Gameboy Color (unterstützt auch originale Gameboy-Spiele)

## Features
- Persistente Einstellungen (Lautstärke, Tastenbelegungen, Sprache, etc.)
- Persistente Speicherstände
- Visueller Debugger / Disassembler
- Lokaler Mehrspieler (NES)
- Gamepad-Unterstützung
- CRT-Shader
- Lokalisierung über JSON-Dateien (standardmäßig deutsch, englisch)
- React-Web-Oberfläche, ermöglicht Nutzung auf Smartphones

## Aufbau
Das Projekt ist zum Großteil in C++ geschrieben. Die einzelnen Konsolen sind größtenteils von der Benutzeroberfläche entkoppelt, dadurch lässt sich das Programm auch zumindest theoretisch leicht erweitern, allerdings gehen einige Teile der Benutzeroberfläche noch strikt von einem NES / Gameboy aus (z.B. Tastenbelegungen).
Die Benutzeroberfläche ist aktuell in Dear ImGui implementiert, das ganze Frontend läuft in einem OpenGL-Kontext. In der Zukunft könnte man noch ein Webfrontend in WebAssembly hinzufügen.

Den Gameboy-Emulator hatte ich zuerst in einem eigenständigen Projekt in Rust implementiert, um die Sprache zu lernen. Anschließend habe ich in dem Projekt ein Foreign Function Interface zu C++ mit Rust CXX gebaut, um es in das NES-Projekt zu integrieren. Mit Corrosion lässt sich ein Rust-Paket mit CXX sehr leicht in ein vorhandenes CMake-Skript eingliedern.

Für die Web-App definiert der Emulator eine Javascript-Schnittstelle und wird mit Emscripten kompiliert.

# Build

Erfordert CMake.
Erfordert Python 3 für das Bauen von glad.
Erfordert Rust bzw. Cargo für den Gameboy-Teil.
Vor dem Bauen müssen die git-Submodule geladen werden.

Bauen:
```
cd build
python -m venv .venv
.venv/Scripts/activate.<ps1/bat/sh...>
python -m pip install jinja2
cmake ..
make
```
## Bauen der Web-App

Erfordert Emscripten und npm.

```
emcmake cmake .. -DBUILD_WEB=ON
make
```
Dann in source/web/amsel-web:
```
npm install
npm run dev         (startet Entwicklungsserver)
npm run build       (Baut Webseite für Auslieferung)
```
