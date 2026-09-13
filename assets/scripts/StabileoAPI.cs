// =============================================================================
//  StabileoAPI.cs — API C# pour StabileoViewer (Architecture style Hazel Engine)
//
//  Fournit les interfaces et wrappers pour :
//   - Stabileo.UI      : Composants graphiques Dear ImGui (fenêtres, boutons, sliders, etc.)
//   - Stabileo.Model   : Accès et manipulation du modèle de structure (nœuds, barres, appuis, charges)
//   - Stabileo.Solver  : Exécution du solveur EF direct et extraction des résultats
//   - Stabileo.Log     : Journalisation dans la console de l'application
//   - IStabileoPlugin  : Interface de cycle de vie pour plugins et panneaux d'ingénierie C#
// =============================================================================

using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Stabileo
{
    // -------------------------------------------------------------------------
    // Attribut & Interface de Plugin C#
    // -------------------------------------------------------------------------
    [AttributeUsage(AttributeTargets.Class)]
    public class StabileoPluginAttribute : Attribute
    {
        public string Name { get; }
        public string Category { get; }
        public string Description { get; }
        public string Author { get; }

        public StabileoPluginAttribute(string name, string category = "Général", string description = "", string author = "Ingénieur")
        {
            Name = name;
            Category = category;
            Description = description;
            Author = author;
        }
    }

    public interface IStabileoPlugin
    {
        void OnLoad();
        void OnUIRender();
        void OnUnload();
    }

    // -------------------------------------------------------------------------
    // Stabileo.Log
    // -------------------------------------------------------------------------
    public static class Log
    {
        public static void Info(string message)    => Internal.Native.Log(0, message);
        public static void Warning(string message) => Internal.Native.Log(1, message);
        public static void Error(string message)   => Internal.Native.Log(2, message);
    }

    // -------------------------------------------------------------------------
    // Stabileo.UI — Wrappers ImGui élégants
    // -------------------------------------------------------------------------
    public static class UI
    {
        public static bool Begin(string title) => Internal.Native.Begin(title, IntPtr.Zero, 0);
        public static void End() => Internal.Native.End();

        public static void Text(string text) => Internal.Native.Text(text);
        public static void TextColored(string text, float r, float g, float b, float a = 1.0f) => Internal.Native.TextColored(r, g, b, a, text);

        public static bool Button(string label, float width = 0.0f, float height = 0.0f) => Internal.Native.Button(label, width, height);
        public static bool Checkbox(string label, ref bool val) => Internal.Native.Checkbox(label, ref val);

        public static bool SliderFloat(string label, ref float val, float min, float max) => Internal.Native.SliderFloat(label, ref val, min, max);
        public static bool SliderInt(string label, ref int val, int min, int max) => Internal.Native.SliderInt(label, ref val, min, max);

        public static bool InputFloat(string label, ref float val) => Internal.Native.InputFloat(label, ref val);
        public static bool InputInt(string label, ref int val) => Internal.Native.InputInt(label, ref val);

        public static bool ColorEdit3(string label, ref float r, ref float g, ref float b)
        {
            float[] col = new float[] { r, g, b };
            bool changed = Internal.Native.ColorEdit3(label, col);
            if (changed)
            {
                r = col[0]; g = col[1]; b = col[2];
            }
            return changed;
        }

        public static void Separator() => Internal.Native.Separator();
        public static void SameLine(float offsetFromStartX = 0.0f, float spacing = -1.0f) => Internal.Native.SameLine(offsetFromStartX, spacing);
        public static void Spacing() => Internal.Native.Spacing();

        public static void ProgressBar(float fraction, string overlay = null, float width = -1.0f, float height = 0.0f)
            => Internal.Native.ProgressBar(fraction, width, height, overlay);

        public static bool CollapsingHeader(string label, bool defaultOpen = false)
            => Internal.Native.CollapsingHeader(label, defaultOpen ? (1 << 5) : 0);

        public static bool TreeNode(string label) => Internal.Native.TreeNode(label);
        public static void TreePop() => Internal.Native.TreePop();

        public static void HelpMarker(string desc)
        {
            TextColored("(?)", 0.5f, 0.7f, 1.0f, 1.0f);
            SameLine();
            Text(desc);
        }
    }

    // -------------------------------------------------------------------------
    // Stabileo.Model — Accès au modèle de structure
    // -------------------------------------------------------------------------
    public static class Model
    {
        public static int NodeCount => Internal.Native.GetNodeCount();
        public static int ElementCount => Internal.Native.GetElementCount();

        public static bool GetNodePosition(int id, out float x, out float y, out float z)
            => Internal.Native.GetNodePos(id, out x, out y, out z);

        public static int AddNode(float x, float y, float z)
            => Internal.Native.AddNode(x, y, z);

        public static int AddElement(int nodeI, int nodeJ, string sectionName = "IPE 300", bool isTruss = false)
            => Internal.Native.AddElement(nodeI, nodeJ, sectionName, isTruss);

        public enum SupportType
        {
            Fixed = 0,
            Pinned = 1,
            RollerX = 2,
            RollerY = 3,
            RollerZ = 4
        }

        public static bool AddSupport(int nodeId, SupportType type)
            => Internal.Native.AddSupport(nodeId, (int)type);

        public static bool AddNodalLoad(int nodeId, float fx, float fy, float fz, float mx = 0.0f, float my = 0.0f, float mz = 0.0f)
            => Internal.Native.AddNodalLoad(nodeId, fx, fy, fz, mx, my, mz);

        public static void ClearAll() => Internal.Native.ClearModel();
        public static void RequestRebuild() => Internal.Native.RequestRebuild();
    }

    // -------------------------------------------------------------------------
    // Stabileo.Solver — Solveur EF direct
    // -------------------------------------------------------------------------
    public static class Solver
    {
        public static bool Solve() => Internal.Native.SolveStatic();
        public static bool HasResults => Internal.Native.HasResults();

        public static float MaxDisplacement => Internal.Native.GetMaxDisplacement();
        public static float MaxNormalForce => Internal.Native.GetMaxNormalForce();
        public static float MaxBendingMoment => Internal.Native.GetMaxBendingMoment();

        public static float GetElementNormalForce(int elemId) => Internal.Native.GetElementNormalForce(elemId);
        public static float GetElementBendingMoment(int elemId) => Internal.Native.GetElementBendingMoment(elemId);
        public static float GetElementStressRatio(int elemId) => Internal.Native.GetElementStressRatio(elemId);
    }

    // -------------------------------------------------------------------------
    // Infrastructure interne & Point d'entrée P/Invoke
    // -------------------------------------------------------------------------
    namespace Internal
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool Native_Begin(string name, IntPtr p_open, int flags);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void Native_End();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate void Native_Text(string text);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate void Native_TextColored(float r, float g, float b, float a, string text);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool Native_Button(string label, float width, float height);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool Native_Checkbox(string label, [In, Out] ref bool val);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool Native_SliderFloat(string label, [In, Out] ref float val, float min, float max);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool Native_SliderInt(string label, [In, Out] ref int val, int min, int max);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool Native_InputFloat(string label, [In, Out] ref float val);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool Native_InputInt(string label, [In, Out] ref int val);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool Native_ColorEdit3(string label, [In, Out] float[] col3);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void Native_Separator();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void Native_SameLine(float offset, float spacing);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void Native_Spacing();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate void Native_ProgressBar(float fraction, float width, float height, string overlay);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool Native_CollapsingHeader(string label, int flags);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate bool Native_TreeNode(string label);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void Native_TreePop();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate void Native_Log(int level, string message);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int Native_GetNodeCount();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int Native_GetElementCount();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate bool Native_GetNodePos(int id, out float x, out float y, out float z);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate int Native_AddNode(float x, float y, float z);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate int Native_AddElement(int nodeI, int nodeJ, string sectionName, bool isTruss);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate bool Native_AddSupport(int nodeId, int type);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate bool Native_AddNodalLoad(int nodeId, float fx, float fy, float fz, float mx, float my, float mz);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void Native_ClearModel();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void Native_RequestRebuild();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate bool Native_SolveStatic();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate bool Native_HasResults();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate float Native_GetMaxDisplacement();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate float Native_GetMaxNormalForce();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate float Native_GetMaxBendingMoment();

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate float Native_GetElementNormalForce(int elemId);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate float Native_GetElementBendingMoment(int elemId);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate float Native_GetElementStressRatio(int elemId);

        [StructLayout(LayoutKind.Sequential)]
        public struct NativeEngineTable
        {
            public IntPtr Begin;
            public IntPtr End;
            public IntPtr Text;
            public IntPtr TextColored;
            public IntPtr Button;
            public IntPtr Checkbox;
            public IntPtr SliderFloat;
            public IntPtr SliderInt;
            public IntPtr InputFloat;
            public IntPtr InputInt;
            public IntPtr ColorEdit3;
            public IntPtr Separator;
            public IntPtr SameLine;
            public IntPtr Spacing;
            public IntPtr ProgressBar;
            public IntPtr CollapsingHeader;
            public IntPtr TreeNode;
            public IntPtr TreePop;
            public IntPtr Log;
            public IntPtr GetNodeCount;
            public IntPtr GetElementCount;
            public IntPtr GetNodePos;
            public IntPtr AddNode;
            public IntPtr AddElement;
            public IntPtr AddSupport;
            public IntPtr AddNodalLoad;
            public IntPtr ClearModel;
            public IntPtr RequestRebuild;
            public IntPtr SolveStatic;
            public IntPtr HasResults;
            public IntPtr GetMaxDisplacement;
            public IntPtr GetMaxNormalForce;
            public IntPtr GetMaxBendingMoment;
            public IntPtr GetElementNormalForce;
            public IntPtr GetElementBendingMoment;
            public IntPtr GetElementStressRatio;
        }

        public static class Native
        {
            public static Native_Begin Begin;
            public static Native_End End;
            public static Native_Text Text;
            public static Native_TextColored TextColored;
            public static Native_Button Button;
            public static Native_Checkbox Checkbox;
            public static Native_SliderFloat SliderFloat;
            public static Native_SliderInt SliderInt;
            public static Native_InputFloat InputFloat;
            public static Native_InputInt InputInt;
            public static Native_ColorEdit3 ColorEdit3;
            public static Native_Separator Separator;
            public static Native_SameLine SameLine;
            public static Native_Spacing Spacing;
            public static Native_ProgressBar ProgressBar;
            public static Native_CollapsingHeader CollapsingHeader;
            public static Native_TreeNode TreeNode;
            public static Native_TreePop TreePop;
            public static Native_Log Log;
            public static Native_GetNodeCount GetNodeCount;
            public static Native_GetElementCount GetElementCount;
            public static Native_GetNodePos GetNodePos;
            public static Native_AddNode AddNode;
            public static Native_AddElement AddElement;
            public static Native_AddSupport AddSupport;
            public static Native_AddNodalLoad AddNodalLoad;
            public static Native_ClearModel ClearModel;
            public static Native_RequestRebuild RequestRebuild;
            public static Native_SolveStatic SolveStatic;
            public static Native_HasResults HasResults;
            public static Native_GetMaxDisplacement GetMaxDisplacement;
            public static Native_GetMaxNormalForce GetMaxNormalForce;
            public static Native_GetMaxBendingMoment GetMaxBendingMoment;
            public static Native_GetElementNormalForce GetElementNormalForce;
            public static Native_GetElementBendingMoment GetElementBendingMoment;
            public static Native_GetElementStressRatio GetElementStressRatio;

            public static void Bind(ref NativeEngineTable t)
            {
                Begin = Marshal.GetDelegateForFunctionPointer<Native_Begin>(t.Begin);
                End = Marshal.GetDelegateForFunctionPointer<Native_End>(t.End);
                Text = Marshal.GetDelegateForFunctionPointer<Native_Text>(t.Text);
                TextColored = Marshal.GetDelegateForFunctionPointer<Native_TextColored>(t.TextColored);
                Button = Marshal.GetDelegateForFunctionPointer<Native_Button>(t.Button);
                Checkbox = Marshal.GetDelegateForFunctionPointer<Native_Checkbox>(t.Checkbox);
                SliderFloat = Marshal.GetDelegateForFunctionPointer<Native_SliderFloat>(t.SliderFloat);
                SliderInt = Marshal.GetDelegateForFunctionPointer<Native_SliderInt>(t.SliderInt);
                InputFloat = Marshal.GetDelegateForFunctionPointer<Native_InputFloat>(t.InputFloat);
                InputInt = Marshal.GetDelegateForFunctionPointer<Native_InputInt>(t.InputInt);
                ColorEdit3 = Marshal.GetDelegateForFunctionPointer<Native_ColorEdit3>(t.ColorEdit3);
                Separator = Marshal.GetDelegateForFunctionPointer<Native_Separator>(t.Separator);
                SameLine = Marshal.GetDelegateForFunctionPointer<Native_SameLine>(t.SameLine);
                Spacing = Marshal.GetDelegateForFunctionPointer<Native_Spacing>(t.Spacing);
                ProgressBar = Marshal.GetDelegateForFunctionPointer<Native_ProgressBar>(t.ProgressBar);
                CollapsingHeader = Marshal.GetDelegateForFunctionPointer<Native_CollapsingHeader>(t.CollapsingHeader);
                TreeNode = Marshal.GetDelegateForFunctionPointer<Native_TreeNode>(t.TreeNode);
                TreePop = Marshal.GetDelegateForFunctionPointer<Native_TreePop>(t.TreePop);
                Log = Marshal.GetDelegateForFunctionPointer<Native_Log>(t.Log);
                GetNodeCount = Marshal.GetDelegateForFunctionPointer<Native_GetNodeCount>(t.GetNodeCount);
                GetElementCount = Marshal.GetDelegateForFunctionPointer<Native_GetElementCount>(t.GetElementCount);
                GetNodePos = Marshal.GetDelegateForFunctionPointer<Native_GetNodePos>(t.GetNodePos);
                AddNode = Marshal.GetDelegateForFunctionPointer<Native_AddNode>(t.AddNode);
                AddElement = Marshal.GetDelegateForFunctionPointer<Native_AddElement>(t.AddElement);
                AddSupport = Marshal.GetDelegateForFunctionPointer<Native_AddSupport>(t.AddSupport);
                AddNodalLoad = Marshal.GetDelegateForFunctionPointer<Native_AddNodalLoad>(t.AddNodalLoad);
                ClearModel = Marshal.GetDelegateForFunctionPointer<Native_ClearModel>(t.ClearModel);
                RequestRebuild = Marshal.GetDelegateForFunctionPointer<Native_RequestRebuild>(t.RequestRebuild);
                SolveStatic = Marshal.GetDelegateForFunctionPointer<Native_SolveStatic>(t.SolveStatic);
                HasResults = Marshal.GetDelegateForFunctionPointer<Native_HasResults>(t.HasResults);
                GetMaxDisplacement = Marshal.GetDelegateForFunctionPointer<Native_GetMaxDisplacement>(t.GetMaxDisplacement);
                GetMaxNormalForce = Marshal.GetDelegateForFunctionPointer<Native_GetMaxNormalForce>(t.GetMaxNormalForce);
                GetMaxBendingMoment = Marshal.GetDelegateForFunctionPointer<Native_GetMaxBendingMoment>(t.GetMaxBendingMoment);
                GetElementNormalForce = Marshal.GetDelegateForFunctionPointer<Native_GetElementNormalForce>(t.GetElementNormalForce);
                GetElementBendingMoment = Marshal.GetDelegateForFunctionPointer<Native_GetElementBendingMoment>(t.GetElementBendingMoment);
                GetElementStressRatio = Marshal.GetDelegateForFunctionPointer<Native_GetElementStressRatio>(t.GetElementStressRatio);
            }
        }
    }

    // -------------------------------------------------------------------------
    // Point d'entrée du runtime C# géré (CoreCLR)
    // -------------------------------------------------------------------------
    public static class EntryPoint
    {
        private static readonly List<IStabileoPlugin> s_Plugins = new List<IStabileoPlugin>();
        private static readonly List<string> s_PluginNames = new List<string>();

        [UnmanagedCallersOnly]
        public static int Initialize(IntPtr apiTablePtr)
        {
            try
            {
                if (apiTablePtr == IntPtr.Zero) return -1;
                var table = Marshal.PtrToStructure<Internal.NativeEngineTable>(apiTablePtr);
                Internal.Native.Bind(ref table);

                DiscoverPlugins();
                Log.Info($"[C# ScriptEngine] Initialisation réussie. {s_Plugins.Count} plugin(s) détecté(s).");
                return s_Plugins.Count;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[C# Exception in Initialize]: {ex}");
                return -2;
            }
        }

        [UnmanagedCallersOnly]
        public static void OnUIRender()
        {
            for (int i = 0; i < s_Plugins.Count; ++i)
            {
                try
                {
                    s_Plugins[i].OnUIRender();
                }
                catch (Exception ex)
                {
                    Log.Error($"[C# Plugin Error in {s_PluginNames[i]}]: {ex.Message}");
                }
            }
        }

        [UnmanagedCallersOnly]
        public static void Shutdown()
        {
            foreach (var plugin in s_Plugins)
            {
                try { plugin.OnUnload(); } catch { }
            }
            s_Plugins.Clear();
            s_PluginNames.Clear();
        }

        [UnmanagedCallersOnly]
        public static int GetPluginCount()
        {
            return s_Plugins.Count;
        }

        private static void DiscoverPlugins()
        {
            s_Plugins.Clear();
            s_PluginNames.Clear();

            var currentAssembly = Assembly.GetExecutingAssembly();
            foreach (var type in currentAssembly.GetTypes())
            {
                if (!type.IsAbstract && typeof(IStabileoPlugin).IsAssignableFrom(type))
                {
                    try
                    {
                        var plugin = (IStabileoPlugin)Activator.CreateInstance(type);
                        var attr = type.GetCustomAttribute<StabileoPluginAttribute>();
                        string name = attr?.Name ?? type.Name;

                        plugin.OnLoad();
                        s_Plugins.Add(plugin);
                        s_PluginNames.Add(name);
                        Log.Info($"[C# Plugin] Chargé : {name}");
                    }
                    catch (Exception ex)
                    {
                        Log.Error($"[C# Plugin] Échec chargement {type.FullName} : {ex.Message}");
                    }
                }
            }
        }
    }
}
