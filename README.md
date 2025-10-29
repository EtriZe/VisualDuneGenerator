# 🌵 Dune Viewer — Procedural Dune Visualizer (C++ / SDL2 / ImGui / OpenGL)

[🇫🇷 Read this in French](README.fr.md)

**Dune Viewer** is an interactive tool for visualizing **procedurally generated sand dunes** using **Perlin / FBM / Ridged noise** with **anisotropy** and **domain warp**.  
All parameters can be modified in real time through **Dear ImGui**, allowing instant visual feedback in 3D.

---

## 🧭 Main Features
- Generates dunes from **Perlin noise** and **Fractal Brownian Motion (FBM)**
- Real-time adjustable parameters (octaves, gain, lacunarity, amplitude, etc.)
- **Anisotropy** to simulate wind direction
- **Domain warp** for natural turbulence
- **Ridged mode** for sharp dune crests
- Free 3D view (rotation, zoom, pan)
- **Wireframe** or **solid** rendering
- Compatible with **Linux** and **Windows**

---

## 🎮 Controls
| Action | Effect |
|--------|---------|
| **Right-click + drag** | Rotate the camera |
| **Mouse wheel** | Zoom in / out |
| **ImGui interface** | Adjust all parameters in real time |

---

## ⚙️ Dependencies
- **SDL2** — window and input handling  
- **OpenGL** + **GLU** — 3D rendering  
- **GLEW** — OpenGL extension loader  
- **Dear ImGui** — user interface  

All libraries are open-source and available through standard package managers.

---

## 💻 Installation & Launch

### 🔹 On Linux
#### Prerequisites:
```bash
sudo apt install build-essential cmake libsdl2-dev libglew-dev libglu1-mesa-dev
```

#### Build:
```bash
git clone https://github.com/<your-username>/dune-viewer.git
cd dune-viewer
mkdir build && cd build
cmake ..
make -j
```

#### Run:
```bash
./dune_viewer
```

#### Result:
A window titled **"Dune Studio — Advanced Viewer"** opens with:
- 3D dunes rendered in real time
- ImGui controls on the right for interactive tuning

---

### 🔹 On Windows

#### Option 1 — **MSYS2 + MinGW64**
1. Install MSYS2: [https://www.msys2.org/](https://www.msys2.org/)
2. Open **MSYS2 MinGW64** and run:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2 mingw-w64-x86_64-glew
   ```
3. Clone the project:
   ```bash
   git clone https://github.com/<your-username>/dune-viewer.git
   cd dune-viewer
   mkdir build && cd build
   cmake -G "MinGW Makefiles" ..
   mingw32-make
   ```
4. Launch:
   ```bash
   ./dune_viewer.exe
   ```

#### Option 2 — **Visual Studio + vcpkg**
1. Install [Visual Studio Community](https://visualstudio.microsoft.com/)
2. Install [vcpkg](https://github.com/microsoft/vcpkg) then run:
   ```bash
   vcpkg install sdl2 glew
   ```
3. Open the project folder in Visual Studio (it will detect `CMakeLists.txt`)
4. Build and run `dune_viewer`

---

## 🏗️ Project Structure
```
dune-viewer/
├── dune_viewer.cpp       ← main source file
├── imgui/                ← Dear ImGui (as folder or submodule)
│   ├── imgui.cpp, backends/, etc.
├── CMakeLists.txt
├── LICENSE
├── README.md
└── .gitignore
```

---

## 🧠 Technical Details
The terrain is a fixed grid (e.g. 128×128 vertices).  
Each vertex height `z` is computed as:

```
z = amplitude * f(x, y)
```

Where `f(x, y)` is a **fractal noise function (FBM)** with:
- **Rotation** → wind direction  
- **Stretching** → anisotropy along X/Y  
- **Domain warp** → noise distortion  
- **RidgedFBM** → accentuates dune crests  

Vertices are rendered using OpenGL, either in **wireframe** or **filled** mode.

---

## 📜 Licenses
| Component | License |
|------------|----------|
| **Your code (Dune Viewer)** | MIT |
| **Dear ImGui** | MIT (© Omar Cornut) |
| **SDL2** | zlib |
| **GLEW** | BSD / MIT-like |
| **GLU** | depends on the system distribution (not redistributed here) |

---

## ✨ Quick Start Examples

**Linux:**
```bash
sudo apt install build-essential cmake libsdl2-dev libglew-dev libglu1-mesa-dev
git clone https://github.com/<your-username>/dune-viewer.git
cd dune-viewer && mkdir build && cd build
cmake .. && make -j && ./dune_viewer
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2 mingw-w64-x86_64-glew
git clone https://github.com/<your-username>/dune-viewer.git
cd dune-viewer && mkdir build && cd build
cmake -G "MinGW Makefiles" .. && mingw32-make && ./dune_viewer.exe
```

---

## 👨‍💻 Author
Developed by [Your Name or Alias].  
Based on **Dear ImGui (MIT)** and **SDL2 (zlib)**.
