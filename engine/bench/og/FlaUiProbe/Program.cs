// FlaUiProbe - auxiliary UIA3 evidence for the og run.
//
// Attaches to the DRWUI main window, dumps the UIA3 element tree (name /
// control type / automation id / bounding rectangle, depth-capped) to a JSON
// file, and captures the screen. Qt5 apps typically expose a SHALLOW UIA tree;
// that fact itself is recorded evidence ("no UIA surface beyond the top-level
// window"), not a failure of the og leg - the primary results come from the
// coordinate driver + .rsf + destination diff.
//
// Usage: FlaUiProbe.exe <out-uia-tree.json>
//
// Licensed under the MIT license. See LICENSE.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Threading;
using FlaUI.Core;
using FlaUI.Core.AutomationElements;
using FlaUI.UIA3;

class FlaUiProbe
{
    static int Main(string[] args)
    {
        if (args.Length < 1)
        {
            Console.Error.WriteLine("usage: FlaUiProbe.exe <out-uia-tree.json>");
            return 2;
        }
        string outPath = args[0];
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(outPath)));
        var root = new Dictionary<string, object>
        {
            ["captured_utc"] = DateTime.UtcNow.ToString("o"),
            ["tool"] = "FlaUI.UIA3 4.0.0",
        };
        try
        {
            using var automation = new UIA3Automation();
            var all = automation.GetDesktop();
            var drwWindows = new List<Dictionary<string, object>>();
            foreach (var w in all.FindAllChildren())
            {
                try
                {
                    if (w.Properties.ProcessId == 0) continue;
                    var procName = "";
                    try { procName = System.Diagnostics.Process.GetProcessById(w.Properties.ProcessId).ProcessName; }
                    catch { }
                    if (procName != "DRWUI" && procName != "DRW") continue;
                    var node = Dump(w, 0, 12);
                    drwWindows.Add(node);
                }
                catch { }
            }
            root["drw_windows"] = drwWindows;
            root["window_count"] = drwWindows.Count;
            // screen capture via FlaUI.Core.Capturing
            try
            {
                var capture = FlaUI.Core.Capturing.Capture.Screen();
                string png = Path.ChangeExtension(outPath, ".png");
                capture.ToFile(png);
                root["screenshot"] = png;
            }
            catch (Exception ex)
            {
                root["screenshot_error"] = ex.Message;
            }
        }
        catch (Exception ex)
        {
            root["error"] = ex.ToString();
        }
        var options = new JsonSerializerOptions { WriteIndented = true };
        File.WriteAllText(outPath, JsonSerializer.Serialize(root, options));
        Console.WriteLine($"uia tree written: {outPath} (windows={root.ContainsKey("drw_windows")})");
        return 0;
    }

    static Dictionary<string, object> Dump(AutomationElement e, int depth, int maxDepth)
    {
        var node = new Dictionary<string, object>();
        try { node["name"] = e.Properties.Name ?? ""; } catch { node["name"] = "<err>"; }
        try { node["control_type"] = e.Properties.ControlType.ToString(); } catch { }
        try { node["automation_id"] = e.Properties.AutomationId ?? ""; } catch { }
        try { node["class"] = e.Properties.ClassName ?? ""; } catch { }
        try { var r = e.Properties.BoundingRectangle; node["rect"] = new[] { r.X, r.Y, r.Width, r.Height }; } catch { }
        if (depth < maxDepth)
        {
            var kids = new List<object>();
            foreach (var c in e.FindAllChildren())
            {
                kids.Add(Dump(c, depth + 1, maxDepth));
            }
            node["children"] = kids;
            node["children_count"] = kids.Count;
        }
        return node;
    }
}
