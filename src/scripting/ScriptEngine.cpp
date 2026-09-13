// =============================================================================
//  ScriptEngine.cpp — Implémentation du moteur de script C# (CoreCLR / Hazel)
// =============================================================================

#include "scripting/ScriptEngine.h"
#include <imgui.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cmath>
#include <algorithm>

namespace fs = std::filesystem;

#define UNMANAGEDCALLERSONLY_METHOD ((const wchar_t*)-1)

enum hostfxr_delegate_type {
    hdt_load_assembly_and_get_function_pointer = 5
};

typedef int (__cdecl *hostfxr_initialize_for_runtime_config_fn)(
    const wchar_t *runtime_config_path,
    const void *parameters,
    void **host_context_handle
);

typedef int (__cdecl *hostfxr_get_runtime_delegate_fn)(
    void *host_context_handle,
    enum hostfxr_delegate_type type,
    void **delegate
);

typedef int (__cdecl *hostfxr_close_fn)(
    void *host_context_handle
);

typedef int (__cdecl *load_assembly_and_get_function_pointer_fn)(
    const wchar_t *assembly_path_to_load,
    const wchar_t *type_name,
    const wchar_t *method_name,
    const wchar_t *delegate_type_name,
    void *reserved,
    void **delegate
);

// Signatures des points d'entrée C#
typedef int (__cdecl *CSharp_Initialize_fn)(void* apiTable);
typedef void (__cdecl *CSharp_OnUIRender_fn)();
typedef void (__cdecl *CSharp_Shutdown_fn)();
typedef int (__cdecl *CSharp_GetPluginCount_fn)();

