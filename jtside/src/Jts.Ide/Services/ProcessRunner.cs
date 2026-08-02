using System.Diagnostics;
using System.Windows.Threading;

namespace JtsIde.Services;

/// Runs the JTS GO toolchain, streaming stdout/stderr back on the UI thread.
public class ProcessRunner
{
    private readonly Dispatcher _dispatcher;
    private Process? _process;

    public ProcessRunner(Dispatcher dispatcher) => _dispatcher = dispatcher;

    public bool IsRunning => _process != null;

    public event Action<string>? OutputLine;
    public event Action<string>? ErrorLine;
    public event Action<int>? Exited;
    public event Action<string>? LaunchFailed;

    public bool Run(string exe, string args, string workDir)
    {
        try
        {
            var psi = new ProcessStartInfo(exe, args)
            {
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                WorkingDirectory = workDir,
            };
            var p = Process.Start(psi);
            if (p is null)
            {
                LaunchFailed?.Invoke("Failed to start the process.");
                return false;
            }

            _process = p;
            p.OutputDataReceived += (_, e) =>
            {
                if (e.Data != null) Post(() => OutputLine?.Invoke(e.Data));
            };
            p.ErrorDataReceived += (_, e) =>
            {
                if (e.Data != null) Post(() => ErrorLine?.Invoke(e.Data));
            };
            p.EnableRaisingEvents = true;
            p.Exited += (_, _) =>
            {
                p.WaitForExit();
                var code = p.ExitCode;
                _process = null;
                Post(() => Exited?.Invoke(code));
            };
            p.BeginOutputReadLine();
            p.BeginErrorReadLine();
            return true;
        }
        catch (Exception ex)
        {
            LaunchFailed?.Invoke(ex.Message);
            return false;
        }
    }

    public void Stop()
    {
        try { _process?.Kill(entireProcessTree: true); } catch { /* already gone */ }
    }

    private void Post(Action a) => _dispatcher.BeginInvoke(a);
}
