// =============================================================================
//  ParametricTrussGenerator.cs — Plugin C# pour StabileoViewer
//
//  Générateur paramétrique de treillis métalliques en temps réel :
//  - Topologies : Warren, Pratt, Howe
//  - Contrôle dynamique de portée, hauteur, nombre de travées
//  - Application automatique des conditions aux limites et charges nodales
//  - Résolution directe par le solveur EF C++ et affichage des efforts normaux
// =============================================================================

using System;

namespace Stabileo.Plugins
{
    [StabileoPlugin("Générateur Paramétrique de Treillis", "Générateurs", "Génération dynamique de fermes et treillis avec calcul EF automatique", "Christinot")]
    public class ParametricTrussGenerator : IStabileoPlugin
    {
        private float m_SpanLength = 12.0f; // [m]
        private float m_TrussHeight = 2.0f; // [m]
        private int m_BayCount = 6;         // Nombre de travées
        private int m_Topology = 0;         // 0: Warren, 1: Pratt, 2: Howe
        private float m_NodalLoadKn = 25.0f;// [kN] Charge descendante par nœud supérieur
        private bool m_AutoSolve = true;

        public void OnLoad()
        {
            Log.Info("[TrussGenerator] Plugin de génération paramétrique chargé.");
        }

        public void OnUnload()
        {
            Log.Info("[TrussGenerator] Plugin de génération paramétrique déchargé.");
        }

        public void OnUIRender()
        {
            if (!UI.Begin("Générateur Paramétrique de Treillis (C#)"))
            {
                UI.End();
                return;
            }

            UI.TextColored("=== Générateur de Treillis Paramétrique ===", 0.2f, 0.9f, 0.6f);
            UI.Text("Création géométrique C# et résolution directe par le moteur C++.");
            UI.Separator();

            UI.SliderFloat("Portée totale L (m)", ref m_SpanLength, 4.0f, 40.0f);
            UI.SliderFloat("Hauteur H (m)", ref m_TrussHeight, 0.5f, 6.0f);
            UI.SliderInt("Nombre de travées (N)", ref m_BayCount, 2, 16);

            UI.Spacing();
            UI.Text("Topologie du treillis :");
            if (UI.Button(m_Topology == 0 ? "[ Warren ]" : "Warren", 100, 24)) m_Topology = 0;
            UI.SameLine();
            if (UI.Button(m_Topology == 1 ? "[ Pratt ]" : "Pratt", 100, 24)) m_Topology = 1;
            UI.SameLine();
            if (UI.Button(m_Topology == 2 ? "[ Howe ]" : "Howe", 100, 24)) m_Topology = 2;

            UI.Spacing();
            UI.SliderFloat("Charge par nœud supérieur (kN)", ref m_NodalLoadKn, 0.0f, 150.0f);
            UI.Checkbox("Résoudre automatiquement le calcul EF", ref m_AutoSolve);

            UI.Separator();
            if (UI.Button("Générer et Calculer EF (C# -> C++)", 300, 32))
            {
                GenerateTruss();
            }

            if (Solver.HasResults)
            {
                UI.Separator();
                UI.TextColored("--- Résultats EF du Treillis ---", 0.3f, 1.0f, 0.5f);
                float maxDispMm = Solver.MaxDisplacement * 1000.0f;
                float maxN = Solver.MaxNormalForce;

                UI.Text($"Flèche maximale f_max : {maxDispMm:F2} mm");
                UI.Text($"Effort normal max |N| : {maxN:F1} kN");
                if (m_SpanLength > 0.1f)
                {
                    float limitDispMm = (m_SpanLength * 1000.0f) / 300.0f; // L/300
                    UI.Text($"Limite admissible (L/300) : {limitDispMm:F2} mm");
                    float dispRatio = limitDispMm > 0 ? (Math.Abs(maxDispMm) / limitDispMm) : 0;
                    UI.ProgressBar(Math.Min(1.0f, dispRatio), $"{dispRatio * 100.0f:F1}%");
                }
            }

            UI.End();
        }

        private void GenerateTruss()
        {
            Log.Info($"[TrussGenerator] Génération d'un treillis : L={m_SpanLength}m, H={m_TrussHeight}m, {m_BayCount} travées.");

            Model.ClearAll();

            int n = m_BayCount;
            float dx = m_SpanLength / n;
            float x0 = -m_SpanLength * 0.5f;

            int[] bottomNodes = new int[n + 1];
            int[] topNodes = new int[n + 1];

            // 1. Création des nœuds
            for (int i = 0; i <= n; ++i)
            {
                float x = x0 + i * dx;
                bottomNodes[i] = Model.AddNode(x, 0.0f, 0.0f);
                topNodes[i] = Model.AddNode(x, m_TrussHeight, 0.0f);
            }

            // 2. Barres de membrure inférieure
            for (int i = 0; i < n; ++i)
            {
                Model.AddElement(bottomNodes[i], bottomNodes[i + 1], "IPE 300", true);
            }

            // 3. Barres de membrure supérieure
            for (int i = 0; i < n; ++i)
            {
                Model.AddElement(topNodes[i], topNodes[i + 1], "IPE 300", true);
            }

            // 4. Montants verticaux
            for (int i = 0; i <= n; ++i)
            {
                Model.AddElement(bottomNodes[i], topNodes[i], "IPE 300", true);
            }

            // 5. Diagonales selon la topologie
            for (int i = 0; i < n; ++i)
            {
                if (m_Topology == 0) // Warren
                {
                    if (i % 2 == 0)
                        Model.AddElement(bottomNodes[i], topNodes[i + 1], "IPE 300", true);
                    else
                        Model.AddElement(topNodes[i], bottomNodes[i + 1], "IPE 300", true);
                }
                else if (m_Topology == 1) // Pratt : diagonales tendues descendant vers le centre
                {
                    if (i < n / 2)
                        Model.AddElement(bottomNodes[i], topNodes[i + 1], "IPE 300", true);
                    else
                        Model.AddElement(topNodes[i], bottomNodes[i + 1], "IPE 300", true);
                }
                else // Howe : diagonales montant vers le centre
                {
                    if (i < n / 2)
                        Model.AddElement(topNodes[i], bottomNodes[i + 1], "IPE 300", true);
                    else
                        Model.AddElement(bottomNodes[i], topNodes[i + 1], "IPE 300", true);
                }
            }

            // 6. Appuis : rotule à gauche, rouleau à droite
            Model.AddSupport(bottomNodes[0], Model.SupportType.Pinned);
            Model.AddSupport(bottomNodes[n], Model.SupportType.RollerX);

            // 7. Charges descendantes sur la membrure supérieure
            if (m_NodalLoadKn > 0.01f)
            {
                for (int i = 0; i <= n; ++i)
                {
                    float factor = (i == 0 || i == n) ? 0.5f : 1.0f;
                    Model.AddNodalLoad(topNodes[i], 0.0f, -m_NodalLoadKn * factor, 0.0f);
                }
            }

            Model.RequestRebuild();

            if (m_AutoSolve)
            {
                bool ok = Solver.Solve();
                if (ok)
                {
                    Log.Info("[TrussGenerator] Modèle résolu avec succès !");
                }
                else
                {
                    Log.Error("[TrussGenerator] Échec de la résolution du système linéaire.");
                }
            }
        }
    }
}
