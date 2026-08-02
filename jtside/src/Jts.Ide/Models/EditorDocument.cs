using System.IO;
using System.Windows.Controls;
using ICSharpCode.AvalonEdit;

namespace JtsIde.Models;

public class EditorDocument
{
    public required TabItem Tab { get; init; }
    public required TextEditor Editor { get; init; }
    public string? FilePath { get; set; }
    public bool IsDirty { get; set; }

    public string Title => FilePath is null ? "untitled" : Path.GetFileName(FilePath);

    public string TabHeader => (IsDirty ? "* " : "") + Title;
}

public class ErrorItem
{
    public ErrorItem(int line, string kind, string message)
    {
        Line = line;
        Kind = kind;
        Message = message;
    }

    public int Line { get; }
    public string Kind { get; }
    public string Message { get; }
}