namespace scripting {

// Table interne de pointeurs de fonction C++ exposés à C#
struct NativeEngineTable {
    void* Begin;
    void* End;
    void* Text;
    void* TextColored;
    void* Button;
    void* Checkbox;
    void* SliderFloat;
    void* SliderInt;
    void* InputFloat;
    void* InputInt;
    void* ColorEdit3;
    void* Separator;
    void* SameLine;
    void* Spacing;
    void* ProgressBar;
    void* CollapsingHeader;
    void* TreeNode;
    void* TreePop;
    void* Log;
    void* GetNodeCount;
    void* GetElementCount;
    void* GetNodePos;
    void* AddNode;
    void* AddElement;
    void* AddSupport;
    void* AddNodalLoad;
    void* ClearModel;
    void* RequestRebuild;
    void* SolveStatic;
    void* HasResults;
    void* GetMaxDisplacement;
    void* GetMaxNormalForce;
    void* GetMaxBendingMoment;
    void* GetElementNormalForce;
    void* GetElementBendingMoment;
    void* GetElementStressRatio;
};

// Données statiques du moteur
static ScriptEngineConfig s_config;
static model::Structure* s_structure = nullptr;
static bool* s_needsRebuildFlag = nullptr;

static HMODULE s_hHostfxr = nullptr;
static void* s_hostContext = nullptr;
static load_assembly_and_get_function_pointer_fn s_loadAssemblyFn = nullptr;
static hostfxr_close_fn s_closeFn = nullptr;

static CSharp_Initialize_fn s_csharpInit = nullptr;
static CSharp_OnUIRender_fn s_csharpUIRender = nullptr;
static CSharp_Shutdown_fn s_csharpShutdown = nullptr;
static CSharp_GetPluginCount_fn s_csharpGetPluginCount = nullptr;

static bool s_isInitialized = false;
static int s_loadedPlugins = 0;
static std::string s_lastLog;
static std::vector<std::string> s_logHistory;
static NativeEngineTable s_nativeTable;

// ---- Callbacks ImGui C++ -> C# ----
static bool ImGui_Begin(const char* name, void*, int flags) {
    return ImGui::Begin(name ? name : "Fenêtre C#", nullptr, flags);
}

static void ImGui_End() {
    ImGui::End();
}

static void ImGui_Text(const char* text) {
    if (text) ImGui::TextUnformatted(text);
}

static void ImGui_TextColored(float r, float g, float b, float a, const char* text) {
    if (text) ImGui::TextColored(ImVec4(r, g, b, a), "%s", text);
}

static bool ImGui_Button(const char* label, float w, float h) {
    return ImGui::Button(label ? label : "Bouton", ImVec2(w, h));
}

static bool ImGui_Checkbox(const char* label, bool* val) {
    bool dummy = false;
    return ImGui::Checkbox(label ? label : "Option", val ? val : &dummy);
}

static bool ImGui_SliderFloat(const char* label, float* val, float min, float max) {
    float dummy = 0.0f;
    return ImGui::SliderFloat(label ? label : "Valeur", val ? val : &dummy, min, max, "%.3f");
}

static bool ImGui_SliderInt(const char* label, int* val, int min, int max) {
    int dummy = 0;
    return ImGui::SliderInt(label ? label : "Entier", val ? val : &dummy, min, max);
}

static bool ImGui_InputFloat(const char* label, float* val) {
    float dummy = 0.0f;
    return ImGui::InputFloat(label ? label : "Nombre", val ? val : &dummy, 0.0f, 0.0f, "%.3f");
}

static bool ImGui_InputInt(const char* label, int* val) {
    int dummy = 0;
    return ImGui::InputInt(label ? label : "Nombre", val ? val : &dummy);
}

static bool ImGui_ColorEdit3(const char* label, float* col3) {
    float dummy[3] = { 1.0f, 1.0f, 1.0f };
    return ImGui::ColorEdit3(label ? label : "Couleur", col3 ? col3 : dummy);
}

static void ImGui_Separator() {
    ImGui::Separator();
}

static void ImGui_SameLine(float offset, float spacing) {
    ImGui::SameLine(offset, spacing);
}

static void ImGui_Spacing() {
    ImGui::Spacing();
}

static void ImGui_ProgressBar(float fraction, float w, float h, const char* overlay) {
    ImGui::ProgressBar(fraction, ImVec2(w, h), (overlay && overlay[0]) ? overlay : nullptr);
}

static bool ImGui_CollapsingHeader(const char* label, int flags) {
    return ImGui::CollapsingHeader(label ? label : "Groupe", flags);
}

static bool ImGui_TreeNode(const char* label) {
    return ImGui::TreeNode(label ? label : "Nœud");
}

static void ImGui_TreePop() {
    ImGui::TreePop();
}

static void Native_Log(int level, const char* msg) {
    std::string prefix = "[C# Info] ";
    if (level == 1) prefix = "[C# Avertissement] ";
    else if (level == 2) prefix = "[C# Erreur] ";

    std::string line = prefix + (msg ? msg : "");
    s_lastLog = line;
    s_logHistory.push_back(line);
    if (s_logHistory.size() > 500) {
        s_logHistory.erase(s_logHistory.begin());
    }
    std::cout << line << std::endl;
}

// ---- Callbacks Modèle de Structure ----
static int Native_GetNodeCount() {
    return s_structure ? static_cast<int>(s_structure->nodes.size()) : 0;
}

static int Native_GetElementCount() {
    return s_structure ? static_cast<int>(s_structure->elements.size()) : 0;
}

static bool Native_GetNodePos(int id, float* x, float* y, float* z) {
    if (!s_structure) return false;
    for (const auto& node : s_structure->nodes) {
        if (node.id == id) {
            if (x) *x = node.position.x;
            if (y) *y = node.position.y;
            if (z) *z = node.position.z;
            return true;
        }
    }
    return false;
}

static int Native_AddNode(float x, float y, float z) {
    if (!s_structure) return -1;
    int nextId = 1;
    for (const auto& n : s_structure->nodes) {
        if (n.id >= nextId) nextId = n.id + 1;
    }
    model::Node node;
    node.id = nextId;
    node.position = glm::vec3(x, y, z);
    node.displacement = glm::vec3(0.0f);
    node.rotation = glm::vec3(0.0f);
    s_structure->nodes.push_back(node);
    return nextId;
}

static int Native_AddElement(int nodeI, int nodeJ, const char* sectionName, bool isTruss) {
    if (!s_structure) return -1;
    int nextId = 1;
    for (const auto& e : s_structure->elements) {
        if (e.id >= nextId) nextId = e.id + 1;
    }

    // Assurer qu'une section par défaut existe
    int secId = 0;
    if (s_structure->sections.empty()) {
        model::Section sec;
        sec.id = 0;
        sec.name = (sectionName && sectionName[0]) ? sectionName : "IPE 300";
        s_structure->sections.push_back(sec);
    } else {
        secId = s_structure->sections.front().id;
    }

    model::Element elem;
    elem.id = nextId;
    elem.nodeI = nodeI;
    elem.nodeJ = nodeJ;
    elem.sectionId = secId;
    elem.isTruss = isTruss;
    elem.rollAngle = 0.0f;
    s_structure->elements.push_back(elem);
    return nextId;
}

static bool Native_AddSupport(int nodeId, int type) {
    if (!s_structure) return false;
    model::Support supp;
    supp.nodeId = nodeId;
    switch (type) {
        case 0: supp.type = model::SupportType::FIXED; break;
        case 1: supp.type = model::SupportType::PINNED; break;
        case 2: supp.type = model::SupportType::ROLLER_X; break;
        case 3: supp.type = model::SupportType::ROLLER_Y; break;
        case 4: supp.type = model::SupportType::ROLLER_Z; break;
        default: supp.type = model::SupportType::FIXED; break;
    }
    supp.blockRx = (type == 0);
    supp.blockRy = (type == 0);
    supp.blockRz = (type == 0);
    s_structure->supports.push_back(supp);
    return true;
}

static bool Native_AddNodalLoad(int nodeId, float fx, float fy, float fz, float mx, float my, float mz) {
    if (!s_structure) return false;
    model::NodalLoad load;
    load.nodeId = nodeId;
    load.force = glm::vec3(fx, fy, fz);
    load.moment = glm::vec3(mx, my, mz);
    s_structure->nodalLoads.push_back(load);
    return true;
}

static void Native_ClearModel() {
    if (!s_structure) return;
    s_structure->nodes.clear();
    s_structure->elements.clear();
    s_structure->supports.clear();
    s_structure->nodalLoads.clear();
    s_structure->distributedLoads.clear();
    s_structure->reactions.clear();
    s_structure->hasResults = false;
}

static void Native_RequestRebuild() {
    if (s_needsRebuildFlag) {
        *s_needsRebuildFlag = true;
    }
}

// ---- Callbacks Solveur EF ----
static bool Native_SolveStatic() {
    if (!s_structure) return false;
    bool ok = solver::solveLinearStatic(*s_structure);
    if (s_needsRebuildFlag) {
        *s_needsRebuildFlag = true;
    }
    return ok;
}

static bool Native_HasResults() {
    return s_structure ? s_structure->hasResults : false;
}

static float Native_GetMaxDisplacement() {
    if (!s_structure || !s_structure->hasResults) return 0.0f;
    float maxD = 0.0f;
    for (const auto& n : s_structure->nodes) {
        float len = glm::length(n.displacement);
        if (len > maxD) maxD = len;
    }
    return maxD;
}

static float Native_GetMaxNormalForce() {
    if (!s_structure || !s_structure->hasResults) return 0.0f;
    float maxN = 0.0f;
    for (const auto& e : s_structure->elements) {
        for (float val : e.N) {
            if (std::abs(val) > std::abs(maxN)) maxN = val;
        }
    }
    return maxN;
}

static float Native_GetMaxBendingMoment() {
    if (!s_structure || !s_structure->hasResults) return 0.0f;
    float maxM = 0.0f;
    for (const auto& e : s_structure->elements) {
        for (float val : e.My) {
            if (std::abs(val) > std::abs(maxM)) maxM = val;
        }
        for (float val : e.Mz) {
            if (std::abs(val) > std::abs(maxM)) maxM = val;
        }
    }
    return maxM;
}

static float Native_GetElementNormalForce(int elemId) {
    if (!s_structure) return 0.0f;
    for (const auto& e : s_structure->elements) {
        if (e.id == elemId) {
            float maxN = 0.0f;
            for (float val : e.N) {
                if (std::abs(val) > std::abs(maxN)) maxN = val;
            }
            return maxN;
        }
    }
    return 0.0f;
}

static float Native_GetElementBendingMoment(int elemId) {
    if (!s_structure) return 0.0f;
    for (const auto& e : s_structure->elements) {
        if (e.id == elemId) {
            float maxM = 0.0f;
            for (float val : e.My) {
                if (std::abs(val) > std::abs(maxM)) maxM = val;
            }
            return maxM;
        }
    }
    return 0.0f;
}

static float Native_GetElementStressRatio(int elemId) {
    if (!s_structure) return 0.0f;
    for (const auto& e : s_structure->elements) {
        if (e.id == elemId) {
            float maxR = 0.0f;
            for (float val : e.stressRatio) {
                if (val > maxR) maxR = val;
            }
            return maxR;
        }
    }
    return 0.0f;
}

std::wstring ScriptEngine::toWideString(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);
    return wstr;
}

