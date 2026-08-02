using System.IO;

namespace JtsIde.Services;

/// Locates the JTS GO toolchain binaries. Search order:
///   1. installed location (%LocalAppData%\Programs\JTS GO\bin)
///   2. ancestor "bin\win32" of the app (development mode inside the repo)
///   3. the PATH environment variable
public class ToolchainResolver
{
    public string? JtsExe { get; }
    public string? JtscExe { get; }
    public string? JtsvmExe { get; }

    public ToolchainResolver()
    {
        JtsExe =
            InstalledJts() ??
            FindInAncestors(AppContext.BaseDirectory) ??
            FindOnPath("jts.exe") ??
            FindOnPath("jts.cmd") ??
            FindOnPath("jts");

        JtscExe = Derive("jtsc.exe");
        JtsvmExe = Derive("jtsvm.exe");
    }

    public string? Current => JtsExe;

    private static string? InstalledJts()
    {
        var local = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        var p = Path.Combine(local, "Programs", "JTS GO", "bin", "jts.exe");
        return File.Exists(p) ? p : null;
    }

    private static string? FindInAncestors(string startDir)
    {
        var dir = new DirectoryInfo(startDir);
        while (dir is not null)
        {
            var p = Path.Combine(dir.FullName, "bin", "win32", "jts.exe");
            if (File.Exists(p)) return p;
            dir = dir.Parent;
        }
        return null;
    }

    private static string? FindOnPath(string name)
    {
        var path = Environment.GetEnvironmentVariable("PATH") ?? "";
        foreach (var dir in path.Split(';', StringSplitOptions.RemoveEmptyEntries))
        {
            try
            {
                var p = Path.Combine(dir.Trim(), name);
                if (File.Exists(p)) return p;
            }
            catch { /* skip unreadable PATH entries */ }
        }
        return null;
    }

    private string? Derive(string name)
    {
        if (JtsExe is null) return null;
        var p = Path.Combine(Path.GetDirectoryName(JtsExe)!, name);
        return File.Exists(p) ? p : null;
    }
}
