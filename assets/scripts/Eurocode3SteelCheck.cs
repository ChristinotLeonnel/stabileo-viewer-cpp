// =============================================================================
//  Eurocode3SteelCheck.cs — Plugin C# pour StabileoViewer
//
//  Vérification de profilés métalliques selon l'Eurocode 3 (NF EN 1993-1-1) :
//  - Résistance de section en traction / compression pure
//  - Flambement par flexion (courbes européennes a, b, c, d)
//  - Moment résistant plastique de flexion My,Ed / Mpl,Rd
//  - Interaction flexion-compression axiale
// =============================================================================

using System;

namespace Stabileo.Plugins
{
    [StabileoPlugin("Eurocode 3 - Vérification Acier", "Eurocodes", "Vérification réglementaire EN 1993-1-1 avec courbes de flambement", "Christinot")]
    public class Eurocode3SteelCheck : IStabileoPlugin
    {
        // Données d'entrée générales
        private float m_SteelGradeFy = 235.0f; // S235 [MPa]
        private float m_YoungModulus = 210000.0f; // [MPa]
        private float m_GammaM0 = 1.00f;
        private float m_GammaM1 = 1.00f;

        // Géométrie de la section (IPE 300 par défaut)
        private float m_AreaCm2 = 53.8f;      // [cm²]
        private float m_WplyCm3 = 628.0f;     // [cm³]
        private float m_RadiusOfGyrationY = 12.5f; // iy [cm]

        // Paramètres de l'élément
        private float m_MemberLengthM = 6.0f; // [m]
        private float m_BucklingFactor = 1.0f; // bi-articulé par défaut

        // Sollicitations de calcul (ELU)
        private float m_Ned = 250.0f;   // [kN] (positif = compression)
        private float m_MyEd = 45.0f;   // [kN·m]
        private bool m_IsTension = false;

        public void OnLoad()
        {
            Log.Info("[Eurocode3] Module de vérification Eurocode 3 chargé avec succès.");
        }

        public void OnUnload()
        {
            Log.Info("[Eurocode3] Déchargement du module.");
        }