void ScriptEngine::bindNativeTable() {
    s_nativeTable.Begin = (void*)&ImGui_Begin;
    s_nativeTable.End = (void*)&ImGui_End;
    s_nativeTable.Text = (void*)&ImGui_Text;
    s_nativeTable.TextColored = (void*)&ImGui_TextColored;
    s_nativeTable.Button = (void*)&ImGui_Button;
    s_nativeTable.Checkbox = (void*)&ImGui_Checkbox;
    s_nativeTable.SliderFloat = (void*)&ImGui_SliderFloat;
    s_nativeTable.SliderInt = (void*)&ImGui_SliderInt;
    s_nativeTable.InputFloat = (void*)&ImGui_InputFloat;
    s_nativeTable.InputInt = (void*)&ImGui_InputInt;
    s_nativeTable.ColorEdit3 = (void*)&ImGui_ColorEdit3;
    s_nativeTable.Separator = (void*)&ImGui_Separator;
    s_nativeTable.SameLine = (void*)&ImGui_SameLine;
    s_nativeTable.Spacing = (void*)&ImGui_Spacing;
    s_nativeTable.ProgressBar = (void*)&ImGui_ProgressBar;
    s_nativeTable.CollapsingHeader = (void*)&ImGui_CollapsingHeader;
    s_nativeTable.TreeNode = (void*)&ImGui_TreeNode;
    s_nativeTable.TreePop = (void*)&ImGui_TreePop;
    s_nativeTable.Log = (void*)&Native_Log;
    s_nativeTable.GetNodeCount = (void*)&Native_GetNodeCount;
    s_nativeTable.GetElementCount = (void*)&Native_GetElementCount;
    s_nativeTable.GetNodePos = (void*)&Native_GetNodePos;
    s_nativeTable.AddNode = (void*)&Native_AddNode;
    s_nativeTable.AddElement = (void*)&Native_AddElement;
    s_nativeTable.AddSupport = (void*)&Native_AddSupport;
    s_nativeTable.AddNodalLoad = (void*)&Native_AddNodalLoad;
    s_nativeTable.ClearModel = (void*)&Native_ClearModel;
    s_nativeTable.RequestRebuild = (void*)&Native_RequestRebuild;
    s_nativeTable.SolveStatic = (void*)&Native_SolveStatic;
    s_nativeTable.HasResults = (void*)&Native_HasResults;
    s_nativeTable.GetMaxDisplacement = (void*)&Native_GetMaxDisplacement;
    s_nativeTable.GetMaxNormalForce = (void*)&Native_GetMaxNormalForce;
    s_nativeTable.GetMaxBendingMoment = (void*)&Native_GetMaxBendingMoment;
    s_nativeTable.GetElementNormalForce = (void*)&Native_GetElementNormalForce;
    s_nativeTable.GetElementBendingMoment = (void*)&Native_GetElementBendingMoment;
    s_nativeTable.GetElementStressRatio = (void*)&Native_GetElementStressRatio;
}

