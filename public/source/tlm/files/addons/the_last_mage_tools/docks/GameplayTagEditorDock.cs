#if TOOLS
#nullable enable
using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Tags;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Validation;

namespace TheLastMage.EditorTools;

[Tool]
public partial class GameplayTagEditorDock : VBoxContainer
{
    private const string CatalogPath = GameplayTagCatalogUtility.DefaultCatalogPath;
    private const double AutoSaveSeconds = 1.0;
    private static readonly Color SelectedRowColor = new(0.22f, 0.34f, 0.52f, 0.62f);
    private static readonly Color DeprecatedColor = new(0.95f, 0.61f, 0.25f);
    private static readonly Color DangerColor = new(0.68f, 0.26f, 0.22f);
    private static readonly Color MutedColor = new(0.55f, 0.55f, 0.55f);

    private Tree? _tree;
    private LineEdit? _search;
    private LineEdit? _path;
    private TextEdit? _description;
    private CheckBox? _deprecated;
    private RichTextLabel? _usage;
    private Label? _selectedTitle;
    private Label? _selectedMeta;
    private Label? _status;
    private Godot.Timer? _autoSaveTimer;
    private GameplayTagCatalog _catalog = new();
    private string _selectedPath = string.Empty;
    private bool _loadingFields;
    private bool _savingFields;
    private bool _hasPendingChanges;
    private bool _autoSaveQueued;
    private bool _closeFlushAttempted;
    private readonly Dictionary<string, TagUsage> _usageByPath = new(StringComparer.OrdinalIgnoreCase);

    public override void _Ready()
    {
        Name = "TLMGameplayTagEditorDockContent";
        BuildUi();
        _autoSaveTimer = new Godot.Timer
        {
            OneShot = true,
            WaitTime = AutoSaveSeconds
        };
        _autoSaveTimer.Timeout += AutoSaveCurrentTag;
        AddChild(_autoSaveTimer);
        LoadCatalog();
        RefreshAll();
    }

    public override void _ExitTree()
    {
        TryFlushPendingChangesBeforeEditorClose(out _);
    }

    private void BuildUi()
    {
        SizeFlagsHorizontal = SizeFlags.ExpandFill;
        SizeFlagsVertical = SizeFlags.ExpandFill;
        AddThemeConstantOverride("separation", 8);

        var split = new HSplitContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            SplitOffsets = new[] { 520 }
        };
        AddChild(split);

