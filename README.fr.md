# 🌵 Dune Viewer — Visualiseur de dunes procédurales (C++ / SDL2 / ImGui / OpenGL)

**Dune Viewer** est un outil interactif permettant de visualiser des dunes de sable générées procéduralement à partir de **bruits de Perlin / FBM / Ridged** avec **anisotropie** et **domain warp**.  
Le programme permet de modifier tous les paramètres en temps réel via **Dear ImGui**, et d’observer instantanément le résultat en 3D.

Exemple : 
<img width="1910" height="1028" alt="image" src="https://github.com/user-attachments/assets/4513c7d3-6199-4524-ba02-8943eea7dda3" />


---

## 🧭 Fonctionnalités principales
- Génération de dunes à partir de **Perlin noise** et de **Fractal Brownian Motion (FBM)**
- Paramètres ajustables en direct (octaves, gain, lacunarity, amplitude, etc.)
- **Anisotropie** pour simuler la direction du vent
- **Domain warp** pour les turbulences du vent
- Mode **Ridged** pour des crêtes plus nettes
- Vue 3D libre (rotation, zoom, translation)
- Rendu **wireframe** ou **plein**
- Compatible **Linux** et **Windows**

---

## 🎮 Contrôles
| Action | Résultat |
|--------|-----------|
| **Clic droit + déplacer la souris** | Rotation de la caméra |
| **Molette de la souris** | Zoom avant / arrière |
| **Interface ImGui** | Ajuste en temps réel tous les paramètres |

---

## ⚙️ Dépendances
- **SDL2** — pour la fenêtre et les événements  
- **OpenGL** + **GLU** — pour le rendu 3D  
- **GLEW** — pour l’accès aux extensions OpenGL  
- **Dear ImGui** — pour l’interface graphique  

Toutes les bibliothèques sont open source et disponibles via les gestionnaires de paquets.

---

## 💻 Installation et lancement

**Linux:**
```bash
sudo apt install build-essential cmake libsdl2-dev libglew-dev libglu1-mesa-dev
git clone https://github.com/EtriZe/VisualDuneGenerator.git
cd VisualDuneGenerator
git clone https://github.com/ocornut/imgui.git
mkdir build && cd build
cmake .. && make -j && ./dune_viewer
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2 mingw-w64-x86_64-glew
git clone https://github.com/EtriZe/VisualDuneGenerator.git
cd VisualDuneGenerator
git clone https://github.com/ocornut/imgui.git
mkdir build && cd build
cmake -G "MinGW Makefiles" .. && mingw32-make && ./dune_viewer.exe
```

---

#### Résultat :
Une fenêtre “**Dune Studio — Advanced Viewer**” s’ouvre, avec :
- Le rendu 3D des dunes au centre
- Les contrôles ImGui à droite pour modifier les paramètres

---

### 🔹 Sous Windows

#### Option 1 — Avec **MSYS2 + MinGW64**
1. Installer MSYS2 : [https://www.msys2.org/](https://www.msys2.org/)
2. Ouvrir **MSYS2 MinGW64** et exécuter :
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2 mingw-w64-x86_64-glew
   ```
3. Cloner le projet :
   ```bash
   git clone https://github.com/EtriZe/VisualDuneGenerator.git
   cd dune-viewer
   mkdir build && cd build
   cmake -G "MinGW Makefiles" ..
   mingw32-make
   ```
4. Lancer :
   ```bash
   ./dune_viewer.exe
   ```

#### Option 2 — Avec **Visual Studio + vcpkg**
1. Installer [Visual Studio Community](https://visualstudio.microsoft.com/fr/vs/)
2. Installer [vcpkg](https://github.com/microsoft/vcpkg) puis :
   ```bash
   vcpkg install sdl2 glew
   ```
3. Ouvrir le dossier du projet dans Visual Studio (CMakeLists.txt est détecté automatiquement)
4. Lancer la **build** puis exécuter `dune_viewer`

---

## 🏗️ Structure du projet
```
VisualDuneGenerator/
├── dune_viewer.cpp       ← code principal
├── imgui/                ← dossier Dear ImGui (ou submodule)
│   ├── imgui.cpp, backends/, etc.
├── CMakeLists.txt
├── LICENSE
├── README.md
└── .gitignore
```

---

## 🧠 Détails techniques
Le terrain est un plan quadrillé (ex: 128×128 vertices).  
Chaque hauteur `z` est calculée par :

```
z = amplitude * f(x, y)
```

où `f(x, y)` est un **bruit fractal (FBM)** avec :
- **Rotation** : direction du vent  
- **Stretch** : anisotropie sur X/Y  
- **Warp** : distorsion du domaine  
- **RidgedFBM** : accentuation des crêtes  

Les vertices sont ensuite rendus sous OpenGL, en mode **wireframe** ou **filled** selon la configuration.

---

## 📜 Licences
| Composant | Licence |
|------------|----------|
| **Mon Code** | MIT |
| **Dear ImGui** | MIT (© Omar Cornut) |
| **SDL2** | zlib |
| **GLEW** | BSD / MIT-like |
| **GLU** | dépend de la distribution (non redistribué ici) |

---

## 🧑‍💻 Auteur
Développé par EtriZe.  
Basé sur **Dear ImGui (MIT)** et **SDL2 (zlib)**.