bool ScriptEngine::initHostfxr() {
    if (s_hHostfxr && s_loadAssemblyFn) return true;

    std::wstring hostfxrW = toWideString(s_config.hostfxrDllPath);
    s_hHostfxr = LoadLibraryW(hostfxrW.c_str());
    if (!s_hHostfxr) {
        s_lastLog = "[ScriptEngine] Erreur : Impossible de charger hostfxr.dll à " + s_config.hostfxrDllPath;
        s_logHistory.push_back(s_lastLog);
        return false;
    }

    auto init_fn = (hostfxr_initialize_for_runtime_config_fn)GetProcAddress(s_hHostfxr, "hostfxr_initialize_for_runtime_config");
    auto get_delegate_fn = (hostfxr_get_runtime_delegate_fn)GetProcAddress(s_hHostfxr, "hostfxr_get_runtime_delegate");
    s_closeFn = (hostfxr_close_fn)GetProcAddress(s_hHostfxr, "hostfxr_close");

    if (!init_fn || !get_delegate_fn || !s_closeFn) {
        s_lastLog = "[ScriptEngine] Erreur : Fonctions hostfxr manquantes dans la DLL.";
        s_logHistory.push_back(s_lastLog);
        return false;
    }

    // Assurer que le chemin runtimeconfig est absolu
    fs::path configPath(s_config.runtimeConfigPath);
    if (!fs::exists(configPath)) {
        // Tenter dans assets/scripts/Stabileo.Scripting.runtimeconfig.json
        if (fs::exists("assets/scripts/Stabileo.Scripting.runtimeconfig.json")) {
            configPath = "assets/scripts/Stabileo.Scripting.runtimeconfig.json";
        }
    }

    std::wstring configPathW = toWideString(fs::absolute(configPath).string());
    int rc = init_fn(configPathW.c_str(), nullptr, &s_hostContext);
    if (rc != 0 || !s_hostContext) {
        std::stringstream ss;
        ss << "[ScriptEngine] Échec hostfxr_initialize_for_runtime_config (code: 0x" << std::hex << rc << ")";
        s_lastLog = ss.str();
        s_logHistory.push_back(s_lastLog);
        return false;
    }

    rc = get_delegate_fn(s_hostContext, hdt_load_assembly_and_get_function_pointer, (void**)&s_loadAssemblyFn);
    if (rc != 0 || !s_loadAssemblyFn) {
        std::stringstream ss;
        ss << "[ScriptEngine] Échec get_runtime_delegate (code: 0x" << std::hex << rc << ")";
        s_lastLog = ss.str();
        s_logHistory.push_back(s_lastLog);
        return false;
    }

    return true;
}