        split.AddChild(BuildCatalogPane());
        split.AddChild(BuildDetailsPane());
    }

    private Control BuildCatalogPane()
    {
        var root = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            CustomMinimumSize = new Vector2(360f, 0f)
        };
        root.AddThemeConstantOverride("separation", 6);

        var title = new Label { Text = "Gameplay Tags" };
        title.AddThemeFontSizeOverride("font_size", 18);
        root.AddChild(title);

        _search = new LineEdit { PlaceholderText = "Search path or description", SizeFlagsHorizontal = SizeFlags.ExpandFill };
        _search.TextChanged += _ => RefreshTree();
        root.AddChild(_search);

        var toolbar = new HBoxContainer();
        toolbar.AddThemeConstantOverride("separation", 5);
        var expand = new Button { Text = "Expand" };
        expand.Pressed += () => SetCollapsedRecursive(false);
        toolbar.AddChild(expand);
        var collapse = new Button { Text = "Collapse" };
        collapse.Pressed += () => SetCollapsedRecursive(true);
        toolbar.AddChild(collapse);
        var import = new Button { Text = "Import Existing" };
        import.Pressed += ImportExistingTags;
        toolbar.AddChild(import);
        root.AddChild(toolbar);

        _tree = new Tree
        {
            HideRoot = true,
            Columns = 3,
            ColumnTitlesVisible = true,
            CustomMinimumSize = new Vector2(0f, 420f),
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            SelectMode = Tree.SelectModeEnum.Row,
            AllowReselect = true
        };
        _tree.SetColumnTitle(0, "Tag");
        _tree.SetColumnTitle(1, "Uses");
        _tree.SetColumnTitle(2, "Branch");
        _tree.SetColumnExpand(0, true);
        _tree.SetColumnCustomMinimumWidth(1, 58);
        _tree.SetColumnCustomMinimumWidth(2, 64);
        _tree.ItemSelected += LoadSelectedTagIntoFields;
        root.AddChild(_tree);

        _status = new Label
        {
            Text = "Ready.",
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        _status.AddThemeColorOverride("font_color", MutedColor);
        root.AddChild(_status);

        return root;
    }

    private Control BuildDetailsPane()
    {
        var root = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            CustomMinimumSize = new Vector2(460f, 0f)
        };
        root.AddThemeConstantOverride("separation", 8);

        _selectedTitle = new Label { Text = "No tag selected" };
        _selectedTitle.AddThemeFontSizeOverride("font_size", 18);
        root.AddChild(_selectedTitle);

        _selectedMeta = new Label
        {
            Text = "Select a tag or enter a new path.",
            AutowrapMode = TextServer.AutowrapMode.WordSmart
        };
        _selectedMeta.AddThemeColorOverride("font_color", MutedColor);
        root.AddChild(_selectedMeta);

        var grid = new GridContainer { Columns = 2 };
        grid.AddThemeConstantOverride("h_separation", 8);
        grid.AddThemeConstantOverride("v_separation", 6);
        root.AddChild(grid);
        _path = new LineEdit { PlaceholderText = "spell.element.fire", SizeFlagsHorizontal = SizeFlags.ExpandFill };
        _path.TextChanged += _ => OnFieldChanged();
        grid.AddChild(new Label { Text = "Path" });
        grid.AddChild(_path);
        _deprecated = new CheckBox { Text = "Deprecated" };
        _deprecated.Toggled += _ => OnFieldChanged();
        grid.AddChild(new Label { Text = "State" });
        grid.AddChild(_deprecated);
        _description = new TextEdit { CustomMinimumSize = new Vector2(0f, 70f), PlaceholderText = "Description or usage guidance" };
        _description.TextChanged += OnFieldChanged;
        grid.AddChild(new Label { Text = "Description" });
        grid.AddChild(_description);

        var actions = new HBoxContainer();
        actions.AddThemeConstantOverride("separation", 5);
        var addRoot = new Button { Text = "Add Root" };
        addRoot.Pressed += AddRootTag;
        actions.AddChild(addRoot);
        var addChild = new Button { Text = "Add Child" };
        addChild.Pressed += AddChildTag;
        actions.AddChild(addChild);
        var save = new Button { Text = "Save Now" };
        save.Pressed += SaveCurrentTag;
        actions.AddChild(save);
        root.AddChild(actions);

        var dangerActions = new HBoxContainer();
        dangerActions.AddThemeConstantOverride("separation", 5);
        var delete = new Button { Text = "Delete If Unused" };
        delete.AddThemeColorOverride("font_color", DangerColor);
        delete.Pressed += DeleteCurrentIfUnused;
        dangerActions.AddChild(delete);
        root.AddChild(dangerActions);

        var usageTitle = new Label { Text = "Usage" };
        usageTitle.AddThemeFontSizeOverride("font_size", 15);
        root.AddChild(usageTitle);

        _usage = new RichTextLabel
        {
            BbcodeEnabled = true,
            FitContent = true,
            ScrollActive = false,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        root.AddChild(_usage);

        return root;
    }

    private void LoadCatalog()
    {
        _catalog = ResourceLoader.Load<GameplayTagCatalog>(CatalogPath, cacheMode: ResourceLoader.CacheMode.Ignore) ?? new GameplayTagCatalog();
    }

    private void RefreshAll()
    {
        RefreshUsage();
        RefreshTree();
        RefreshUsageText();
    }

    private void RefreshTree()
    {
        if (_tree == null)
        {
            return;
        }

        var search = _search?.Text.Trim() ?? string.Empty;
        var visiblePaths = BuildVisiblePaths(search);
        var selectedPath = SelectedPath();
        _tree.Clear();
        var root = _tree.CreateItem();
        var byPath = new Dictionary<string, TreeItem>(StringComparer.OrdinalIgnoreCase);
        foreach (var tag in _catalog.OrderedTags())
        {
            var path = GameplayTagPath.Normalize(tag.Path);
            if (!string.IsNullOrWhiteSpace(search) && !visiblePaths.Contains(path))
            {
                continue;
            }

            var parentPath = GameplayTagPath.GetParent(path);
            var parent = !string.IsNullOrWhiteSpace(parentPath) && byPath.TryGetValue(parentPath, out var parentItem)
                ? parentItem
                : root;
            var item = _tree.CreateItem(parent);
            var directCount = DirectUsageCount(path);
            var branchCount = BranchUsageCount(path);
            item.SetText(0, path);
            item.SetText(1, directCount.ToString());
            item.SetText(2, branchCount.ToString());
            item.SetTooltipText(0, path);
            item.SetMetadata(0, path);
            if (tag.IsDeprecated)
            {
                item.SetCustomColor(0, DeprecatedColor);
            }

            if (string.Equals(path, selectedPath, StringComparison.OrdinalIgnoreCase))
            {
                item.SetCustomBgColor(0, SelectedRowColor);
                item.SetCustomBgColor(1, SelectedRowColor);
                item.SetCustomBgColor(2, SelectedRowColor);
                item.Select(0);
            }

            byPath[path] = item;
        }

        UpdateSelectedHeader();
    }

    private void LoadSelectedTagIntoFields()
    {
        var path = GameplayTagPath.Normalize(_tree?.GetSelected()?.GetMetadata(0).AsString());
        if (string.IsNullOrWhiteSpace(path))
        {
            return;
        }

        if (HasPendingChanges() && !string.Equals(_selectedPath, path, StringComparison.OrdinalIgnoreCase))
        {
            if (!TryFlushPendingChanges("Selection-change save", refreshUi: false, out _))
            {
                return;
            }
        }

        _selectedPath = path;
        var tag = _catalog.Find(path);
        if (tag == null)
        {
            return;
        }

        _loadingFields = true;
        try
        {
            if (_path != null)
            {
                _path.Text = GameplayTagPath.Normalize(tag.Path);
            }

            if (_description != null)
            {
                _description.Text = tag.Description;
            }

            if (_deprecated != null)
            {
                _deprecated.ButtonPressed = tag.IsDeprecated;
            }
        }
        finally
        {
            _loadingFields = false;
        }

        UpdateSelectedHeader();
        RefreshUsageText();
    }

    private void AddRootTag()
    {
        var path = GameplayTagPath.Normalize(_path?.Text ?? string.Empty);
        if (string.IsNullOrWhiteSpace(path) || path.Contains('.', StringComparison.Ordinal))
        {
            SetUsageText("Enter a root tag path without dots.");
            return;
        }

        EnsureAndSave(path);
    }

    private void AddChildTag()
    {
        var selected = SelectedPath();
        var child = GameplayTagPath.Normalize(_path?.Text ?? string.Empty);
        if (string.IsNullOrWhiteSpace(selected) || string.IsNullOrWhiteSpace(child))
        {
            SetUsageText("Select a parent tag and enter a child leaf/path.");
            return;
        }

        var path = child.StartsWith(selected + ".", StringComparison.OrdinalIgnoreCase)
            ? child
            : $"{selected}.{child}";
        EnsureAndSave(path);
    }

    private void SaveCurrentTag()
    {
        _autoSaveTimer?.Stop();
        _autoSaveQueued = false;
        SaveCurrentTagCore("Save", refreshUi: true, out _);
    }

    private bool SaveCurrentTagCore(string label, bool refreshUi, out string message)
    {
        message = string.Empty;
        var selected = _selectedPath;
        var next = GameplayTagPath.Normalize(_path?.Text ?? string.Empty);
        if (!GameplayTagPath.IsValid(next))
        {
            message = $"{label} blocked: invalid gameplay tag path.";
            SetUsageText("Invalid gameplay tag path.");
            SetStatus(message, true);
            return false;
        }

        var tag = _catalog.Find(selected);
        if (tag == null)
        {
            var created = EnsureAndSave(next);
            message = created ? $"{label} added '{next}'." : $"{label} failed.";
            return created;
        }

        var old = GameplayTagPath.Normalize(tag.Path);
        if (!string.Equals(old, next, StringComparison.OrdinalIgnoreCase))
        {
            RenameTagReferences(old, next);
            tag.Path = next;
            foreach (var parent in GameplayTagPath.GetAncestors(next))
            {
                _catalog.EnsureTag(parent);
            }
        }

        tag.Description = _description?.Text ?? tag.Description;
        tag.IsDeprecated = _deprecated?.ButtonPressed ?? tag.IsDeprecated;
        if (!SaveCatalog())
        {
            message = $"{label} failed validation.";
            return false;
        }

        _selectedPath = next;
        _hasPendingChanges = false;
        _autoSaveQueued = false;
        _closeFlushAttempted = false;
        message = string.Equals(old, next, StringComparison.OrdinalIgnoreCase)
            ? $"{label} saved '{next}'."
            : $"{label} renamed '{old}' to '{next}' and migrated direct content references.";
        SetStatus(message);
        if (refreshUi)
        {
            RefreshAll();
        }

        return true;
    }

    private void DeleteCurrentIfUnused()
    {
        var path = SelectedPath();
        if (string.IsNullOrWhiteSpace(path))
        {
            return;
        }

        if (_usageByPath.TryGetValue(path, out var usage) && usage.Count > 0)
        {
            var tag = _catalog.Find(path);
            if (tag != null)
            {
                tag.IsDeprecated = true;
                if (!SaveCatalog())
                {
                    return;
                }
            }

            SetUsageText($"Tag '{path}' is used {usage.Count} time(s), so it was marked deprecated instead of deleted.");
            SetStatus($"'{path}' is still used and was marked deprecated instead of deleted.", true);
            RefreshAll();
            return;
        }

        var branchUsageCount = BranchUsageCount(path);
        if (branchUsageCount > 0 || HasChildTags(path))
        {
            var tag = _catalog.Find(path);
            if (tag != null)
            {
                tag.IsDeprecated = true;
                if (!SaveCatalog())
                {
                    return;
                }
            }

            var reason = branchUsageCount > 0
                ? $"its child tags have {branchUsageCount} active usage(s)"
                : "it still has child tags";
            SetUsageText($"Tag '{path}' was marked deprecated instead of deleted because {reason}.");
            SetStatus($"'{path}' is a branch tag and was marked deprecated instead of deleted.", true);
            RefreshAll();
            return;
        }

        for (var i = _catalog.Tags.Count - 1; i >= 0; i--)
        {
            if (string.Equals(GameplayTagPath.Normalize(_catalog.Tags[i].Path), path, StringComparison.OrdinalIgnoreCase))
            {
                _catalog.Tags.RemoveAt(i);
            }
        }

        if (!SaveCatalog())
        {
            return;
        }

        _selectedPath = string.Empty;
        SetStatus($"Deleted unused tag '{path}'.");
        RefreshAll();
    }

    private void ImportExistingTags()
    {
        var content = ResourceLoader.Load<ContentCatalog>("res://data/catalogs/content_catalog.tres", cacheMode: ResourceLoader.CacheMode.Ignore);
        if (content == null)
        {
            SetUsageText("Could not load content catalog.");
            return;
        }

        GameplayTagCatalogUtility.BuildImportedCatalog(content, _catalog);
        if (!SaveCatalog())
        {
            return;
        }

        SetStatus("Imported existing authored tags into the catalog.");
        RefreshAll();
    }

    private bool EnsureAndSave(string path)
    {
        _catalog.EnsureTag(path, _description?.Text ?? string.Empty);
        SaveCurrentTagFields(path);
        if (!SaveCatalog())
        {
            return false;
        }

        _selectedPath = path;
        _hasPendingChanges = false;
        _autoSaveQueued = false;
        SetStatus($"Saved '{path}'.");
        RefreshAll();
        return true;
    }

    private void SaveCurrentTagFields(string path)
    {
        var tag = _catalog.Find(path);
        if (tag == null)
        {
            return;
        }

        tag.Description = _description?.Text ?? tag.Description;
        tag.IsDeprecated = _deprecated?.ButtonPressed ?? tag.IsDeprecated;
    }

    private bool SaveCatalog()
    {
        var validation = GameplayTagCatalogUtility.ValidateCatalog(_catalog);
        if (validation.HasErrors)
        {
            SetUsageText(string.Join('\n', validation.Issues.Select(issue => $"{issue.Severity}: {issue.Message}")));
            SetStatus("Catalog save failed validation.", true);
            return false;
        }

        ResourceSaver.Save(_catalog, CatalogPath);
        EditorInterface.Singleton.GetResourceFilesystem()?.Scan();
        return true;
    }

    public bool TryFlushPendingChangesBeforePlay(out string message)
    {
        return TryFlushPendingChanges("Pre-play save", refreshUi: true, out message);
    }

    public bool TryFlushPendingChangesBeforeEditorClose(out string message)
    {
        if (_closeFlushAttempted && HasPendingChanges())
        {
            GD.PushWarning("TLM Tags: retrying editor-close save while gameplay tag editor still has pending changes.");
        }

        _closeFlushAttempted = true;
        return TryFlushPendingChanges("Editor-close save", refreshUi: false, out message);
    }

    public bool HasPendingChanges()
    {
        return _hasPendingChanges || _autoSaveQueued || _autoSaveTimer?.IsStopped() == false;
    }

    private bool TryFlushPendingChanges(string label, bool refreshUi, out string message)
    {
        message = string.Empty;
        if (!HasPendingChanges())
        {
            return true;
        }

        _autoSaveTimer?.Stop();
        _autoSaveQueued = false;
        var result = SaveCurrentTagCore(label, refreshUi, out message);
        if (result && refreshUi)
        {
            EditorInterface.Singleton.GetResourceFilesystem()?.Scan();
        }

        return result;
    }

    private void OnFieldChanged()
    {
        if (_loadingFields || _savingFields)
        {
            return;
        }

        UpdateSelectedHeaderFromFields();
        ScheduleAutoSave();
    }

    private void ScheduleAutoSave()
    {
        if (_autoSaveTimer == null)
        {
            return;
        }

        _hasPendingChanges = true;
        _closeFlushAttempted = false;
        SetStatus("Pending tag edit. Auto-save queued.");
        if (_autoSaveQueued)
        {
            _autoSaveTimer.Start();
            return;
        }

        _autoSaveQueued = true;
        _autoSaveTimer.Start();
    }

    private void AutoSaveCurrentTag()
    {
        _autoSaveQueued = false;
        if (!HasPendingChanges())
        {
            return;
        }

        _savingFields = true;
        try
        {
            if (!SaveCurrentTagCore("Auto-save", refreshUi: true, out var message))
            {
                SetStatus(string.IsNullOrWhiteSpace(message) ? "Auto-save failed." : message, true);
            }
        }
        finally
        {
            _savingFields = false;
        }
    }

    private void RefreshUsage()
    {
        _usageByPath.Clear();
        var catalog = ResourceLoader.Load<ContentCatalog>("res://data/catalogs/content_catalog.tres", cacheMode: ResourceLoader.CacheMode.Ignore);
        if (catalog == null)
        {
            return;
        }

        RecordContentUsage("mage", catalog.Mages.Cast<ContentDefinition>());
        RecordContentUsage("spell", catalog.Spells);
        RecordContentUsage("item", catalog.Items);
        RecordContentUsage("enemy", catalog.Enemies);
        RecordContentUsage("wave", catalog.Waves);
        RecordContentUsage("defense", catalog.Defenses);
        RecordContentUsage("achievement", catalog.Achievements);
        foreach (var item in catalog.Items)
        {
            foreach (var effect in item.Effects)
            {
                RecordUsage(effect.TargetTag, $"item:{item.Id}:effect_target");
                foreach (var compiled in ItemEffectCompiler.Compile(effect))
                {
                    RecordUsage(compiled.TargetTag, $"item:{item.Id}:compiled_target");
                }
            }
        }
    }

    private void RecordContentUsage(string kind, IEnumerable<ContentDefinition> definitions)
    {
        foreach (var definition in definitions)
        {
            foreach (var tag in definition.Tags)
            {
                RecordUsage(tag, $"{kind}:{definition.Id}");
            }
        }
    }

    private void RecordUsage(string tag, string location)
    {
        var normalized = GameplayTagPath.Normalize(tag);
        if (!GameplayTagPath.IsValid(normalized))
        {
            return;
        }

        if (!_usageByPath.TryGetValue(normalized, out var usage))
        {
            usage = new TagUsage();
            _usageByPath[normalized] = usage;
        }

        usage.Locations.Add(location);
    }

    private void RenameTagReferences(string oldPath, string newPath)
    {
        var catalog = ResourceLoader.Load<ContentCatalog>("res://data/catalogs/content_catalog.tres", cacheMode: ResourceLoader.CacheMode.Ignore);
        if (catalog == null)
        {
            return;
        }

        foreach (var definition in catalog.Mages.Cast<ContentDefinition>()
                     .Concat(catalog.Spells)
                     .Concat(catalog.Items)
                     .Concat(catalog.Enemies)
                     .Concat(catalog.Waves)
                     .Concat(catalog.Defenses)
                     .Concat(catalog.Achievements))
        {
            var changed = false;
            for (var i = 0; i < definition.Tags.Count; i++)
            {
                if (string.Equals(GameplayTagPath.Normalize(definition.Tags[i]), oldPath, StringComparison.OrdinalIgnoreCase))
                {
                    definition.Tags[i] = newPath;
                    changed = true;
                }
            }

            if (changed)
            {
                ResourceSaver.Save(definition, definition.ResourcePath);
            }
        }

        foreach (var item in catalog.Items)
        {
            var changed = false;
            foreach (var effect in item.Effects)
            {
                if (string.Equals(GameplayTagPath.Normalize(effect.TargetTag), oldPath, StringComparison.OrdinalIgnoreCase))
                {
                    effect.TargetTag = newPath;
                    changed = true;
                }
            }

            if (changed)
            {
                ResourceSaver.Save(item, item.ResourcePath);
            }
        }
    }

    private void RefreshUsageText()
    {
        var path = SelectedPath();
        if (string.IsNullOrWhiteSpace(path))
        {
            SetUsageText("Select a tag to inspect usage.");
            UpdateSelectedHeader();
            return;
        }

        var branch = BranchUsageCount(path);
        if (!_usageByPath.TryGetValue(path, out var usage) || usage.Count == 0)
        {
            if (branch > 0)
            {
                SetUsageText($"[b]{Escape(path)}[/b]\nDirect usages: 0\nBranch usages: {branch}\n\nThis parent tag is not used directly, but child tags are used.");
                return;
            }

            SetUsageText($"[b]{Escape(path)}[/b]\nNo active usages.");
            return;
        }

        var header = branch == usage.Count
            ? $"[b]{Escape(path)}[/b]\nDirect usages: {usage.Count}"
            : $"[b]{Escape(path)}[/b]\nDirect usages: {usage.Count}\nBranch usages: {branch}";
        SetUsageText(header + "\n\n" + string.Join('\n', usage.Locations.Take(60).Select(Escape)));
    }

    private string SelectedPath()
    {
        var selected = _tree?.GetSelected();
        if (selected != null)
        {
            _selectedPath = GameplayTagPath.Normalize(selected.GetMetadata(0).AsString());
        }

        return _selectedPath;
    }

    private void SetCollapsedRecursive(bool collapsed)
    {
        if (_tree?.GetRoot() == null)
        {
            return;
        }

        SetCollapsedRecursive(_tree.GetRoot(), collapsed);
    }

    private static void SetCollapsedRecursive(TreeItem item, bool collapsed)
    {
        item.Collapsed = collapsed;
        var child = item.GetFirstChild();
        while (child != null)
        {
            SetCollapsedRecursive(child, collapsed);
            child = child.GetNext();
        }
    }

    private void SetUsageText(string text)
    {
        if (_usage != null)
        {
            _usage.Text = text;
        }
    }

    private void SetStatus(string text, bool warning = false)
    {
        if (_status == null)
        {
            return;
        }

        _status.Text = text;
        _status.AddThemeColorOverride("font_color", warning ? DeprecatedColor : MutedColor);
    }

    private void UpdateSelectedHeader()
    {
        var path = SelectedPath();
        var tag = _catalog.Find(path);
        if (_selectedTitle == null || _selectedMeta == null)
        {
            return;
        }

        if (tag == null)
        {
            _selectedTitle.Text = "No tag selected";
            _selectedMeta.Text = "Select a tag or enter a new path.";
            return;
        }

        var direct = DirectUsageCount(path);
        var branch = BranchUsageCount(path);
        _selectedTitle.Text = path;
        _selectedMeta.Text = tag.IsDeprecated
            ? $"{path} | deprecated | {direct} direct / {branch} branch usages"
            : $"{path} | {direct} direct / {branch} branch usages";
    }

    private void UpdateSelectedHeaderFromFields()
    {
        if (_selectedTitle == null || _selectedMeta == null)
        {
            return;
        }

        var path = GameplayTagPath.Normalize(_path?.Text ?? string.Empty);
        if (string.IsNullOrWhiteSpace(path))
        {
            _selectedTitle.Text = "No tag selected";
            _selectedMeta.Text = "Enter a valid tag path.";
            return;
        }

        var direct = DirectUsageCount(SelectedPath());
        var branch = BranchUsageCount(SelectedPath());
        _selectedTitle.Text = path;
        _selectedMeta.Text = _deprecated?.ButtonPressed == true
            ? $"{path} | deprecated | pending save | {direct} direct / {branch} branch usages"
            : $"{path} | pending save | {direct} direct / {branch} branch usages";
    }

    private HashSet<string> BuildVisiblePaths(string search)
    {
        var visible = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        if (string.IsNullOrWhiteSpace(search))
        {
            return visible;
        }

        foreach (var tag in _catalog.OrderedTags())
        {
            var path = GameplayTagPath.Normalize(tag.Path);
            if (!path.Contains(search, StringComparison.OrdinalIgnoreCase)
                && !tag.Description.Contains(search, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            foreach (var ancestor in GameplayTagPath.GetSelfAndAncestors(path))
            {
                visible.Add(ancestor);
            }
        }

        return visible;
    }

    private int DirectUsageCount(string path)
    {
        return _usageByPath.TryGetValue(path, out var usage) ? usage.Count : 0;
    }

    private int BranchUsageCount(string path)
    {
        var normalized = GameplayTagPath.Normalize(path);
        var prefix = normalized + ".";
        var count = 0;
        foreach (var pair in _usageByPath)
        {
            if (string.Equals(pair.Key, normalized, StringComparison.OrdinalIgnoreCase)
                || pair.Key.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            {
                count += pair.Value.Count;
            }
        }

        return count;
    }

    private bool HasChildTags(string path)
    {
        var normalized = GameplayTagPath.Normalize(path);
        var prefix = normalized + ".";
        return _catalog.OrderedTags()
            .Any(tag => GameplayTagPath.Normalize(tag.Path).StartsWith(prefix, StringComparison.OrdinalIgnoreCase));
    }

    private static string Escape(string value)
    {
        return value.Replace("[", "\\[", StringComparison.Ordinal);
    }

    private sealed class TagUsage
    {
        public List<string> Locations { get; } = new();
        public int Count => Locations.Count;
    }
}
#endif
