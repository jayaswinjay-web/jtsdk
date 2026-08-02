using System.Diagnostics;
using System.IO;
using System.Windows.Threading;

namespace JtsIde.Services;

/// <summary>
/// A long-lived JTS GO REPL process. The interpreter keeps its VM alive between
/// commands, so variables and functions declared in one line are visible in the
/// next. stdout/stderr are streamed back on the UI thread; stdin stays writable.
/// </summary>
public class ReplSession
{
    private readonly Dispatcher _dispatcher;
    private Process? _process;
    private StreamWriter? _stdin;
    private bool _exited;

    public ReplSession(Dispatcher dispatcher) => _dispatcher = dispatcher;

    public bool IsRunning => _process is not null && !_exited;

    public event Action<string>? OutputLine;
    public event Action<string>? ErrorLine;
    public event Action? Exited;

    public bool Start(string exe, string workDir)
    {
        Stop();
        try
        {
            var psi = new ProcessStartInfo(exe)
            {
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardInput = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                WorkingDirectory = Directory.Exists(workDir) ? workDir : Environment.CurrentDirectory,
            };
            var p = Process.Start(psi);
            if (p is null) return false;

            _process = p;
            _stdin = p.StandardInput;
            _exited = false;

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
                _exited = true;
                _stdin = null;
                Post(() => Exited?.Invoke());
            };
            p.BeginOutputReadLine();
            p.BeginErrorReadLine();
            return true;
        }
        catch
        {
            return false;
        }
    }

    public void SendLine(string line)
    {
        if (_stdin is null || _exited) return;
        try
        {
            _stdin.WriteLine(line);
            _stdin.Flush();
        }
        catch
        {
            /* pipe closed */
        }
    }

    public void Stop()
    {
        try { _process?.Kill(); } catch { /* already gone */ }
        _process = null;
        _stdin = null;
    }

    private void Post(Action a) => _dispatcher.BeginInvoke(a);
}