bool ScriptEngine::init(model::Structure* structure, bool* needsRebuildFlag) {
    s_structure = structure;
    s_needsRebuildFlag = needsRebuildFlag;

    bindNativeTable();

    // S'assurer que le binaire initial existe (compilation si besoin)
    if (!fs::exists(s_config.assemblyPath)) {
        compileScripts();
    }

    return loadAssembly();
}

void ScriptEngine::shutdown() {
    if (s_csharpShutdown) {
        try {
            s_csharpShutdown();
        } catch (...) {}
    }

    s_csharpInit = nullptr;
    s_csharpUIRender = nullptr;
    s_csharpShutdown = nullptr;
    s_csharpGetPluginCount = nullptr;
    s_isInitialized = false;

    if (s_closeFn && s_hostContext) {
        s_closeFn(s_hostContext);
        s_hostContext = nullptr;
    }

    if (s_hHostfxr) {
        FreeLibrary(s_hHostfxr);
        s_hHostfxr = nullptr;
    }
}

bool ScriptEngine::compileScripts() {
    s_lastLog = "[ScriptEngine] Lancement de la compilation Roslyn (csc.exe)...";
    s_logHistory.push_back(s_lastLog);

    if (!fs::exists(s_config.roslynCscPath)) {
        s_lastLog = "[ScriptEngine] Erreur : Compilateur Roslyn introuvable à " + s_config.roslynCscPath;
        s_logHistory.push_back(s_lastLog);
        return false;
    }

    // Créer le dossier bin de sortie si nécessaire
    fs::create_directories(fs::path(s_config.assemblyPath).parent_path());

    // Collecter les fichiers .cs
    std::vector<std::string> csFiles;
    if (fs::exists(s_config.scriptsDir)) {
        for (const auto& entry : fs::directory_iterator(s_config.scriptsDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".cs") {
                csFiles.push_back(entry.path().string());
            }
        }
    }

    if (csFiles.empty()) {
        s_lastLog = "[ScriptEngine] Avertissement : Aucun fichier .cs trouvé dans " + s_config.scriptsDir;
        s_logHistory.push_back(s_lastLog);
        return false;
    }

    // Préparer la commande csc.exe
    std::string dotNetDir = s_config.dotNetSharedPath;
    std::stringstream cmd;
    cmd << "\"" << s_config.roslynCscPath << "\" /target:library /nostdlib "
        << "/r:\"" << dotNetDir << "\\System.Runtime.dll\" "
        << "/r:\"" << dotNetDir << "\\System.Collections.dll\" "
        << "/r:\"" << dotNetDir << "\\System.Console.dll\" "
        << "/r:\"" << dotNetDir << "\\System.Private.CoreLib.dll\" "
        << "/r:\"" << dotNetDir << "\\System.Runtime.InteropServices.dll\" "
        << "/out:\"" << s_config.assemblyPath << "\" ";

    for (const auto& f : csFiles) {
        cmd << "\"" << f << "\" ";
    }

    // Exécuter et capturer le flux de sortie
    std::string cmdStr = cmd.str() + " 2>&1";
    FILE* pipe = _popen(cmdStr.c_str(), "r");
    if (!pipe) {
        s_lastLog = "[ScriptEngine] Impossible d'exécuter Roslyn csc.exe.";
        s_logHistory.push_back(s_lastLog);
        return false;
    }

    char buffer[256];
    std::string compilerOutput;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        compilerOutput += buffer;
    }
    int returnCode = _pclose(pipe);

    if (returnCode == 0) {
        s_lastLog = "[ScriptEngine] Compilation C# réussie avec succès !";
        s_logHistory.push_back(s_lastLog);
        return true;
    } else {
        s_lastLog = "[ScriptEngine] Échec compilation Roslyn :\n" + compilerOutput;
        s_logHistory.push_back(s_lastLog);
        return false;
    }
}

