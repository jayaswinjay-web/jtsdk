using System.Collections.Generic;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using ICSharpCode.AvalonEdit;
using Microsoft.FSharp.Core;
using Jts.Core;
using JtsIde.Models;
using JtsIde.Services;

namespace JtsIde;

public partial class MainWindow : Window
{
    private const string DemoSource =
        "# Welcome to JTS IDE\n" +
        "# Press F5 to run this program.\n" +
        "\n" +
        "func fib(n)\n" +
        "    if n <= 1\n" +
        "        return n\n" +
        "    end\n" +
        "    return fib(n - 1) + fib(n - 2)\n" +
        "end\n" +
        "\n" +
        "for i in 0 to 10\n" +
        "    print(fib(i))\n" +
        "end\n";

    private readonly List<EditorDocument> _docs = new();
    private readonly ProcessRunner _runner;
    private readonly ReplSession _repl;
    private readonly ToolchainResolver _toolchain;
    private readonly List<string> _stderrLines = new();
    private string? _workspaceFolder;
    private bool _suppressDirty;

    private EditorDocument? Current =>
        EditorTabs.SelectedItem is TabItem ti
            ? _docs.FirstOrDefault(d => ReferenceEquals(d.Tab, ti))
            : null;

    public MainWindow()
    {
        InitializeComponent();
        SetRunInputEnabled(false);

        Title = "JTS IDE " + LanguageInfo.Version;
        Services.NativeTheme.EnableDarkTitleBar(this);
        _runner = new ProcessRunner(Dispatcher);
        _toolchain = new ToolchainResolver();
        _repl = new ReplSession(Dispatcher);
        _repl.OutputLine += line =>
        {
            ReplOutputBox.AppendText(line);
            ReplOutputBox.ScrollToEnd();
        };
        _repl.ErrorLine += line =>
        {
            ReplOutputBox.AppendText(line);
            ReplOutputBox.ScrollToEnd();
        };
        _repl.Exited += () =>
        {
            ReplOutputBox.AppendText("[console exited]\r\n");
            ReplOutputBox.ScrollToEnd();
            ReplInputBox.IsEnabled = false;
            StatusText.Text = "Console exited";
        };

        _runner.OutputLine += line => AppendRaw(line);
        _runner.ErrorLine += line =>
        {
            _stderrLines.Add(line);
            AppendOutput(line, isError: true);
        };
        _runner.Exited += code =>
        {
            AppendOutput("", isError: false);
            AppendOutput($"[process exited with code {code}]", isError: false);
            StatusText.Text = $"Process exited with code {code}";
            SetRunInputEnabled(false);
            ShowErrors(_stderrLines);
        };
        _runner.LaunchFailed += msg =>
        {
            AppendOutput($"[error] {msg}", isError: true);
            StatusText.Text = "Failed to launch toolchain";
        };

        ToolchainText.Text = _toolchain.JtsExe is null
            ? "jts: not found"
            : "jts: " + Path.GetFileName(Path.GetDirectoryName(_toolchain.JtsExe)) + "\\jts.exe";

        NewFile();

        if (_toolchain.JtsExe is null)
        {
            MessageBox.Show(
                "Could not find the JTS GO toolchain (jts.exe).\n\n" +
                "Install it with the JTS GO setup, or make sure jts is on your PATH.",
                "JTS IDE",
                MessageBoxButton.OK,
                MessageBoxImage.Warning);
        }
    }

    // ---------- documents / editor ----------

