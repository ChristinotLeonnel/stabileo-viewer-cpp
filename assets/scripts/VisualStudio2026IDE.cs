// =============================================================================
//  VisualStudio2026IDE.cs — Modèle C# Haute-Fidélité de Visual Studio 2026
//
//  Reproduit point par point la capture d'écran de Visual Studio 2026 :
//   - Barre supérieure : Ruban violet VS [⯌], menus et recherche globale
//   - Barre de débogage : Cibles x64-Debug, bouton vert [▶ StabileoViewer.exe]
//   - Éditeur de code à onglets : SupportGizmos.cpp avec numéros de lignes et syntaxe C++
//   - Panneau de Sortie : Logs réels de génération CMake Ninja & Liste d'erreurs
//   - Explorateur de solutions : Arborescence de dossiers, recherche et onglet Git
//   - Barre d'état basse : Prêt, statut Git main, encodage et zoom
// =============================================================================

using System;
using System.Collections.Generic;

namespace Stabileo
{
    [StabileoPlugin("Visual Studio 2026 IDE", "Environnement", "Réplique ergonomique intégrale de Visual Studio 2026 en C#")]
    public class VisualStudio2026IDE : IStabileoPlugin
    {
        // Onglet actif dans l'éditeur de code
        private int m_SelectedDocTab = 0;
        private readonly List<string> m_OpenTabs = new List<string>
        {
            "SupportGizmos.cpp",
            "UIManager.h",
            "CMakeLists.txt",
            "Nouveautés"
        };

        // Filtre de recherche dans l'Explorateur de solutions
        private string m_SearchFilter = "";
        private string m_GlobalSearch = "stabileo-viewer-cpp";

        // Navigation dans le panneau inférieur (0 = Sortie, 1 = Liste d'erreurs)
        private int m_BottomPanelTab = 0;
        private int m_RightPanelTab = 0; // 0 = Explorateur, 1 = Git

        // Statut du solveur EF déclenché par le bouton [▶ StabileoViewer.exe]
        private string m_BuildStatus = "Prêt";
        private bool m_IsSimulating = false;

        // Fichiers du projet
        private readonly Dictionary<string, string[]> m_ProjectFiles = new Dictionary<string, string[]>
        {
            { "scripting", new string[] { "ScriptEngine.cpp", "ScriptEngine.h" } },
            { "solver", new string[] { "LinearStaticSolver.cpp", "LinearStaticSolver.h" } },
            { "structural", new string[] { "ModelDatabase.cpp", "ModelDatabase.h", "BuildingGenerator.cpp", "BuildingGenerator.h" } },
            { "tools", new string[] { "SnapEngine.cpp", "SnapEngine.h", "DrawingTools.cpp", "DrawingTools.h" } },
            { "ui", new string[] {
                "ContentBrowserPanel.cpp", "ContentBrowserPanel.h",
                "HazelUI.cpp", "HazelUI.h",
                "IconManager.cpp", "IconManager.h",
                "SceneHierarchyPanel.cpp", "SceneHierarchyPanel.h",
                "StructuralModelPanel.cpp", "StructuralModelPanel.h",
                "UIManager.cpp", "UIManager.h",
                "ViewCube.cpp", "ViewCube.h"
            } }
        };

        private readonly string[] m_RootFiles = new string[]
        {
            "main.cpp",
            ".gitignore",
            "build_and_run.bat",
            "CMakeLists.txt",
            "generate_vs2026.bat",
            "README.md"
        };

        public void OnLoad()
        {
            Log.Info("[VS2026] Plugin Visual Studio 2026 chargé avec succès.");
        }

        public void OnUnload()
        {
            Log.Info("[VS2026] Plugin déchargé.");
        }

