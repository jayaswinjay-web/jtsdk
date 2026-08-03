using System;
using System.IO;
using System.Text;
using System.Windows;
using System.Windows.Threading;

namespace JtsIde;

public partial class App : Application
{
    private static readonly string LogPath = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "JTS GO", "ide-error.log");

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        DispatcherUnhandledException += OnDispatcherUnhandledException;
        AppDomain.CurrentDomain.UnhandledException += OnUnhandledException;

        try
        {
            var window = new MainWindow();
            MainWindow = window;
            if (e.Args.Length > 0)
            {
                window.OpenFileFromArg(e.Args[0]);
            }
            window.Show();
        }
        catch (Exception ex)
        {
            Report("JTS IDE failed to start.", ex);
            Shutdown(1);
        }
    }

    private static void OnDispatcherUnhandledException(object sender, DispatcherUnhandledExceptionEventArgs e)
    {
        Report("JTS IDE hit an unexpected error.", e.Exception);
        e.Handled = true;
    }

    private static void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
    {
        if (e.ExceptionObject is Exception ex)
        {
            Report("JTS IDE hit a fatal error.", ex);
        }
    }

    private static void Report(string title, Exception ex)
    {
        var text = new StringBuilder();
        text.AppendLine("=== " + title);
        text.AppendLine("Time: " + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"));
        text.AppendLine("Version: " + Jts.Core.LanguageInfo.Version);
        text.AppendLine("OS: " + Environment.OSVersion);
        text.AppendLine("64-bit: " + (Environment.Is64BitOperatingSystem ? "yes" : "no"));
        text.AppendLine();
        text.AppendLine(ex.ToString());
        text.AppendLine();
        text.AppendLine("--- JTS GO installed at ---");
        text.AppendLine(System.Reflection.Assembly.GetExecutingAssembly().Location);

        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(LogPath)!);
            File.AppendAllText(LogPath, text.ToString());
        }
        catch
        {
            // logging is best-effort; never let it mask the real error
        }

        try
        {
            MessageBox.Show(
                title + "\n\n" + ex.Message +
                "\n\nDetails were written to:\n" + LogPath,
                "JTS IDE",
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }
        catch
        {
            // no window available to show a dialog
        }
    }
}
