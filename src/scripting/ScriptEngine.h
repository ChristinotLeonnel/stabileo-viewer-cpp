#pragma once
// =============================================================================
//  ScriptEngine.h — Sous-système de Scripting C# et d'extension UI (Hazel Engine)
//
//  Intègre le runtime .NET CoreCLR via hostfxr.dll pour exécuter des scripts C#,
//  compiler dynamiquement avec Roslyn (csc.exe) et recharger à chaud (Hot-Reload).
// =============================================================================

#include "scene/StructureModel.h"
#include "solver/LinearSolver.h"
#include <string>
#include <vector>

namespace scripting {

struct ScriptEngineConfig {
    std::string hostfxrDllPath = "C:\\Program Files\\dotnet\\host\\fxr\\6.0.8\\hostfxr.dll";
    std::string roslynCscPath = "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\MSBuild\\Current\\Bin\\Roslyn\\csc.exe";
    std::string dotNetSharedPath = "C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\6.0.8";
    std::string scriptsDir = "assets/scripts";
    std::string assemblyPath = "assets/scripts/bin/StabileoScripts.dll";
    std::string runtimeConfigPath = "assets/scripts/bin/StabileoScripts.runtimeconfig.json";
};

class ScriptEngine {
public:
    /// Initialise le sous-système de script C# et lie le modèle de structure actif.
    static bool init(model::Structure* structure, bool* needsRebuildFlag);

    /// Arrête proprement le runtime C#.
    static void shutdown();

    /// Compile tous les scripts C# (.cs) du dossier assets/scripts avec Roslyn csc.exe.
    static bool compileScripts();

    /// Charge ou recharge à chaud (Hot-Reload) l'assembly C# StabileoScripts.dll.
    static bool loadAssembly();

    /// Exécute OnUIRender de tous les plugins C# (appelé à chaque frame ImGui).
    static void onUIRender();

    /// Panneau ImGui de gestion du moteur C# (compilation, statut, logs, plugins).
    static void drawScriptingWindow(bool* p_open = nullptr);

    /// Indique si le moteur C# est initialisé et prêt.
    static bool isInitialized();

    /// Nombre de plugins C# actifs détectés.
    static int getLoadedPluginCount();

    /// Dernier journal de compilation ou message système.
    static const std::string& getLastLog();

    /// Historique des messages émis par C# ou le moteur de script.
    static const std::vector<std::string>& getLogHistory();

private:
    static bool initHostfxr();
    static void bindNativeTable();
    static std::wstring toWideString(const std::string& str);
};

} // namespace scripting
