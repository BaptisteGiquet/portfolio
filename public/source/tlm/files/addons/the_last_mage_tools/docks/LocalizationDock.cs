#if TOOLS
#nullable enable
using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Localization;

namespace TheLastMage.EditorTools;

[Tool]
public partial class LocalizationDock : VBoxContainer
{
    private const string CatalogPath = "res://data/catalogs/content_catalog.tres";

    private readonly LineEdit _csvPath = new()
    {
        Text = LocalizationSpreadsheet.DefaultProjectPath,
        SizeFlagsHorizontal = SizeFlags.ExpandFill
    };

    private readonly LineEdit _previewLocale = new()
    {
        Text = LocalizationService.DefaultLocale,
        SizeFlagsHorizontal = SizeFlags.ExpandFill
    };

    private readonly RichTextLabel _status = new()
    {
        BbcodeEnabled = false,
        FitContent = false,
        ScrollActive = true,
        SelectionEnabled = true,
        CustomMinimumSize = new Vector2(0f, 260f),
        SizeFlagsHorizontal = SizeFlags.ExpandFill,
        SizeFlagsVertical = SizeFlags.ExpandFill
    };

    public LocalizationDock()
    {
        Name = "TLMLocalizationDockContent";
        SizeFlagsHorizontal = SizeFlags.ExpandFill;
        SizeFlagsVertical = SizeFlags.ExpandFill;
        AddThemeConstantOverride("separation", 8);
        BuildUi();
    }

    private void BuildUi()
    {
        AddChild(new Label { Text = "Localization" });

        AddChild(new Label { Text = "Spreadsheet CSV path" });
        AddChild(_csvPath);

        var buttons = new HBoxContainer();
        buttons.AddThemeConstantOverride("separation", 6);
        AddChild(buttons);

        var export = new Button { Text = "Export English Source" };
        export.Pressed += ExportSource;
        buttons.AddChild(export);

        var import = new Button { Text = "Import / Normalize CSV" };
        import.Pressed += ImportCsv;
        buttons.AddChild(import);

        var validate = new Button { Text = "Validate" };
        validate.Pressed += ValidateCsv;
        buttons.AddChild(validate);

        AddChild(new Label { Text = "Preview locale" });
        var previewRow = new HBoxContainer();
        previewRow.AddThemeConstantOverride("separation", 6);
        AddChild(previewRow);
        previewRow.AddChild(_previewLocale);
        var applyPreview = new Button { Text = "Apply Preview Locale" };
        applyPreview.Pressed += ApplyPreviewLocale;
        previewRow.AddChild(applyPreview);

        _status.Text = "Ready.\nExport creates a spreadsheet-compatible CSV with columns: key, context, en, notes, then any locale columns such as fr or de.";
        AddChild(_status);
    }

    private void ExportSource()
    {
        var catalog = LoadCatalog();
        if (catalog == null)
        {
            return;
        }

        var spreadsheet = LocalizationSpreadsheet.ExportSource(catalog, SpreadsheetPath);
        SetStatus($"Exported {spreadsheet.Rows.Count} source rows to {SpreadsheetPath}.\nAdd locale columns such as fr, de, es, then use Import / Normalize CSV.");
    }

    private void ImportCsv()
    {
        var catalog = LoadCatalog();
        if (catalog == null)
        {
            return;
        }

        var spreadsheet = LocalizationSpreadsheet.ImportAndNormalize(catalog, SpreadsheetPath);
        var validation = spreadsheet.ValidateAgainst(catalog);
        LocalizationService.Current.LoadEnglish(catalog, LocalizationService.Current.CurrentLocale, SpreadsheetPath);
        SetStatus(
            $"Imported {spreadsheet.Rows.Count} rows from {SpreadsheetPath}.\n" +
            $"Locales: {FormatLocales(spreadsheet)}\n\n" +
            FormatValidation(validation));
    }

    private void ValidateCsv()
    {
        var catalog = LoadCatalog();
        if (catalog == null)
        {
            return;
        }

        var spreadsheet = LocalizationSpreadsheet.Load(SpreadsheetPath);
        var validation = spreadsheet.ValidateAgainst(catalog);
        SetStatus(
            $"Validated {SpreadsheetPath}.\n" +
            $"Rows: {spreadsheet.Rows.Count}\n" +
            $"Locales: {FormatLocales(spreadsheet)}\n\n" +
            FormatValidation(validation));
    }

    private void ApplyPreviewLocale()
    {
        var catalog = LoadCatalog();
        if (catalog == null)
        {
            return;
        }

        var locale = LocalizationSpreadsheet.NormalizeLocale(_previewLocale.Text);
        LocalizationService.Current.LoadEnglish(catalog, locale, SpreadsheetPath);
        SetStatus($"Preview locale set to '{LocalizationService.Current.CurrentLocale}'. Restarting a run or reopening UI panels will show translated text where available.");
    }

    private ContentCatalog? LoadCatalog()
    {
        var catalog = ResourceLoader.Load<ContentCatalog>(CatalogPath);
        if (catalog != null)
        {
            return catalog;
        }

        SetStatus($"Could not load {CatalogPath}.");
        return null;
    }

    private string SpreadsheetPath => string.IsNullOrWhiteSpace(_csvPath.Text)
        ? LocalizationSpreadsheet.DefaultProjectPath
        : _csvPath.Text.Trim();

    private static string FormatLocales(LocalizationSpreadsheet spreadsheet)
    {
        return spreadsheet.LocaleColumns.Count == 0
            ? "none yet"
            : string.Join(", ", spreadsheet.LocaleColumns);
    }

    private static string FormatValidation(Foundation.Validation.ValidationResult validation)
    {
        if (validation.Issues.Count == 0)
        {
            return "Validation passed with 0 issues.";
        }

        return string.Join('\n', validation.Issues.Select(issue => $"{issue.Severity} {issue.Code}: {issue.Message}"));
    }

    private void SetStatus(string text)
    {
        _status.Text = text;
        GD.Print($"TLM Localization: {text}");
    }
}
#endif
