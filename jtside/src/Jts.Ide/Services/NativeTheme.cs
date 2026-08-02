using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Interop;

namespace JtsIde.Services;

/// Enables the immersive dark mode on the Windows title bar so the IDE
/// does not show a light system chrome on top of the dark theme.
public static class NativeTheme
{
    [DllImport("dwmapi.dll", PreserveSig = true)]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int attrValue, int attrSize);

    public static void EnableDarkTitleBar(Window window)
    {
        var os = Environment.OSVersion.Version;
        if (os.Major < 10) return;
        try
        {
            var hwnd = new WindowInteropHelper(window).EnsureHandle();
            const int DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
            int enabled = 1;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, ref enabled, sizeof(int));
        }
        catch
        {
            // older Windows without DWM attribute support
        }
    }
}
