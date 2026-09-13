# StabileoViewer (C++20 / OpenGL 3.3 Core)

**StabileoViewer** est un moteur graphique 3D haute performance et un logiciel d'analyse par éléments finis (FEA) développé en **C++20**, **OpenGL 3.3 Core**, **Dear ImGui (Docking)** et **Eigen 3.4**.

Conçu pour le calcul et la visualisation interactive des structures de génie civil et mécanique (bâtiments, tours, ponts à haubans/suspendus, dômes, treillis spatiaux, portiques industriels).

---

## 🚀 Fonctionnalités Principales

### 1. Solveur Éléments Finis 3D (Direct Stiffness Method)
- Formulation matricielle rigoureuse 3D Navier-Bernoulli / Timoshenko (6 DDL par nœud : $u_x, u_y, u_z, \theta_x, \theta_y, \theta_z$).
- Prise en charge des charges réparties ($w$) et ponctuelles ($F, M$).
- Calcul analytique exact des efforts internes aux stations ($N, V_y, V_z, M_y, M_z, T$) et des contraintes de von Mises $\sigma / f_y$.
- Calcul des réactions d'appui et vérification de l'équilibre statique global $\sum \mathbf{R} + \sum \mathbf{F} = 0$.
- Validation analytique intégrée (`--test`) avec une précision exacte ($3.34 \times 10^{-6} \%$ d'erreur relative sur flèche $5wL^4/(384EI)$).

### 2. Interface Utilisateur Moderne avec Dear ImGui Docking
- **Système de Docking 100% Modulaire** : chaque onglet est une fenêtre déplaçable, scindable, empilable ou détachable librement.
- **10 Fenêtres / Onglets Indépendants** :
  - *Affichage & Calques* : Nœuds, barres filaires, profilés extrudés 3D, appuis, charges, grille, axes locaux.
  - *Déformée 3D* : Amplification logarithmique ($1\times$ à $2000\times$) et animation oscillatoire dynamique.
  - *Diagrammes d'Efforts* : Normal ($N$), tranchants ($V_y, V_z$), moments fléchissants ($M_y, M_z$), torsion ($T$).
  - *Carte des Contraintes (Heatmap)* : Palettes perceptuelles Google Turbo et Viridis selon $\sigma / f_y$.
  - *Inspecteur & Propriétés* : Propriétés détaillées du nœud ou de la barre sélectionnée, jauge de contrainte, catalogue des sections.
  - *Modèles C++ Phares* : 15 structures complètes (Bâtiments 3D, Tour Diagrid, Ponts suspendus/haubanés, Dôme, Pylône HT, etc.).
  - *Catalogue JSON Stabileo* : 59 structures issues de l'écosystème Stabileo.
  - *Importateur DXF* : Importation de dessins AutoCAD avec fusion de nœuds.
  - *Tables de Données* : Déplacements nodaux en mm, efforts extrêmes, réactions et équilibre global.
- **Navigation 3D Fluide** : Viewport central transparent (`PassthruCentralNode`) permettant l'interaction caméra directe (orbite, zoom, panoramique).

### 3. Importateur de Fichiers AutoCAD DXF
- Analyseur syntaxique DXF ASCII autonome (`LINE`, `3DLINE`, `LWPOLYLINE`, `POLYLINE`, `POINT`).
- **Node Snapping / Welding** : Fusion géométrique automatique des nœuds voisins dans une tolérance $\varepsilon$ (ex: 5 mm).
- **Mappage de Calques** : Attribution automatique des profilés réels 3D selon le calque (`HEA`, `IPE`, `TUBE`, `SHS`, etc.).
- Détection automatique des appuis au sol et ajout de charges d'essai.

---

## 🛠️ Prérequis & Dépendances

- **Compilateur C++20** : MSVC (Visual Studio 2022 ou 2026), GCC 11+ ou Clang 13+.
- **CMake** : Version 3.20 ou supérieure.
- **Dépendances gérées automatiquement via CMake FetchContent** :
  - GLFW 3.4 (Fenêtrage et contexte OpenGL)
  - GLEW 2.2.0 (Extensions OpenGL)
  - GLM 1.0.1 (Algèbre linéaire graphique)
  - Dear ImGui (`docking` branch)
  - nlohmann/json 3.11.3 (Parsing JSON)
  - Eigen 3.4.0 (Solveur matriciel creux / dense)

---

## 📦 Compilation & Exécution

### Avec CMake (Ligne de commande)

```powershell
# Configuration
cmake -B build

# Compilation en configuration Release
cmake --build build --config Release --parallel

# Lancement de l'application
.\build\bin\Release\StabileoViewer.exe
```

### Validation Analytique CLI (Mode Test)

```powershell
.\build\bin\Release\StabileoViewer.exe --test
```

---

## ⌨️ Raccourcis Clavier & Contrôles

- **Clic Gauche + Glisser** : Rotation orbitale de la caméra 3D
- **Clic Droit + Glisser** : Déplacement panoramique (Pan)
- **Molette Souris** : Zoom avant / arrière
- **Touche 1** : Vue de Face
- **Touche 2** : Vue de Dessus
- **Touche 3** : Vue de Côté
- **Touche 4** : Vue Isométrique
- **Touche F** : Cadrer l'ensemble de la structure
- **Touche Échap** : Quitter l'application