bool ScriptEngine::loadAssembly() {
    if (!initHostfxr()) {
        return false;
    }

    if (!fs::exists(s_config.assemblyPath)) {
        s_lastLog = "[ScriptEngine] L'assembly " + s_config.assemblyPath + " n'existe pas.";
        s_logHistory.push_back(s_lastLog);
        return false;
    }

    std::wstring assemblyPathW = toWideString(fs::absolute(s_config.assemblyPath).string());
    const wchar_t* entryTypeName = L"Stabileo.EntryPoint, StabileoScripts";

    // 1. Point d'entrée Initialize
    int rc = s_loadAssemblyFn(
        assemblyPathW.c_str(),
        entryTypeName,
        L"Initialize",
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        (void**)&s_csharpInit
    );

    if (rc != 0 || !s_csharpInit) {
        std::stringstream ss;
        ss << "[ScriptEngine] Échec de liaison Initialize (code: 0x" << std::hex << rc << ")";
        s_lastLog = ss.str();
        s_logHistory.push_back(s_lastLog);
        return false;
    }

    // 2. Point d'entrée OnUIRender
    rc = s_loadAssemblyFn(
        assemblyPathW.c_str(),
        entryTypeName,
        L"OnUIRender",
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        (void**)&s_csharpUIRender
    );

    // 3. Point d'entrée Shutdown
    rc = s_loadAssemblyFn(
        assemblyPathW.c_str(),
        entryTypeName,
        L"Shutdown",
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        (void**)&s_csharpShutdown
    );

    // 4. Point d'entrée GetPluginCount
    rc = s_loadAssemblyFn(
        assemblyPathW.c_str(),
        entryTypeName,
        L"GetPluginCount",
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        (void**)&s_csharpGetPluginCount
    );

    // Exécuter l'initialisation C# avec notre table API native
    int count = s_csharpInit(&s_nativeTable);
    if (count >= 0) {
        s_isInitialized = true;
        s_loadedPlugins = count;
        std::stringstream ss;
        ss << "[ScriptEngine] Assembly C# chargé avec succès. " << count << " plugin(s) opérationnel(s).";
        s_lastLog = ss.str();
        s_logHistory.push_back(s_lastLog);
        return true;
    } else {
        s_lastLog = "[ScriptEngine] Erreur lors de l'appel Initialize dans l'assembly C#.";
        s_logHistory.push_back(s_lastLog);
        return false;
    }
}