    private TextEditor CreateEditor()
    {
        var editor = new TextEditor
        {
            FontFamily = new FontFamily("Consolas"),
            FontSize = 14,
            ShowLineNumbers = true,
            WordWrap = false,
            HorizontalScrollBarVisibility = ScrollBarVisibility.Auto,
            VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
            Background = new SolidColorBrush(Color.FromRgb(0x1E, 0x1E, 0x1E)),
            Foreground = new SolidColorBrush(Color.FromRgb(0xD4, 0xD4, 0xD4)),
            LineNumbersForeground = new SolidColorBrush(Color.FromRgb(0x85, 0x85, 0x85)),
            BorderBrush = new SolidColorBrush(Color.FromRgb(0x3F, 0x3F, 0x46)),
        };
        editor.Options.HighlightCurrentLine = true;
        editor.TextArea.SelectionBrush = new SolidColorBrush(Color.FromRgb(0x26, 0x4F, 0x78));
        editor.TextArea.SelectionForeground = Brushes.White;
        editor.TextArea.Caret.CaretBrush = Brushes.White;
        editor.TextArea.TextView.CurrentLineBackground = new SolidColorBrush(Color.FromRgb(0x2A, 0x2D, 0x2E));
        editor.TextArea.TextView.LineTransformers.Add(new Highlighting.JtsHighlightTransformer());

        editor.TextChanged += (_, _) =>
        {
            if (_suppressDirty) return;
            var doc = FindDocument(editor);
            if (doc is null) return;
            doc.IsDirty = true;
            RefreshTabHeader(doc);
        };
        editor.TextArea.Caret.PositionChanged += (_, _) => UpdateCaret(editor);
        return editor;
    }

    private void NewFile(string? path = null, bool setDemo = false)
    {
        var editor = CreateEditor();
        var tab = new TabItem { Header = "" };
        tab.Content = editor;
        var doc = new EditorDocument { Tab = tab, Editor = editor, FilePath = path };
        _docs.Add(doc);
        EditorTabs.Items.Add(tab);
        EditorTabs.SelectedItem = tab;

        _suppressDirty = true;
        if (path is not null)
        {
            editor.Text = File.ReadAllText(path);
            editor.Document.UndoStack.ClearAll();
        }
        else if (setDemo)
        {
            editor.Text = DemoSource;
            editor.Document.UndoStack.ClearAll();
        }
        _suppressDirty = false;

        doc.IsDirty = false;
        RefreshTabHeader(doc);
        editor.Focus();
        UpdateCaret(editor);
    }

    private EditorDocument? FindDocument(TextEditor editor) =>
        _docs.FirstOrDefault(d => ReferenceEquals(d.Editor, editor));

    private void RefreshTabHeader(EditorDocument doc) => doc.Tab.Header = doc.TabHeader;

    private bool EnsureSaved(EditorDocument doc)
    {
        if (doc.FilePath is null)
        {
            var dlg = new Microsoft.Win32.SaveFileDialog
            {
                Title = "Save As",
                Filter = "JTS source (*.jts)|*.jts|All files (*.*)|*.*",
                FileName = doc.Title.EndsWith(".jts", StringComparison.OrdinalIgnoreCase)
                    ? doc.Title
                    : doc.Title + ".jts",
            };
            if (dlg.ShowDialog(this) != true) return false;
            doc.FilePath = dlg.FileName;
        }
        File.WriteAllText(doc.FilePath, doc.Editor.Text);
        doc.IsDirty = false;
        RefreshTabHeader(doc);
        StatusText.Text = "Saved " + doc.FilePath;
        return true;
    }

    private void CloseDocument(EditorDocument doc)
    {
        if (doc.IsDirty)
        {
            var r = MessageBox.Show(
                $"Save changes to {doc.Title}?",
                "JTS IDE",
                MessageBoxButton.YesNoCancel,
                MessageBoxImage.Question);
            if (r == MessageBoxResult.Cancel) return;
            if (r == MessageBoxResult.Yes && !EnsureSaved(doc)) return;
        }
        EditorTabs.Items.Remove(doc.Tab);
        _docs.Remove(doc);
        if (_docs.Count == 0) NewFile(setDemo: true);
    }

    private void UpdateCaret(TextEditor editor)
    {
        var pos = editor.TextArea.Caret.Position;
        CaretText.Text = $"Ln {pos.Line}, Col {pos.Column}";
    }

    // ---------- running / building ----------