        public void OnUIRender()
        {
            // Appliquer le thème sombre officiel Visual Studio 2026
            UI.PushStyleColor(ImGuiCol.WindowBg, VSColors.DarkBg.r, VSColors.DarkBg.g, VSColors.DarkBg.b, 1.0f);
            UI.PushStyleColor(ImGuiCol.TitleBg, VSColors.MenuBg.r, VSColors.MenuBg.g, VSColors.MenuBg.b, 1.0f);
            UI.PushStyleColor(ImGuiCol.TitleBgActive, VSColors.MenuBg.r, VSColors.MenuBg.g, VSColors.MenuBg.b, 1.0f);
            UI.PushStyleColor(ImGuiCol.FrameBg, 0.15f, 0.15f, 0.16f, 1.0f);
            UI.PushStyleColor(ImGuiCol.Button, 0.20f, 0.20f, 0.22f, 0.8f);
            UI.PushStyleColor(ImGuiCol.ButtonHovered, 0.28f, 0.28f, 0.32f, 1.0f);
            UI.PushStyleColor(ImGuiCol.Header, 0.22f, 0.22f, 0.25f, 1.0f);
            UI.PushStyleColor(ImGuiCol.HeaderHovered, 0.28f, 0.35f, 0.45f, 1.0f);

            bool windowOpen = UI.Begin("Visual Studio 2026 - stabileo-viewer-cpp###VS2026IDE");
            if (!windowOpen)
            {
                UI.PopStyleColor(8);
                UI.End();
                return;
            }

            // 1. Barre de Titre & Menus Principaux
            DrawTitleAndMenuBar();

            // 2. Barre d'outils Standard / Débogage
            DrawStandardToolbar();

            UI.Separator();

            // 3. Corps principal : Gauche (Éditeur + Console Sortie) / Droite (Explorateur de Solutions)
            float totalWidth = UI.ContentRegionAvailX;
            float totalHeight = UI.ContentRegionAvailY - 26.0f; // Réserve 26px pour la barre d'état basse

            float rightPanelWidth = 320.0f;
            float leftPanelWidth = totalWidth - rightPanelWidth - 10.0f;
            if (leftPanelWidth < 300.0f) leftPanelWidth = 300.0f;

            // ---- Panneau Gauche : Éditeur de Code + Console Sortie ----
            if (UI.BeginChild("VSLeftPanel", leftPanelWidth, totalHeight, false))
            {
                float editorHeight = totalHeight * 0.62f;
                float outputHeight = totalHeight - editorHeight - 8.0f;

                // 3.1 Éditeur de code
                if (UI.BeginChild("VSEditorRegion", leftPanelWidth, editorHeight, true))
                {
                    DrawCodeEditor();
                }
                UI.EndChild();

                UI.Spacing();

                // 3.2 Console Sortie / Liste d'erreurs
                if (UI.BeginChild("VSOutputRegion", leftPanelWidth, outputHeight, true))
                {
                    DrawOutputPanel();
                }
                UI.EndChild();
            }
            UI.EndChild();

            UI.SameLine(0.0f, 6.0f);

            // ---- Panneau Droit : Explorateur de Solutions Visual Studio ----
            if (UI.BeginChild("VSSolutionExplorerPanel", rightPanelWidth, totalHeight, true))
            {
                DrawSolutionExplorer();
            }
            UI.EndChild();

            // 4. Barre d'état basse Visual Studio (Status Bar)
            DrawStatusBar();

            UI.PopStyleColor(8);
            UI.End();
        }

        // =========================================================================
        //  1. Barre de Titre & Menus Principaux
        // =========================================================================
        private void DrawTitleAndMenuBar()
        {
            // Logo violet Visual Studio 2026
            UI.TextColored(" ⯌ ", VSColors.PurpleRibbon.r, VSColors.PurpleRibbon.g, VSColors.PurpleRibbon.b, 1.0f);
            UI.SameLine();

            // Menus classiques
            string[] menus = { "Fichier", "Édition", "Affichage", "Git", "Projet", "Générer", "Déboguer", "Test", "Outils", "Extensions", "Fenêtre", "Aide" };
            for (int i = 0; i < menus.Length; ++i)
            {
                UI.TextColored(menus[i], 0.85f, 0.85f, 0.85f, 1.0f);
                UI.SameLine(0.0f, 8.0f);
            }

            // Boîte de recherche globale centrée style VS 2026
            UI.SameLine(0.0f, 25.0f);
            UI.TextColored("🔍", 0.6f, 0.6f, 0.6f, 1.0f);
            UI.SameLine(0.0f, 4.0f);
            UI.InputText("##GlobalSearch", ref m_GlobalSearch, 64);
        }

