using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;
using System.Windows.Threading;

namespace JtsIde.Services;

/// Runs the JTS GO toolchain, streaming stdout/stderr back on the UI thread
/// and keeping stdin writable so running programs can call ask().
public class ProcessRunner
{
    private readonly Dispatcher _dispatcher;
    private Process? _process;
    private StreamWriter? _stdin;

    public ProcessRunner(Dispatcher dispatcher) => _dispatcher = dispatcher;

    public bool IsRunning => _process != null;

    /// Raw stdout text as it arrives (partial lines included), so prompts
    /// printed without a trailing newline show up immediately.
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
                RedirectStandardInput = true,
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
            _stdin = p.StandardInput;
            _ = Pump(p.StandardOutput, s => Post(() => OutputLine?.Invoke(s)));
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
                _stdin = null;
                Post(() => Exited?.Invoke(code));
            };
            p.BeginErrorReadLine();
            return true;
        }
        catch (Exception ex)
        {
            LaunchFailed?.Invoke(ex.Message);
            return false;
        }
    }

    private static async Task Pump(StreamReader reader, Action<string> deliver)
    {
        var buf = new char[4096];
        try
        {
            while (true)
            {
                int n = await reader.ReadAsync(buf, 0, buf.Length).ConfigureAwait(false);
                if (n == 0) break;
                deliver(new string(buf, 0, n));
            }
        }
        catch
        {
            // pipe closed by process exit or Stop()
        }
    }

    public void SendLine(string line)
    {
        if (_stdin is null || _process is null) return;
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
    }

    private void Post(Action a) => _dispatcher.BeginInvoke(a);
}
