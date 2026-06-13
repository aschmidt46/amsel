# Antons Multi-System-Emulator (AMSEL)

Cross-Plattform Multi-System-Emulator in C++ (NES, DMG, CGB)


<img src="resources/kristall.png" width="48%">   <img src="resources/zelda.png" width="48%">

## Systeme und Kompatibilität
### Nintendo Entertainment System (nur NTSC)
- Taktgeschwindigkeiten und Verhalten ist an NTSC abgestimmt
- Die häufigst verwendeten Mapper sind implementiert (MMC1, UxROM, CNROM, (MMC3), AxROM)
- MMC3 implementierung ist unvollständig mit stark vereinfachtem IRQ Timing, manche Spiele laufen wenn überhaupt nur mit visuellen Glitches
### Nintendo Gameboy Color (unterstützt auch originale Gameboy-Spiele)
- DMG Spiele laufen nur in schwarz-weiß
- Die meisten Mapper sind implementiert (MBC1, MBC2, MBC3, MBC5)
- Die Echtzeituhr von entsprechenden Cartridges ist aktuell noch nicht implementiert
- Einige wenige Spiele funktionieren aufgrund von obskuren DMA Timings nicht korrekt
### Nintendo Gameboy Advance (in Arbeit)
- TODO

## Features
- Persistente Einstellungen (Lautstärke, Tastenbelegungen, Sprache, etc.)
- Persistente Speicherstände
- Visueller Debugger / Disassembler
- Lokaler Mehrspieler (NES)
- Gamepad-Unterstützung
- CRT-Shader
- Lokalisierung über JSON-Dateien (standardmäßig deutsch, englisch)
- React-Weboberfläche, ermöglicht Nutzung auf Smartphones

## Aufbau
Das Projekt ist zum Großteil in C++ geschrieben. Die einzelnen Konsolen sind größtenteils von der Benutzeroberfläche entkoppelt, dadurch lässt sich das Programm auch zumindest theoretisch leicht erweitern, allerdings gehen einige Teile der Benutzeroberfläche noch strikt von einem NES / Gameboy aus (z.B. Tastenbelegungen).
Die Benutzeroberfläche ist aktuell in Dear ImGui implementiert, das ganze Desktop-Frontend läuft in einem OpenGL-Kontext.

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