        public void OnUIRender()
        {
            if (!UI.Begin("Eurocode 3 — Vérification Acier (C# Plugin)"))
            {
                UI.End();
                return;
            }

            UI.TextColored("=== Vérification aux États-Limites Ultimes (ELU) ===", 0.3f, 0.8f, 1.0f);
            UI.Text("Norme européenne NF EN 1993-1-1");
            UI.Separator();

            // Bouton de synchronisation directe avec le solveur EF C++
            if (Solver.HasResults)
            {
                if (UI.Button("Importer les efforts max du calcul EF", 320, 26))
                {
                    float maxN = Solver.MaxNormalForce;
                    float maxM = Solver.MaxBendingMoment;

                    if (maxN < 0)
                    {
                        m_Ned = Math.Abs(maxN);
                        m_IsTension = false; // compression
                    }
                    else
                    {
                        m_Ned = maxN;
                        m_IsTension = true;
                    }
                    m_MyEd = Math.Abs(maxM);
                    Log.Info($"[Eurocode3] Importé depuis le solveur : Ned={m_Ned:F1} kN, MyEd={m_MyEd:F1} kNm");
                }
            }
            else
            {
                UI.TextColored("Solveur non calculé : valeurs manuelles actives.", 0.7f, 0.7f, 0.7f);
            }

            UI.Spacing();

            // Section 1 : Matériau & Section
            if (UI.CollapsingHeader("1. Propriétés Section et Matériau", true))
            {
                UI.SliderFloat("Limite élastique fy (MPa)", ref m_SteelGradeFy, 235.0f, 460.0f);
                UI.SliderFloat("Aire A (cm²)", ref m_AreaCm2, 5.0f, 300.0f);
                UI.SliderFloat("Module plastique Wpl,y (cm³)", ref m_WplyCm3, 20.0f, 4000.0f);
                UI.SliderFloat("Rayon de giration iy (cm)", ref m_RadiusOfGyrationY, 1.0f, 30.0f);
            }

            // Section 2 : Conditions aux limites & Longueur
            if (UI.CollapsingHeader("2. Longueur de flambement & Instabilité", true))
            {
                UI.SliderFloat("Longueur barre L (m)", ref m_MemberLengthM, 0.5f, 25.0f);
                UI.SliderFloat("Coeff. de longueur k (Lcr = k*L)", ref m_BucklingFactor, 0.5f, 2.5f);
                UI.Checkbox("Effort en traction (sans flambement)", ref m_IsTension);
            }

            // Section 3 : Sollicitations
            if (UI.CollapsingHeader("3. Sollicitations ELU appliquées", true))
            {
                UI.SliderFloat("Effort normal |Ned| (kN)", ref m_Ned, 0.0f, 2500.0f);
                UI.SliderFloat("Moment fléchissant |My,Ed| (kN·m)", ref m_MyEd, 0.0f, 600.0f);
            }

            UI.Separator();
            UI.TextColored("--- Résultats des ratios d'utilisation ---", 1.0f, 0.8f, 0.2f);

            // ================= CALCULS EUROCODE 3 =================
            float areaM2 = m_AreaCm2 * 1e-4f;
            float wplM3 = m_WplyCm3 * 1e-6f;
            float iyM = m_RadiusOfGyrationY * 1e-2f;
            float lcr = m_MemberLengthM * m_BucklingFactor;

            // 1. Résistance en traction / compression de section pure
            float nPlRd = (areaM2 * m_SteelGradeFy * 1e3f) / m_GammaM0; // [kN]
            float ratioN = nPlRd > 1e-3f ? (m_Ned / nPlRd) : 0.0f;

            // 2. Résistance en flexion pure
            float mPlRd = (wplM3 * m_SteelGradeFy * 1e3f) / m_GammaM0; // [kN·m]
            float ratioM = mPlRd > 1e-3f ? (m_MyEd / mPlRd) : 0.0f;

            // 3. Flambement (si compression)
            float lambda1 = (float)(Math.PI * Math.Sqrt(m_YoungModulus / m_SteelGradeFy));
            float slenderness = (iyM > 1e-4f) ? (lcr / iyM) : 1.0f;
            float lambdaBar = slenderness / lambda1;

            // Courbe b (alpha = 0.34 pour profilés I laminés h/b > 1.2)
            float alpha = 0.34f;
            float phi = 0.5f * (1.0f + alpha * (lambdaBar - 0.2f) + lambdaBar * lambdaBar);
            float chi = 1.0f;
            if (lambdaBar > 0.2f)
            {
                float radical = (float)Math.Sqrt(Math.Max(0.0f, phi * phi - lambdaBar * lambdaBar));
                chi = 1.0f / (phi + radical);
                if (chi > 1.0f) chi = 1.0f;
            }

            float nbRd = (chi * areaM2 * m_SteelGradeFy * 1e3f) / m_GammaM1; // [kN]
            float ratioBuckling = nbRd > 1e-3f ? (m_Ned / nbRd) : 0.0f;

            // 4. Interaction globale
            float ratioTotal = 0.0f;
            if (m_IsTension)
            {
                ratioTotal = ratioN + ratioM;
            }
            else
            {
                // Formule approchée d'interaction EC3 simplification
                float kyy = 1.0f;
                ratioTotal = (nbRd > 1e-3f ? (m_Ned / nbRd) : 0.0f) + (mPlRd > 1e-3f ? (kyy * m_MyEd / mPlRd) : 0.0f);
            }

            // Affichage des barres de progression ImGui avec seuils
            UI.Text($"Résistance normale de section : {m_Ned:F1} / {nPlRd:F1} kN");
            UI.ProgressBar(Math.Min(1.0f, ratioN), $"{ratioN * 100.0f:F1}%");

            if (!m_IsTension)
            {
                UI.Text($"Résistance au flambement Nb,Rd (chi={chi:F3}) : {m_Ned:F1} / {nbRd:F1} kN");
                UI.ProgressBar(Math.Min(1.0f, ratioBuckling), $"{ratioBuckling * 100.0f:F1}%");
            }

            UI.Text($"Résistance en flexion My,Ed / Mpl,Rd : {m_MyEd:F1} / {mPlRd:F1} kN·m");
            UI.ProgressBar(Math.Min(1.0f, ratioM), $"{ratioM * 100.0f:F1}%");

            UI.Spacing();
            UI.Text("Ratio combiné ultime Eurocode 3 :");
            if (ratioTotal <= 0.90f)
            {
                UI.TextColored($"CONFORME (Taux : {ratioTotal * 100.0f:F1}%)", 0.2f, 1.0f, 0.3f);
            }
            else if (ratioTotal <= 1.00f)
            {
                UI.TextColored($"PROCHE LIMITE (Taux : {ratioTotal * 100.0f:F1}%)", 1.0f, 0.8f, 0.2f);
            }
            else
            {
                UI.TextColored($"NON CONFORME - DÉPASSEMENT ELU (Taux : {ratioTotal * 100.0f:F1}%)", 1.0f, 0.2f, 0.2f);
            }
            UI.ProgressBar(Math.Min(1.5f, ratioTotal) / 1.5f, $"{ratioTotal * 100.0f:F1}%");

            UI.End();
        }
    }
}
