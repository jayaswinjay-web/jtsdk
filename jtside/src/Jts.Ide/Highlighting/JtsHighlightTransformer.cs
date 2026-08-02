using System.Windows;
using System.Windows.Media;
using ICSharpCode.AvalonEdit.Rendering;
using Jts.Core;

namespace JtsIde.Highlighting;

/// Colors JTS GO source lines using the F# tokenizer.
/// Each line renders as a single element (no word wrap), so whole-line
/// token matches map cleanly onto the element's text runs.
public class JtsHighlightTransformer : IVisualLineTransformer
{
    private static readonly Brush Keyword = Brushes("#569CD6");   // blue
    private static readonly Brush String  = Brushes("#CE9178");   // orange
    private static readonly Brush Number  = Brushes("#B5CEA8");   // light green
    private static readonly Brush Comment = Brushes("#6A9955");   // green
    private static readonly Brush Operator = Brushes("#D4D4D4");  // default text
    private static readonly Brush Default = Brushes("#D4D4D4");   // near-white

    private static readonly Typeface CommentFace =
        new(new FontFamily("Consolas"), FontStyles.Italic, FontWeights.Normal, FontStretches.Normal);

    private static Brush Brushes(string hex) =>
        new SolidColorBrush((Color)ColorConverter.ConvertFromString(hex)!);

    public void Transform(ITextRunConstructionContext context, IList<VisualLineElement> elements)
    {
        if (elements.Count == 0) return;

        var line = context.VisualLine.FirstDocumentLine;
        var text = context.GetText(line.Offset, line.Length).Text;
        var tokens = Tokenizer.tokenize(text);

        foreach (var element in elements)
        {
            if (element.DocumentLength == 0) continue;

            int start = element.RelativeTextOffset;
            int end = start + element.DocumentLength;

            foreach (var tok in tokens)
            {
                if (tok.Start >= end) break;
                if (tok.Start + tok.Length <= start) continue;
                if (tok.Start == start && tok.Start + tok.Length == end)
                {
                    Apply(element, tok.Kind);
                }
            }
        }
    }

    private static void Apply(VisualLineElement element, Tokenizer.TokenKind kind)
    {
        if (kind == Tokenizer.TokenKind.Keyword)
        {
            element.TextRunProperties.SetForegroundBrush(Keyword);
        }
        else if (kind == Tokenizer.TokenKind.String)
        {
            element.TextRunProperties.SetForegroundBrush(String);
        }
        else if (kind == Tokenizer.TokenKind.Number)
        {
            element.TextRunProperties.SetForegroundBrush(Number);
        }
        else if (kind == Tokenizer.TokenKind.Comment)
        {
            element.TextRunProperties.SetForegroundBrush(Comment);
            element.TextRunProperties.SetTypeface(CommentFace);
        }
        else if (kind == Tokenizer.TokenKind.Operator)
        {
            element.TextRunProperties.SetForegroundBrush(Operator);
        }
        else
        {
            element.TextRunProperties.SetForegroundBrush(Default);
        }
    }
}