void ScriptEngine::onUIRender() {
    if (s_isInitialized && s_csharpUIRender) {
        try {
            s_csharpUIRender();
        } catch (...) {
            s_lastLog = "[ScriptEngine] Exception non gérée lors du rendu UI C#.";
            s_logHistory.push_back(s_lastLog);
        }
    }
}

void ScriptEngine::drawScriptingWindow(bool* p_open) {
    if (!ImGui::Begin("Scripts & Plugins C# (Hazel)###CSharpScripting", p_open)) {
        ImGui::End();
        return;
    }

    // En-tête statut
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Moteur de Scripting C# .NET 6 (Architecture Hazel Engine)");
    ImGui::Text("Personnalisation d'interface, calculs Eurocodes et générateurs paramétriques.");
    ImGui::Separator();

    // État du runtime
    if (s_isInitialized) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "[ Connecté ] Runtime .NET CoreCLR actif");
        ImGui::SameLine();
        ImGui::Text("| %d plugin(s) en mémoire", s_loadedPlugins);
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.2f, 1.0f), "[ Déconnecté ] Runtime C# inactif");
    }

    ImGui::Spacing();

    // Boutons d'action
    if (ImGui::Button("Compiler les scripts C# (Roslyn csc.exe)", ImVec2(280, 28))) {
        if (compileScripts()) {
            loadAssembly();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Recharger à chaud (Hot-Reload)", ImVec2(220, 28))) {
        if (s_csharpShutdown) {
            try { s_csharpShutdown(); } catch (...) {}
        }
        loadAssembly();
    }

    ImGui::Spacing();

    // Arborescence des plugins détectés
    if (ImGui::CollapsingHeader("Plugins C# actifs", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("Eurocode 3 - Vérification Acier (EN 1993-1-1)");
        ImGui::BulletText("Générateur Paramétrique de Treillis (Warren / Pratt / Howe)");
    }

    // Paramètres des chemins d'accès
    if (ImGui::CollapsingHeader("Configuration des Chemins Moteur")) {
        ImGui::TextDisabled("Compilateur Roslyn :");
        ImGui::TextWrapped("%s", s_config.roslynCscPath.c_str());
        ImGui::TextDisabled("Bibliothèque hostfxr :");
        ImGui::TextWrapped("%s", s_config.hostfxrDllPath.c_str());
        ImGui::TextDisabled("Dossier des scripts C# :");
        ImGui::TextWrapped("%s", s_config.scriptsDir.c_str());
        ImGui::TextDisabled("Assembly binaire généré :");
        ImGui::TextWrapped("%s", s_config.assemblyPath.c_str());
    }

    // Journal des événements et compilation
    if (ImGui::CollapsingHeader("Journal du Moteur C# & Roslyn", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginChild("LogScrollRegion", ImVec2(0, 180), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& log : s_logHistory) {
            if (log.find("Erreur") != std::string::npos || log.find("Error") != std::string::npos || log.find("Échec") != std::string::npos) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", log.c_str());
            } else if (log.find("Avertissement") != std::string::npos || log.find("Warning") != std::string::npos) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%s", log.c_str());
            } else {
                ImGui::TextUnformatted(log.c_str());
            }
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        if (ImGui::Button("Effacer le journal", ImVec2(140, 22))) {
            s_logHistory.clear();
        }
    }

    ImGui::End();
}

bool ScriptEngine::isInitialized() {
    return s_isInitialized;
}

int ScriptEngine::getLoadedPluginCount() {
    return s_loadedPlugins;
}

const std::string& ScriptEngine::getLastLog() {
    return s_lastLog;
}

const std::vector<std::string>& ScriptEngine::getLogHistory() {
    return s_logHistory;
}

} // namespace scripting
