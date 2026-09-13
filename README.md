# StabileoViewer (C++20 / OpenGL 3.3 Core / C# Scripting)

**StabileoViewer** est un moteur graphique 3D haute performance et un logiciel d'analyse par éléments finis (FEA) développé en **C++20**, **OpenGL 3.3 Core**, **Dear ImGui (Docking)**, **Eigen 3.4** et **.NET 6 CoreCLR (Scripting C#)**.

Conçu pour le calcul, le dimensionnement réglementaire et la visualisation interactive des structures de génie civil et mécanique (bâtiments, tours, ponts à haubans/suspendus, dômes, treillis spatiaux, portiques industriels).

---

## 🚀 Fonctionnalités Principales

### 1. Solveur Éléments Finis 3D (Direct Stiffness Method)
- Formulation matricielle rigoureuse 3D Navier-Bernoulli / Timoshenko (6 DDL par nœud : $u_x, u_y, u_z, \theta_x, \theta_y, \theta_z$).
- Prise en charge des charges réparties ($w$) et ponctuelles ($F, M$).
- Calcul analytique exact des efforts internes aux stations ($N, V_y, V_z, M_y, M_z, T$) et des contraintes de von Mises $\sigma / f_y$.
- Calcul des réactions d'appui et vérification de l'équilibre statique global $\sum \mathbf{R} + \sum \mathbf{F} = 0$.
- Validation analytique intégrée (`--test`) avec une précision exacte ($3.34 \times 10^{-6} \%$ d'erreur relative sur la flèche $5wL^4/(384EI)$).

### 2. Moteur de Scripting C# & Extension UI (Architecture Hazel Engine)
- **Hébergement .NET CoreCLR** via `hostfxr.dll` avec table d'appels natifs optimisée (`[UnmanagedCallersOnly]`).
- **Compilation dynamique à chaud (Hot-Reload)** avec le compilateur Roslyn `csc.exe` de Visual Studio 2026.
- **API C# complète (`StabileoAPI.cs`)** :
  - `Stabileo.UI` : Fenêtres, boutons, sliders, barres de progression, séparateurs, arborescences.
  - `Stabileo.Model` : Création et manipulation des nœuds, barres, appuis et chargements.
  - `Stabileo.Solver` : Exécution du solveur EF et extraction des extrema ($f_{\max}, N_{\max}, M_{\max}$).
- **Plugins d'ingénierie inclus** :
  - **Eurocode 3 (NF EN 1993-1-1)** : Vérification de profilés acier (traction, compression, courbes européennes de flambement $\chi$, flexion et interaction).
  - **Générateur Paramétrique de Treillis** : Topologies Warren, Pratt, Howe avec maillage et calcul automatique en direct.

### 3. Interface Visual Studio 2026 sans Superposition
- **Viewport 3D plein écran direct** (`PassthruCentralNode`) avec navigation fluide (orbite, pan, zoom, clic de sélection d'objets).
- **Boutons caméra d'accès rapide** intégrés directement dans la barre de menus principale (`Face`, `Plan`, `Côté`, `Iso`, `Cadrer`, `Persp/Ortho`).
- **Cube de navigation 3D interactif (ViewCube)** inspiré d'Autodesk Robot Structural Analysis.
- **Organisation ergonomique étanche** :
  - *Gauche (22%)* : Explorateur de modèle, Calques d'affichage, Catalogues JSON & DXF.
  - *Droite (26%)* : Inspecteur de propriétés, Plans de coupe 3D, Résultats EF & Plugins C#.
  - *Bas (24%)* : Journal de calcul EF, Tables de données (Nœuds, Barres, Réactions), Console C# Scripting.
  - *Centre* : 100% dégagé pour le rendu graphique 3D.

### 4. Importateur AutoCAD DXF
- Analyseur syntaxique DXF ASCII autonome (`LINE`, `3DLINE`, `LWPOLYLINE`, `POLYLINE`, `POINT`).
- **Node Snapping / Welding** : Fusion géométrique automatique des nœuds voisins dans une tolérance réglable.
- **Mappage automatique de calques** : Profilés 3D réels (HEA, IPE, UPN, tubes).

---

## 🛠️ Bibliothèques Locales (`libs/`) & 100% Hors-Ligne

Toutes les dépendances tierces sont incluses localement dans le dossier `libs/` à la racine :
- `libs/glfw/` (GLFW 3.4)
- `libs/glm/` (GLM 1.0.1)
- `libs/glew/` (GLEW 2.2.0)
- `libs/imgui/` (Dear ImGui Docking)
- `libs/json/` (nlohmann/json 3.11.3)
- `libs/eigen/` (Eigen 3.4.0)

> **Avantages** : Aucun téléchargement internet nécessaire. La configuration CMake prend moins d'une seconde (`0.3s`).

---

## 📦 Compilation & Lancement Rapide

### 1. En un clic (Fichiers Batch)
- Double-cliquez sur `build_and_run.bat` pour compiler et lancer en Release.
- Double-cliquez sur `generate_vs2026.bat` pour générer la solution Visual Studio 2026 (`StabileoViewer.sln`).

### 2. En ligne de commande (PowerShell / CMD)
```powershell
# Configuration instantanée (0.3s)
cmake -B build -G "Visual Studio 18 2026" -A x64

# Compilation Release multi-cœurs
cmake --build build --config Release --parallel

# Lancement
.\build\bin\Release\StabileoViewer.exe
```

### 3. Validation Analytique CLI (Mode Test)
```powershell
.\build\bin\Release\StabileoViewer.exe --test
```

---

## ⌨️ Raccourcis Clavier & Contrôles

- **Clic Gauche + Glisser** : Rotation orbitale de la caméra 3D
- **Clic Droit / Milieu + Glisser** : Panoramique (Pan)
- **Molette Souris** : Zoom centré sur le curseur
- **Clic Gauche net** : Sélection interactive de nœud ou barre
- **Touche 1** : Vue de Face (XZ)
- **Touche 2** : Vue de Dessus (Plan XY)
- **Touche 3** : Vue de Côté (YZ)
- **Touche 4** : Vue Isométrique
- **Touche 5** : Bascule Perspective / Orthographique
- **Touche F** : Cadrer l'ensemble de la structure
- **Touche Échap** : Désélectionner