        // =========================================================================
        //  2. Barre d'outils Standard / Débogage (Local x64-Debug + Play Button)
        // =========================================================================
        private void DrawStandardToolbar()
        {
            UI.Spacing();

            // Boutons de navigation et historique
            if (UI.Button(" ⮜ ")) { }
            UI.SameLine(0.0f, 3.0f);
            if (UI.Button(" ⮞ ")) { }
            UI.SameLine(0.0f, 10.0f);

            if (UI.Button(" 💾 ")) { }
            UI.SameLine(0.0f, 3.0f);
            if (UI.Button(" 💾💾 ")) { }
            UI.SameLine(0.0f, 10.0f);

            if (UI.Button(" ↩ ")) { }
            UI.SameLine(0.0f, 3.0f);
            if (UI.Button(" ↪ ")) { }
            UI.SameLine(0.0f, 12.0f);

            // Sélecteur cible et configuration
            UI.PushStyleColor(ImGuiCol.Button, 0.18f, 0.18f, 0.20f, 1.0f);
            if (UI.Button(" Ordinateur local ▾ ")) { }
            UI.SameLine(0.0f, 4.0f);
            if (UI.Button(" x64-Debug ▾ ")) { }
            UI.PopStyleColor();
            UI.SameLine(0.0f, 8.0f);

            // BOUTON VERT D'EXÉCUTION PRINCIPAL (Visual Studio Green Play Button)
            UI.PushStyleColor(ImGuiCol.Button,        VSColors.PlayGreen.r, VSColors.PlayGreen.g, VSColors.PlayGreen.b, 0.85f);
            UI.PushStyleColor(ImGuiCol.ButtonHovered, VSColors.PlayGreen.r + 0.1f, VSColors.PlayGreen.g + 0.1f, VSColors.PlayGreen.b + 0.1f, 1.0f);
            UI.PushStyleColor(ImGuiCol.ButtonActive,  VSColors.PlayGreen.r - 0.1f, VSColors.PlayGreen.g - 0.1f, VSColors.PlayGreen.b - 0.1f, 1.0f);
            if (UI.Button(" ▶ StabileoViewer.exe (bin\\Debug\\StabileoViewer.exe) "))
            {
                // Lancement interactif du calcul et compilation
                m_IsSimulating = true;
                m_BuildStatus = "Génération en cours...";
                Solver.Solve();
                Log.Info("[VS2026] Démarrage du débogage de StabileoViewer.exe (x64)");
            }
            UI.PopStyleColor(3);

            UI.SameLine(0.0f, 8.0f);
            if (UI.Button(" ⬇ ")) { }
            UI.SameLine(0.0f, 3.0f);
            if (UI.Button(" ↷ ")) { }
            UI.SameLine(0.0f, 3.0f);
            if (UI.Button(" 🔄 ")) { }
            UI.SameLine(0.0f, 3.0f);
            if (UI.Button(" ⏹ ")) { m_IsSimulating = false; m_BuildStatus = "Prêt"; }

            UI.SameLine(0.0f, 12.0f);
            if (UI.Button(" // ")) { }
            UI.SameLine(0.0f, 3.0f);
            if (UI.Button(" /* */ ")) { }
            UI.SameLine(0.0f, 3.0f);
            if (UI.Button(" 🔖 ")) { }
        }