    private void RunProgram()
    {
        var doc = Current;
        if (doc is null) return;
        if (_runner.IsRunning)
        {
            MessageBox.Show("A program is already running. Stop it first.", "JTS IDE",
                MessageBoxButton.OK, MessageBoxImage.Information);
            return;
        }
        if (!EnsureSaved(doc)) return;
        if (_toolchain.JtsExe is null)
        {
            MessageBox.Show("jts.exe was not found.", "JTS IDE", MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }

        ClearConsole();
        AppendOutput($"> {_toolchain.JtsExe} \"{doc.FilePath}\"", isError: false);
        StatusText.Text = "Running " + Path.GetFileName(doc.FilePath) + " ...";
        SetRunInputEnabled(true);
        _runner.Run(_toolchain.JtsExe, "\"" + doc.FilePath + "\"", Path.GetDirectoryName(doc.FilePath)!);
    }

    private void SetRunInputEnabled(bool enabled)
    {
        RunInputBox.IsEnabled = enabled;
        RunInputBox.Foreground = enabled
            ? new SolidColorBrush(Color.FromRgb(0xD4, 0xD4, 0xD4))
            : new SolidColorBrush(Color.FromRgb(0x85, 0x85, 0x85));
        RunInputBox.Text = enabled
            ? ""
            : "Run a program, or open the Console tab, to type input";
    }

    private void OnRunInputKeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key != Key.Enter) return;
        e.Handled = true;
        if (!_runner.IsRunning) return;
        var text = RunInputBox.Text.TrimEnd('\r', '\n');
        RunInputBox.Clear();
        if (OutputBox.Text.Length > 0 && !OutputBox.Text.EndsWith("\n", StringComparison.Ordinal))
        {
            AppendRaw(Environment.NewLine);
        }
        AppendOutput("> " + text, isError: false);
        _runner.SendLine(text);
    }

    private void BuildProgram()
    {
        var doc = Current;
        if (doc is null) return;
        if (_runner.IsRunning)
        {
            MessageBox.Show("A program is already running. Stop it first.", "JTS IDE",
                MessageBoxButton.OK, MessageBoxImage.Information);
            return;
        }
        if (!EnsureSaved(doc)) return;
        if (_toolchain.JtscExe is null)
        {
            MessageBox.Show("jtsc.exe was not found.", "JTS IDE", MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }

        ClearConsole();
        AppendOutput($"> {_toolchain.JtscExe} \"{doc.FilePath}\"", isError: false);
        StatusText.Text = "Building " + Path.GetFileName(doc.FilePath) + " ...";
        _runner.Run(_toolchain.JtscExe, "\"" + doc.FilePath + "\"", Path.GetDirectoryName(doc.FilePath)!);
    }

    private void ClearConsole()
    {
        OutputBox.Clear();
        _stderrLines.Clear();
        ErrorList.ItemsSource = null;
    }

    private void ShowErrors(IEnumerable<string> lines)
    {
        var errors = new List<ErrorItem>();
        string previous = "";
        foreach (var line in lines)
        {
            var parsed = Errors.tryParseWithPrevious(previous, line);
            if (FSharpOption<Errors.JtsError>.get_IsSome(parsed))
            {
                var e = parsed.Value;
                errors.Add(new ErrorItem(e.Line, e.IsRuntime ? "Runtime" : "Compile", e.Message));
            }
            previous = line;
        }
        ErrorList.ItemsSource = errors;
        if (errors.Count > 0)
        {
            ConsoleTabs.SelectedIndex = 1;
            StatusText.Text = $"{errors.Count} error(s)";
        }
    }

    private void JumpToLine(int line)
    {
        var doc = Current;
        if (doc is null) return;
        var document = doc.Editor.Document;
        if (line < 1) line = 1;
        if (line > document.LineCount) line = document.LineCount;
        var offset = document.GetOffset(line, 1);
        doc.Editor.ScrollTo(line, 1);
        doc.Editor.CaretOffset = offset;
        doc.Editor.TextArea.Caret.BringCaretToView();
        doc.Editor.Focus();
        var lineText = document.GetText(document.GetLineByNumber(line));
        doc.Editor.Select(offset, Math.Min(lineText.Length, 1));
        StatusText.Text = $"Line {line}: {lineText.Trim()}";
    }

    // ---------- console ----------

    private void AppendRaw(string text)
    {
        if (text.Length == 0) return;
        OutputBox.AppendText(text);
        OutputBox.ScrollToEnd();
    }

    private void AppendOutput(string line, bool isError)
    {
        OutputBox.AppendText((line.Length == 0 ? "" : line) + Environment.NewLine);
        OutputBox.ScrollToEnd();
    }

    // ---------- interactive console (REPL) ----------

    private string ReplWorkDir() =>
        _workspaceFolder ??
        (Current?.FilePath is { } p ? Path.GetDirectoryName(p) : null) ??
        Environment.CurrentDirectory;

    private void OnReplInputKeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key != Key.Enter) return;
        e.Handled = true;
        var text = ReplInputBox.Text.TrimEnd('\r', '\n');
        ReplInputBox.Clear();

        if (!_repl.IsRunning)
        {
            if (_toolchain.JtsExe is null)
            {
                ReplOutputBox.AppendText("[error] jts.exe was not found\r\n");
                ReplOutputBox.ScrollToEnd();
                return;
            }
            ReplOutputBox.Clear();
            ReplInputBox.IsEnabled = true;
            if (!_repl.Start(_toolchain.JtsExe, ReplWorkDir()))
            {
                ReplOutputBox.AppendText("[error] could not start the interactive console\r\n");
                ReplOutputBox.ScrollToEnd();
                return;
            }
        }

        if (string.IsNullOrWhiteSpace(text)) return;
        ReplOutputBox.AppendText(text + Environment.NewLine);
        ReplOutputBox.ScrollToEnd();
        _repl.SendLine(text);
    }

    private void OnReplRestart(object sender, RoutedEventArgs e)
    {
        if (_repl.IsRunning) _repl.Stop();
        ReplOutputBox.Clear();
        ReplInputBox.IsEnabled = true;
        if (_toolchain.JtsExe is null)
        {
            ReplOutputBox.AppendText("[error] jts.exe was not found\r\n");
            ReplOutputBox.ScrollToEnd();
            return;
        }
        if (!_repl.Start(_toolchain.JtsExe, ReplWorkDir()))
        {
            ReplOutputBox.AppendText("[error] could not start the interactive console\r\n");
            ReplOutputBox.ScrollToEnd();
        }
    }

    // ---------- file explorer ----------

    private void OpenFolder(string path)
    {
        _workspaceFolder = path;
        FolderPathBox.Text = path;
        FileTree.Items.Clear();
        var root = new TreeViewItem
        {
            Header = Path.GetFileName(path) + "\\",
            Tag = path,
            IsExpanded = true,
        };
        PopulateChildren(root, path);
        FileTree.Items.Add(root);
    }

    private static void PopulateChildren(TreeViewItem parent, string dir)
    {
        parent.Items.Clear();
        try
        {
            foreach (var d in Directory.GetDirectories(dir))
            {
                var node = new TreeViewItem { Header = Path.GetFileName(d) + "\\", Tag = d };
                PopulateChildren(node, d);
                parent.Items.Add(node);
            }
            foreach (var f in Directory.GetFiles(dir, "*.jts"))
            {
                parent.Items.Add(new TreeViewItem { Header = Path.GetFileName(f), Tag = f });
            }
        }
        catch { /* unreadable folder */ }
    }

    private void OnTreeDoubleClick(object sender, MouseButtonEventArgs e)
    {
        if (FileTree.SelectedItem is not TreeViewItem item || item.Tag is not string path) return;
        if (Directory.Exists(path))
        {
            item.IsExpanded = !item.IsExpanded;
            return;
        }
        OpenFile(path);
    }

    private void OpenFile(string path)
    {
        if (string.IsNullOrEmpty(path) || !File.Exists(path)) return;
        var existing = _docs.FirstOrDefault(d => d.FilePath is not null &&
            Path.GetFullPath(d.FilePath).Equals(Path.GetFullPath(path), StringComparison.OrdinalIgnoreCase));
        if (existing is not null)
        {
            EditorTabs.SelectedItem = existing.Tab;
            return;
        }
        NewFile(path);
    }

    public void OpenFileFromArg(string path)
    {
        OpenFile(path);
    }

    // ---------- menu handlers ----------

    private void OnNewFile(object sender, RoutedEventArgs e) => NewFile();
    private void OnOpenFile(object sender, RoutedEventArgs e)
    {
        var dlg = new Microsoft.Win32.OpenFileDialog
        {
            Title = "Open JTS file",
            Filter = "JTS source (*.jts)|*.jts|All files (*.*)|*.*",
        };
        if (dlg.ShowDialog(this) == true) OpenFile(dlg.FileName);
    }
    private void OnOpenFolder(object sender, RoutedEventArgs e)
    {
        using (var dlg = new System.Windows.Forms.FolderBrowserDialog())
        {
            dlg.Description = "Open a JTS GO project folder";
            if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                OpenFolder(dlg.SelectedPath);
        }
    }
    private void OnBrowseFolder(object sender, RoutedEventArgs e) => OnOpenFolder(sender, e);
    private void OnSave(object sender, RoutedEventArgs e)
    {
        if (Current is { } doc) EnsureSaved(doc);
    }
    private void OnSaveAs(object sender, RoutedEventArgs e)
    {
        if (Current is not { } doc) return;
        var dlg = new Microsoft.Win32.SaveFileDialog
        {
            Title = "Save As",
            Filter = "JTS source (*.jts)|*.jts|All files (*.*)|*.*",
            FileName = doc.Title,
        };
        if (dlg.ShowDialog(this) == true)
        {
            doc.FilePath = dlg.FileName;
            File.WriteAllText(doc.FilePath, doc.Editor.Text);
            doc.IsDirty = false;
            RefreshTabHeader(doc);
            StatusText.Text = "Saved " + doc.FilePath;
        }
    }
    private void OnCloseTab(object sender, RoutedEventArgs e)
    {
        if (Current is { } doc) CloseDocument(doc);
    }
    private void OnExit(object sender, RoutedEventArgs e) => Close();

    private void OnUndo(object sender, RoutedEventArgs e) => Current?.Editor.Undo();
    private void OnRedo(object sender, RoutedEventArgs e) => Current?.Editor.Redo();
    private void OnCut(object sender, RoutedEventArgs e) => Current?.Editor.Cut();
    private void OnCopy(object sender, RoutedEventArgs e) => Current?.Editor.Copy();
    private void OnPaste(object sender, RoutedEventArgs e) => Current?.Editor.Paste();
    private void OnSelectAll(object sender, RoutedEventArgs e) => Current?.Editor.SelectAll();

    private void OnRun(object sender, RoutedEventArgs e) => RunProgram();
    private void OnBuild(object sender, RoutedEventArgs e) => BuildProgram();
    private void OnStop(object sender, RoutedEventArgs e) => _runner.Stop();

    private void OnAbout(object sender, RoutedEventArgs e)
    {
        MessageBox.Show(
            $"JTS IDE {LanguageInfo.Version}\n\n" +
            $"An editor for {LanguageInfo.Name}, built by {LanguageInfo.Company}.\n\n" +
            $"Toolchain: {(_toolchain.JtsExe ?? "not found")}\n\n" +
            LanguageInfo.Repository,
            "About JTS IDE",
            MessageBoxButton.OK,
            MessageBoxImage.Information);
    }

    private void OnErrorDoubleClick(object sender, MouseButtonEventArgs e)
    {
        if (ErrorList.SelectedItem is not ErrorItem item) return;
        var doc = Current;
        if (doc is null) return;
        if (_toolchain.JtsExe is null) return;
        JumpToLine(item.Line);
    }

    protected override void OnPreviewKeyDown(KeyEventArgs e)
    {
        var ctrl = Keyboard.Modifiers.HasFlag(ModifierKeys.Control);
        if (ctrl && e.Key == Key.N) { NewFile(); e.Handled = true; }
        else if (ctrl && e.Key == Key.O) { OnOpenFile(this, new RoutedEventArgs()); e.Handled = true; }
        else if (ctrl && e.Key == Key.S) { OnSave(this, new RoutedEventArgs()); e.Handled = true; }
        else if (ctrl && e.Key == Key.W) { OnCloseTab(this, new RoutedEventArgs()); e.Handled = true; }
        else if (e.Key == Key.F5) { RunProgram(); e.Handled = true; }
        else if (e.Key == Key.F6) { BuildProgram(); e.Handled = true; }
        else if (e.Key == Key.Escape) { _runner.Stop(); e.Handled = true; }
        base.OnPreviewKeyDown(e);
    }

    protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
    {
        foreach (var doc in _docs.ToList())
        {
            if (doc.IsDirty)
            {
                var r = MessageBox.Show(
                    $"Save changes to {doc.Title}?",
                    "JTS IDE",
                    MessageBoxButton.YesNoCancel,
                    MessageBoxImage.Question);
                if (r == MessageBoxResult.Cancel) { e.Cancel = true; return; }
                if (r == MessageBoxResult.Yes && !EnsureSaved(doc)) { e.Cancel = true; return; }
            }
        }
        _repl?.Stop();
        base.OnClosing(e);
    }
}