        // =========================================================================
        //  3. Éditeur de Code Central (SupportGizmos.cpp avec coloration syntaxique)
        // =========================================================================
        private void DrawCodeEditor()
        {
            // Onglets de documents ouverts
            for (int i = 0; i < m_OpenTabs.Count; ++i)
            {
                bool isSelected = (m_SelectedDocTab == i);
                if (isSelected)
                {
                    UI.PushStyleColor(ImGuiCol.Button, 0.12f, 0.12f, 0.12f, 1.0f);
                    UI.PushStyleColor(ImGuiCol.Text,   1.0f, 1.0f, 1.0f, 1.0f);
                }
                else
                {
                    UI.PushStyleColor(ImGuiCol.Button, 0.18f, 0.18f, 0.20f, 0.6f);
                    UI.PushStyleColor(ImGuiCol.Text,   0.70f, 0.70f, 0.70f, 1.0f);
                }

                string tabLabel = $" {m_OpenTabs[i]} ✕ ";
                if (UI.Button(tabLabel))
                {
                    m_SelectedDocTab = i;
                }
                UI.PopStyleColor(2);
                UI.SameLine(0.0f, 2.0f);
            }

            UI.Spacing();

            // Fil d'Ariane (Breadcrumbs)
            UI.PushStyleColor(ImGuiCol.Text, 0.55f, 0.55f, 0.55f, 1.0f);
            UI.Text(" StabileoViewer.exe (bin\\Debug\\StabileoViewer.exe) ▾ > x64-Debug ▾ > { } scene ▾ > SupportGizmos.cpp ▾");
            UI.PopStyleColor();
            UI.Separator();

            // Zone de défilement du code
            float availEditorY = UI.ContentRegionAvailY - 24.0f;
            if (UI.BeginChild("CodeTextRegion", 0.0f, availEditorY, false))
            {
                // Rendu ligne par ligne avec numérotation et coloration syntaxique C++
                DrawHighlightedCppCode();
            }
            UI.EndChild();

            // Gouttière basse de l'éditeur (statut de ligne, encodage)
            UI.PushStyleColor(ImGuiCol.Text, 0.6f, 0.6f, 0.6f, 1.0f);
            UI.Text(" 100 % ▾   |   ");
            UI.SameLine();
            UI.TextColored("✓ Aucun problème détecté", 0.3f, 0.85f, 0.4f, 1.0f);
            UI.SameLine(0.0f, 35.0f);
            UI.Text("Lig. : 10, Col. : 1    SPC    LF    UTF-8");
            UI.PopStyleColor();
        }

        private void DrawHighlightedCppCode()
        {
            // Ligne 1
            DrawLine(1, "// =============================================================================", VSColors.CommentGreen);
            // Ligne 2
            DrawLine(2, "//  SupportGizmos.cpp — Géométrie 3D des appuis structurels", VSColors.CommentGreen);
            // Ligne 3
            DrawLine(3, "// =============================================================================", VSColors.CommentGreen);
            // Ligne 4
            DrawLine(4, "");
            // Ligne 5
            DrawIncludeLine(5, "#include \"scene/SupportGizmos.h\"");
            // Ligne 6
            DrawIncludeLine(6, "#include <glm/gtc/constants.hpp>");
            // Ligne 7
            DrawIncludeLine(7, "#include <cmath>");
            // Ligne 8
            DrawLine(8, "");
            // Ligne 9
            DrawNamespaceLine(9, "namespace scene {");
            // Ligne 10
            DrawLine(10, "");
            // Ligne 11
            DrawLine(11, "    // Génère un mesh combiné pour le gizmo d'un appui", VSColors.CommentGreen);
            // Ligne 12
            DrawFunctionHeaderLine(12, "    Mesh createSupportGizmo(model::SupportType type, float size) {");
            // Ligne 13
            DrawKeywordLine(13, "        switch (type) {");
            // Ligne 14
            DrawLine(14, "");
            // Ligne 15
            DrawLine(15, "            // ---- Encastrement : boîte épaisse hachurée ----", VSColors.CommentGreen);
            // Ligne 16
            DrawCaseLine(16, "            case model::SupportType::FIXED:");
            // Ligne 17
            DrawReturnLine(17, "                return Mesh::createBox(size * 1.6f, size * 0.4f, size * 1.6f);");
            // Ligne 18
            DrawLine(18, "");
            // Ligne 19
            DrawCaseLine(19, "            case model::SupportType::PINNED:");
            // Ligne 20
            DrawReturnLine(20, "                return Mesh::createPyramid(size * 1.2f, size * 1.4f);");
            // Ligne 21
            DrawLine(21, "        }");
            // Ligne 22
            DrawLine(22, "    }");
            // Ligne 23
            DrawLine(23, "} // namespace scene", VSColors.CommentGreen);
        }

        private void DrawLine(int lineNum, string text, (float r, float g, float b, float a)? col = null)
        {
            UI.TextColored($"{lineNum,3}   ", VSColors.LineNumber.r, VSColors.LineNumber.g, VSColors.LineNumber.b, 0.7f);
            UI.SameLine();
            if (col.HasValue)
            {
                UI.TextColored(text, col.Value.r, col.Value.g, col.Value.b, col.Value.a);
            }
            else
            {
                UI.Text(text);
            }
        }

        private void DrawIncludeLine(int lineNum, string text)
        {
            UI.TextColored($"{lineNum,3}   ", VSColors.LineNumber.r, VSColors.LineNumber.g, VSColors.LineNumber.b, 0.7f);
            UI.SameLine();
            UI.TextColored("#include ", VSColors.Preprocessor.r, VSColors.Preprocessor.g, VSColors.Preprocessor.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            string header = text.Replace("#include ", "");
            UI.TextColored(header, VSColors.StringOrange.r, VSColors.StringOrange.g, VSColors.StringOrange.b, 1.0f);
        }

        private void DrawNamespaceLine(int lineNum, string text)
        {
            UI.TextColored($"{lineNum,3}   ", VSColors.LineNumber.r, VSColors.LineNumber.g, VSColors.LineNumber.b, 0.7f);
            UI.SameLine();
            UI.TextColored("namespace ", VSColors.KeywordBlue.r, VSColors.KeywordBlue.g, VSColors.KeywordBlue.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.Text("scene {");
        }

        private void DrawFunctionHeaderLine(int lineNum, string text)
        {
            UI.TextColored($"{lineNum,3}   ", VSColors.LineNumber.r, VSColors.LineNumber.g, VSColors.LineNumber.b, 0.7f);
            UI.SameLine();
            UI.TextColored("    Mesh ", VSColors.TypeTurquoise.r, VSColors.TypeTurquoise.g, VSColors.TypeTurquoise.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.Text("createSupportGizmo(");
            UI.SameLine(0.0f, 0.0f);
            UI.TextColored("model::SupportType ", VSColors.TypeTurquoise.r, VSColors.TypeTurquoise.g, VSColors.TypeTurquoise.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.Text("type, ");
            UI.SameLine(0.0f, 0.0f);
            UI.TextColored("float ", VSColors.KeywordBlue.r, VSColors.KeywordBlue.g, VSColors.KeywordBlue.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.Text("size) {");
        }

        private void DrawKeywordLine(int lineNum, string text)
        {
            UI.TextColored($"{lineNum,3}   ", VSColors.LineNumber.r, VSColors.LineNumber.g, VSColors.LineNumber.b, 0.7f);
            UI.SameLine();
            UI.TextColored("        switch ", VSColors.KeywordBlue.r, VSColors.KeywordBlue.g, VSColors.KeywordBlue.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.Text("(type) {");
        }

        private void DrawCaseLine(int lineNum, string text)
        {
            UI.TextColored($"{lineNum,3}   ", VSColors.LineNumber.r, VSColors.LineNumber.g, VSColors.LineNumber.b, 0.7f);
            UI.SameLine();
            UI.TextColored("            case ", VSColors.KeywordBlue.r, VSColors.KeywordBlue.g, VSColors.KeywordBlue.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.TextColored("model::SupportType", VSColors.TypeTurquoise.r, VSColors.TypeTurquoise.g, VSColors.TypeTurquoise.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.Text("::FIXED:");
        }

        private void DrawReturnLine(int lineNum, string text)
        {
            UI.TextColored($"{lineNum,3}   ", VSColors.LineNumber.r, VSColors.LineNumber.g, VSColors.LineNumber.b, 0.7f);
            UI.SameLine();
            UI.TextColored("                return ", VSColors.KeywordBlue.r, VSColors.KeywordBlue.g, VSColors.KeywordBlue.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.TextColored("Mesh", VSColors.TypeTurquoise.r, VSColors.TypeTurquoise.g, VSColors.TypeTurquoise.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.Text("::createBox(size * ");
            UI.SameLine(0.0f, 0.0f);
            UI.TextColored("1.6f", VSColors.NumberGreen.r, VSColors.NumberGreen.g, VSColors.NumberGreen.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.Text(", size * ");
            UI.SameLine(0.0f, 0.0f);
            UI.TextColored("0.4f", VSColors.NumberGreen.r, VSColors.NumberGreen.g, VSColors.NumberGreen.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.Text(", size * ");
            UI.SameLine(0.0f, 0.0f);
            UI.TextColored("1.6f", VSColors.NumberGreen.r, VSColors.NumberGreen.g, VSColors.NumberGreen.b, 1.0f);
            UI.SameLine(0.0f, 0.0f);
            UI.Text(");");
        }

        // =========================================================================
        //  4. Panneau Inférieur (Sortie CMake & Liste d'erreurs)
        // =========================================================================
        private void DrawOutputPanel()
        {
            // En-tête des onglets
            bool isOutput = (m_BottomPanelTab == 0);
            if (isOutput) UI.PushStyleColor(ImGuiCol.Button, 0.12f, 0.12f, 0.12f, 1.0f);
            if (UI.Button(" Sortie ")) m_BottomPanelTab = 0;
            if (isOutput) UI.PopStyleColor();

            UI.SameLine();
            bool isErrors = (m_BottomPanelTab == 1);
            if (isErrors) UI.PushStyleColor(ImGuiCol.Button, 0.12f, 0.12f, 0.12f, 1.0f);
            if (UI.Button(" Liste d'erreurs (0) ")) m_BottomPanelTab = 1;
            if (isErrors) UI.PopStyleColor();

            UI.Separator();

            if (m_BottomPanelTab == 0)
            {
                // Barre d'outils de la Sortie
                UI.Text("Afficher la sortie à partir de : ");
                UI.SameLine();
                UI.PushStyleColor(ImGuiCol.Button, 0.18f, 0.18f, 0.20f, 1.0f);
                if (UI.Button(" CMake ▾ ")) { }
                UI.PopStyleColor();

                UI.SameLine(0.0f, 12.0f);
                if (UI.Button(" 🗑 ")) { }
                UI.SameLine(0.0f, 4.0f);
                if (UI.Button(" ↩ ")) { }
                UI.SameLine(0.0f, 4.0f);
                if (UI.Button(" ⏱ ")) { }

                UI.Spacing();

                // Console de logs CMake exacte de la capture d'écran
                UI.TextColored("1> [CMake] --- StabileoViewer configured successfully ---", 0.35f, 0.75f, 1.0f, 1.0f);
                UI.Text("1> [CMake]   Generator   : Ninja");
                UI.Text("1> [CMake]   C++ Standard: C++20");
                UI.Text("1> [CMake]   Sources     : E:/Book/Dev/stabileo-viewer-cpp/src");
                UI.Text("1> [CMake]   Output      : E:/Book/Dev/stabileo-viewer-cpp/out/build/x64-Debug/bin");
                UI.Text("1> [CMake] -- Configuring done (3.1s)");
                UI.Text("1> [CMake] -- Generating done (0.1s)");
                UI.Text("1> [CMake] -- Build files have been written to: E:/Book/Dev/stabileo-viewer-cpp/out/build/x64-Debug");
                UI.Text("1> Variables CMake extraites.");
                UI.Text("1> Fichiers sources et en-têtes extraits.");
                UI.Text("1> Modèle de code extrait.");
                UI.Text("1> Extraction effectuée des configurations de chaîne d'outils.");
                UI.Text("1> Chemins include extraits.");
                UI.TextColored("1> Fin de la génération de CMake.", 0.4f, 0.9f, 0.4f, 1.0f);

                if (m_IsSimulating)
                {
                    UI.TextColored("1> [Solveur EF] Exécution DSM 3D réussie : Déplacement max = " + Solver.MaxDisplacement.ToString("F3") + " mm", 0.2f, 1.0f, 0.5f, 1.0f);
                }
            }
            else
            {
                // Onglet Liste d'erreurs
                UI.TextColored("✓ 0 Erreur   |   ! 0 Avertissement   |   ℹ 0 Message", 0.3f, 0.85f, 0.4f, 1.0f);
                UI.Separator();
                UI.TextDisabled("Aucun problème n'a été détecté dans la solution stabileo-viewer-cpp.");
            }
        }

        // =========================================================================
        //  5. Panneau Droit (Explorateur de Solutions - Affichage des dossiers)
        // =========================================================================
        private void DrawSolutionExplorer()
        {
            // Onglets du panneau droit : Explorateur vs Git
            bool isExp = (m_RightPanelTab == 0);
            if (isExp) UI.PushStyleColor(ImGuiCol.Button, 0.12f, 0.12f, 0.12f, 1.0f);
            if (UI.Button(" Explorateur ")) m_RightPanelTab = 0;
            if (isExp) UI.PopStyleColor();

            UI.SameLine();
            bool isGit = (m_RightPanelTab == 1);
            if (isGit) UI.PushStyleColor(ImGuiCol.Button, 0.12f, 0.12f, 0.12f, 1.0f);
            if (UI.Button(" Modifications Git ")) m_RightPanelTab = 1;
            if (isGit) UI.PopStyleColor();

            UI.Separator();

            if (m_RightPanelTab == 0)
            {
                // Titre
                UI.TextColored("Explorateur de solutions", 0.9f, 0.9f, 0.9f, 1.0f);
                UI.TextDisabled("Affichage des dossiers");

                // Boutons d'action miniatures
                if (UI.Button(" 🏠 ")) { }
                UI.SameLine(0.0f, 3.0f);
                if (UI.Button(" 🔄 ")) { }
                UI.SameLine(0.0f, 3.0f);
                if (UI.Button(" − ")) { }
                UI.SameLine(0.0f, 3.0f);
                if (UI.Button(" 📋 ")) { }
                UI.SameLine(0.0f, 3.0f);
                if (UI.Button(" 📁 ")) { }

                UI.Spacing();

                // Boîte de recherche dans l'explorateur
                UI.InputText("##TreeSearch", ref m_SearchFilter, 32);
                UI.Separator();

                // Arborescence de dossiers
                foreach (var folder in m_ProjectFiles)
                {
                    string folderName = folder.Key;
                    if (UI.TreeNode($"📁 {folderName}"))
                    {
                        foreach (var file in folder.Value)
                        {
                            if (!string.IsNullOrEmpty(m_SearchFilter) &&
                                !file.ToLower().Contains(m_SearchFilter.ToLower()))
                            {
                                continue;
                            }

                            string icon = file.EndsWith(".h") || file.EndsWith(".hpp") ? "📄 [h]" : "📄 [cpp]";
                            if (UI.Selectable($"   {icon} {file}"))
                            {
                                if (!m_OpenTabs.Contains(file))
                                {
                                    m_OpenTabs.Add(file);
                                }
                                m_SelectedDocTab = m_OpenTabs.IndexOf(file);
                            }
                        }
                        UI.TreePop();
                    }
                }

                // Fichiers racine
                foreach (var file in m_RootFiles)
                {
                    if (!string.IsNullOrEmpty(m_SearchFilter) &&
                        !file.ToLower().Contains(m_SearchFilter.ToLower()))
                    {
                        continue;
                    }

                    if (UI.Selectable($"📄 {file}"))
                    {
                        if (!m_OpenTabs.Contains(file))
                        {
                            m_OpenTabs.Add(file);
                        }
                        m_SelectedDocTab = m_OpenTabs.IndexOf(file);
                    }
                }
            }
            else
            {
                // Onglet Git
                UI.TextColored("Modifications Git", 0.9f, 0.9f, 0.9f, 1.0f);
                UI.TextDisabled("Branche : main");
                UI.Separator();

                UI.Text("1 modification non indexée :");
                UI.TextColored(" M assets/scripts/VisualStudio2026IDE.cs", 0.9f, 0.8f, 0.3f, 1.0f);

                UI.Spacing();
                UI.PushStyleColor(ImGuiCol.Button, 0.15f, 0.45f, 0.75f, 0.9f);
                if (UI.Button(" Valider tout et pousser (Commit & Push) ", -1.0f, 28.0f))
                {
                    Log.Info("[VS2026 Git] Synchronisation avec GitHub effectuée.");
                }
                UI.PopStyleColor();
            }
        }

        // =========================================================================
        //  6. Barre d'État Basse (Status Bar)
        // =========================================================================
        private void DrawStatusBar()
        {
            UI.Separator();
            UI.TextColored(m_BuildStatus, 0.85f, 0.85f, 0.85f, 1.0f);

            UI.SameLine(UI.ContentRegionAvailX - 320.0f);
            UI.Text("↑ 0 / ↓ 0   |   ⑂ main   |   stabileo-viewer-cpp   |   🔔   ENG 12:10");
        }
    }
}
