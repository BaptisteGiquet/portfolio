#if TOOLS
#nullable enable
using System.Globalization;
using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Tags;
using TheLastMage.Data.Validation;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Validation;

namespace TheLastMage.EditorTools;

[Tool]
public partial class ItemAuthoringDock : VBoxContainer
{
    private const string ItemDirectory = "res://data/items";
    private const string CatalogPath = "res://data/catalogs/content_catalog.tres";
    private const string ForensicLogPath = "res://Saved/ItemAuthoringForensics/tlm_item_authoring_forensics.log";
    private const string CrashBreadcrumbPath = "res://Saved/ItemAuthoringForensics/tlm_item_authoring_crash_breadcrumb.log";
    private const string LastSaveReportPath = "res://Saved/ItemAuthoringForensics/tlm_item_authoring_last_save_report.log";
    private const string SaveReportDirectory = "res://Saved/ItemAuthoringForensics/save_reports";
    private const int RequiredIconSize = 256;
    private const int ColumnResizeGrabPixels = 6;
    private const int MaxStartupRefreshAttempts = 8;
    private const string UnresolvedOptionSuffix = AuthoringDropdownOptions.UnresolvedSuffix;

    private static readonly Color DefinitionSectionColor = new(0.33f, 0.49f, 0.74f);
    private static readonly Color TagsSectionColor = new(0.27f, 0.59f, 0.42f);
    private static readonly Color PresentationSectionColor = new(0.68f, 0.49f, 0.22f);
    private static readonly Color EffectsSectionColor = new(0.62f, 0.38f, 0.70f);
    private static readonly Color ActiveEffectsSectionColor = new(0.75f, 0.36f, 0.34f);

    private static readonly string[] KnownStats =
    {
        "health",
        "move_speed",
        "damage",
        "fire_rate",
        "range",
        "luck",
        "duration",
        "radius",
        "projectile_speed",
        "status_power",
        "summon_damage",
        "defense_damage",
        "spell_count",
        "projectile_split",
        "homing_strength",
        "homing_angle_degrees",
        "homing_acquire_radius"
    };

    private readonly List<ItemAsset> _items = new();
    private readonly List<ItemAsset> _visibleItems = new();
    private readonly List<string> _deletedPathsThisSession = new();
    private readonly List<Control> _columnResizeHandles = new();
    private readonly Dictionary<SpreadsheetColumn, int> _columnWidths = new();
    private readonly HashSet<string> _dirtyItemPaths = new(StringComparer.OrdinalIgnoreCase);

    private TabContainer? _mainTabs;
    private Tree? _itemTable;
    private ItemList? _itemList;
    private LineEdit? _search;
    private CheckBox? _showIconsCheckbox;
    private Label? _status;
    private SpinBox? _itemNumber;
    private LineEdit? _id;
    private LineEdit? _name;
    private OptionButton? _itemKind;
    private VBoxContainer? _poolWeightsBox;
    private CheckBox? _unlimitedActivations;
    private SpinBox? _maxActivations;
    private LineEdit? _tags;
    private GameplayTagSelector? _itemTagSelector;
    private OptionButton? _unlockAchievement;
    private TextEdit? _hiddenDescription;
    private TextEdit? _description;
    private TextEdit? _revealedStatText;
    private TextEdit? _revealedBehaviorText;
    private TextureRect? _iconPreview;
    private LineEdit? _enemyFormScenePath;
    private VBoxContainer? _effectsBox;
    private VBoxContainer? _activeEffectsBox;
    private RichTextLabel? _cardPreview;
    private RichTextLabel? _statPreview;
    private RichTextLabel? _procPreview;
    private RichTextLabel? _lootPreview;
    private RichTextLabel? _synergyPreview;
    private RichTextLabel? _validationPreview;
    private RichTextLabel? _spellImpactHeader;
    private VBoxContainer? _spellImpactLeftColumn;
    private VBoxContainer? _spellImpactRightColumn;
    private Label? _telemetryLabel;
    private FileDialog? _iconDialog;
    private ConfirmationDialog? _deleteConfirm;
    private ItemAsset? _selected;
    private readonly List<AchievementOption> _achievementOptions = new();
    private bool _loadingFields;
    private bool _syncingSelection;
    private bool _showIcons = true;
    private int _resizingColumn = -1;
    private float _resizeStartX;
    private int _resizeStartWidth;
    private int _startupRefreshAttempts;
    private bool _suppressNextSort;
    private bool _deleteProbeRan;
    private bool _savingFields;
    private bool _hasPendingChanges;
    private bool _closeFlushAttempted;
    private bool _autoSaveQueued;
    private bool _stableIdEditDirty;
    private bool _committingStableIdEdit;
    private int _diagnosticEventIndex;
    private readonly string _diagnosticSessionId = DateTimeOffset.Now.ToString("yyyyMMdd_HHmmss_fff", CultureInfo.InvariantCulture);
    private SpreadsheetColumn _sortColumn = SpreadsheetColumn.Number;
    private bool _sortDescending;

    private enum SpreadsheetColumn
    {
        Icon,
        Number,
        StableId,
        Name,
        PoolWeights,
        NightMarketShare,
        Tags,
        UnlockAchievement,
        RevealedStats,
        RevealedBehavior,
        Telemetry,
        Path
    }

    private readonly record struct SpellPreviewStat(
        string Label,
        float BaseValue,
        float FinalValue,
        string Suffix,
        bool Integer,
        bool HideWhenZero);

    public override void _Ready()
    {
        Name = "TLMItemAuthoringDockContent";
        SizeFlagsHorizontal = SizeFlags.ExpandFill;
        SizeFlagsVertical = SizeFlags.ExpandFill;
        BuildUi();
        LogForensics("READY", $"session={_diagnosticSessionId} autoSave=disabled disk=[{BuildDiskItemSnapshot()}] catalog=[{BuildCatalogSnapshot()}]");
        SetStatus("Waiting for editor item resources...");
        Callable.From(RefreshItems).CallDeferred();
    }

    public override void _ExitTree()
    {
        LogForensics("EXIT_TREE_BEGIN", BuildDiagnosticState("exit_tree_begin"));
        TryFlushPendingChangesBeforeEditorClose(out _);
        LogForensics("EXIT_TREE", $"deletedThisSession=[{string.Join(", ", _deletedPathsThisSession)}] disk=[{BuildDiskItemSnapshot()}] catalog=[{BuildCatalogSnapshot()}] loaded=[{BuildLoadedItemSnapshot()}]");
    }

    public override bool _CanDropData(Vector2 atPosition, Variant data)
    {
        return TryExtractFirstImagePath(data, out _);
    }

    public override void _DropData(Vector2 atPosition, Variant data)
    {
        if (TryExtractFirstImagePath(data, out var path))
        {
            AssignIconFromPath(path);
        }
    }

    private void BuildUi()
    {
        _mainTabs = new TabContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        AddChild(_mainTabs);

        _mainTabs.AddChild(BuildSpreadsheetTab());
        _mainTabs.AddChild(BuildCreateEditTab());

        _iconDialog = new FileDialog
        {
            Access = FileDialog.AccessEnum.Filesystem,
            FileMode = FileDialog.FileModeEnum.OpenFile,
            Filters = new[] { "*.png,*.jpg,*.jpeg,*.webp;Images" },
            Title = "Choose Item Icon"
        };
        _iconDialog.FileSelected += AssignIconFromPath;
        AddChild(_iconDialog);

        _deleteConfirm = new ConfirmationDialog
        {
            Title = "Delete Item",
            DialogText = "Delete the selected item resource?"
        };
        _deleteConfirm.Confirmed += DeleteSelectedItem;
        AddChild(_deleteConfirm);
    }

    private Control BuildSpreadsheetTab()
    {
        var root = new VBoxContainer
        {
            Name = "Items",
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        root.AddThemeConstantOverride("separation", 8);

        var title = new Label { Text = "Item Database" };
        title.AddThemeFontSizeOverride("font_size", 18);
        root.AddChild(title);

        var tools = new HBoxContainer();
        tools.AddThemeConstantOverride("separation", 6);
        root.AddChild(tools);

        _search = new LineEdit
        {
            PlaceholderText = "Search name, id, or tag",
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        _search.TextChanged += _ => RefreshVisibleItems();
        tools.AddChild(_search);

        _showIconsCheckbox = new CheckBox { Text = "Icons", ButtonPressed = true };
        _showIconsCheckbox.Toggled += pressed =>
        {
            _showIcons = pressed;
            RefreshVisibleItemsPreservingSelection();
        };
        tools.AddChild(_showIconsCheckbox);

        var editSelected = new Button { Text = "Open Create / Edit" };
        editSelected.Pressed += () =>
        {
            if (_mainTabs != null)
            {
                _mainTabs.CurrentTab = 1;
            }
        };
        tools.AddChild(editSelected);

        var reload = new Button { Text = "Reload" };
        reload.Pressed += RefreshItems;
        tools.AddChild(reload);

        _itemTable = new Tree
        {
            Columns = Enum.GetValues<SpreadsheetColumn>().Length,
            ColumnTitlesVisible = true,
            HideRoot = true,
            SelectMode = Tree.SelectModeEnum.Row,
            AllowReselect = true,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        _itemTable.ItemSelected += SelectSpreadsheetRow;
        _itemTable.ColumnTitleClicked += OnSpreadsheetColumnClicked;
        _itemTable.GuiInput += OnItemTableGuiInput;
        _itemTable.Resized += PositionColumnResizeHandles;
        ConfigureSpreadsheetColumns();
        root.AddChild(_itemTable);
        BuildColumnResizeHandles();

        _status = new Label
        {
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            Text = "Ready."
        };
        root.AddChild(_status);

        return root;
    }

    private Control BuildCreateEditTab()
    {
        var detailSplit = new HSplitContainer
        {
            Name = "Create / Edit",
            SplitOffsets = new[] { 315 },
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };

        var left = new VBoxContainer
        {
            CustomMinimumSize = new Vector2(300f, 0f),
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        left.AddThemeConstantOverride("separation", 6);
        detailSplit.AddChild(left);

        var title = new Label { Text = "Create / Edit Item" };
        title.AddThemeFontSizeOverride("font_size", 18);
        left.AddChild(title);

        var actionGrid = new GridContainer { Columns = 2 };
        actionGrid.AddThemeConstantOverride("h_separation", 6);
        actionGrid.AddThemeConstantOverride("v_separation", 6);
        var newButton = new Button { Text = "New Blank" };
        newButton.Pressed += CreateBlankItem;
        var duplicateButton = new Button { Text = "Duplicate" };
        duplicateButton.Pressed += DuplicateSelectedItem;
        var save = new Button { Text = "Save Item" };
        save.Pressed += SaveSelectedItem;
        var delete = new Button { Text = "Delete Item" };
        delete.Pressed += RequestDeleteSelectedItem;
        actionGrid.AddChild(newButton);
        actionGrid.AddChild(duplicateButton);
        actionGrid.AddChild(save);
        actionGrid.AddChild(delete);
        left.AddChild(actionGrid);

        _itemList = new ItemList
        {
            CustomMinimumSize = new Vector2(0f, 190f),
            SizeFlagsVertical = SizeFlags.ExpandFill,
            AllowReselect = true
        };
        _itemList.ItemSelected += index => SelectVisibleItem((int)index);
        left.AddChild(_itemList);

        var scroll = new ScrollContainer
        {
            CustomMinimumSize = new Vector2(620f, 0f),
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        detailSplit.AddChild(scroll);

        var editorMargin = new MarginContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        editorMargin.AddThemeConstantOverride("margin_left", 10);
        editorMargin.AddThemeConstantOverride("margin_top", 8);
        editorMargin.AddThemeConstantOverride("margin_right", 10);
        editorMargin.AddThemeConstantOverride("margin_bottom", 8);
        scroll.AddChild(editorMargin);

        var editor = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        editor.AddThemeConstantOverride("separation", 10);
        editorMargin.AddChild(editor);
        BuildEditorFields(editor);

        return detailSplit;
    }

    private void BuildEditorFields(VBoxContainer editor)
    {
        editor.AddChild(SectionHeader("Definition", DefinitionSectionColor));

        _itemNumber = BuildSpin(1, 10000, 1);
        _id = BuildLine("stable_item_id", liveChanges: false);
        _id.TextChanged += _ => OnStableIdTextChanged();
        _id.TextSubmitted += _ => CommitStableIdEdit("submitted", markDirty: true);
        _id.FocusExited += () => CommitStableIdEdit("focus_exited", markDirty: true);
        _name = BuildLine("Display name");
        editor.AddChild(HorizontalFields(
            Labeled("Number", _itemNumber, 120f),
            Labeled("Stable ID", _id),
            Labeled("Name", _name)));

        _itemKind = new OptionButton();
        FillEnumOptions<ItemKind>(_itemKind);
        _itemKind.ItemSelected += _ =>
        {
            PullFieldsIntoSelected();
            RebuildActiveEffectRows();
            RefreshActivationControls();
            RefreshPreviews();
        };
        _unlockAchievement = new OptionButton();
        _unlockAchievement.ItemSelected += _ => OnFieldChanged();
        editor.AddChild(HorizontalFields(
            Labeled("Kind", _itemKind),
            Labeled("Unlock Achievement", _unlockAchievement)));

        var poolWeightsPanel = new VBoxContainer();
        poolWeightsPanel.AddThemeConstantOverride("separation", 4);
        _poolWeightsBox = new VBoxContainer();
        _poolWeightsBox.AddThemeConstantOverride("separation", 4);
        poolWeightsPanel.AddChild(_poolWeightsBox);
        var addPoolWeight = new Button { Text = "Add Pool" };
        addPoolWeight.Pressed += AddPoolWeight;
        poolWeightsPanel.AddChild(addPoolWeight);
        editor.AddChild(Labeled("Item Pool Weights", poolWeightsPanel));
        _unlimitedActivations = new CheckBox { Text = "Unlimited activations while equipped" };
        _unlimitedActivations.Toggled += _ =>
        {
            PullFieldsIntoSelected();
            RefreshActivationControls();
            RefreshPreviews();
        };
        _maxActivations = BuildSpin(0, 999, 1);
        editor.AddChild(HorizontalFields(
            Labeled("Activatable Uses", _unlimitedActivations),
            Labeled("Max Activations", _maxActivations, 160f)));

        editor.AddChild(SectionHeader("Tags", TagsSectionColor));
        var tagsPanel = new VBoxContainer();
        tagsPanel.AddThemeConstantOverride("separation", 4);
        _tags = BuildLine("comma,separated,tags");
        tagsPanel.AddChild(_tags);

        _itemTagSelector = new GameplayTagSelector
        {
            MultiSelect = false,
            IncludeClearOption = false,
            CustomMinimumSize = new Vector2(0f, 260f),
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        tagsPanel.AddChild(_itemTagSelector);
        var addTag = new Button { Text = "Add Selected Tag" };
        addTag.Pressed += AddSelectedTagToCurrentItem;
        tagsPanel.AddChild(addTag);
        editor.AddChild(Labeled("Item Tags", tagsPanel));

        editor.AddChild(SectionHeader("Presentation", PresentationSectionColor));
        var iconPanel = new HBoxContainer();
        _iconPreview = new TextureRect { CustomMinimumSize = new Vector2(96f, 96f) };
        iconPanel.AddChild(_iconPreview);
        var iconButtons = new VBoxContainer();
        var browse = new Button { Text = "Choose Icon" };
        browse.Pressed += () => _iconDialog?.PopupFileDialog();
        iconButtons.AddChild(browse);
        iconButtons.AddChild(new Label { Text = "Drop image here; saved texture is forced to 256x256." });
        iconPanel.AddChild(iconButtons);
        editor.AddChild(Labeled("Icon", iconPanel));

        _enemyFormScenePath = BuildLine("res://path/to/enemy_transform_scene.tscn");
        editor.AddChild(Labeled("Enemy Transform Scene", _enemyFormScenePath));

        _description = BuildText(74f);
        _hiddenDescription = BuildText(74f);
        editor.AddChild(HorizontalFields(
            Labeled("Designer Notes", _description),
            Labeled("Hidden Description", _hiddenDescription)));
        _revealedStatText = BuildText(72f);
        _revealedBehaviorText = BuildText(72f);
        editor.AddChild(HorizontalFields(
            Labeled("Revealed Stats", _revealedStatText),
            Labeled("Revealed Behavior", _revealedBehaviorText)));

        _telemetryLabel = new Label
        {
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            Text = "Players picked: 0 | Win rate: 0% | read-only telemetry"
        };
        editor.AddChild(Labeled("Telemetry", _telemetryLabel));

        var addEffect = new Button { Text = "Add Effect" };
        addEffect.Pressed += AddEffect;
        editor.AddChild(ActionSectionHeader("Effects", addEffect, EffectsSectionColor));

        var effectsRow = new HBoxContainer
        {
            CustomMinimumSize = new Vector2(0f, 520f),
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        effectsRow.AddThemeConstantOverride("separation", 28);

        var effectsColumn = new VBoxContainer
        {
            CustomMinimumSize = new Vector2(380f, 0f),
            SizeFlagsHorizontal = SizeFlags.ShrinkBegin
        };
        effectsColumn.AddThemeConstantOverride("separation", 6);

        _effectsBox = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        _effectsBox.AddThemeConstantOverride("separation", 6);
        effectsColumn.AddChild(_effectsBox);
        effectsRow.AddChild(effectsColumn);
        effectsRow.AddChild(BuildSpellImpactPreviewPanel());
        editor.AddChild(effectsRow);

        var addActiveEffect = new Button { Text = "Add Active Effect" };
        addActiveEffect.Pressed += AddActiveEffect;
        editor.AddChild(ActionSectionHeader("Active Effects", addActiveEffect, ActiveEffectsSectionColor));

        _activeEffectsBox = new VBoxContainer();
        _activeEffectsBox.AddThemeConstantOverride("separation", 6);
        editor.AddChild(_activeEffectsBox);
    }

    private Control BuildSpellImpactPreviewPanel()
    {
        var panel = new PanelContainer
        {
            CustomMinimumSize = new Vector2(680f, 500f),
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };

        var margin = new MarginContainer();
        margin.AddThemeConstantOverride("margin_left", 8);
        margin.AddThemeConstantOverride("margin_top", 8);
        margin.AddThemeConstantOverride("margin_right", 8);
        margin.AddThemeConstantOverride("margin_bottom", 8);
        panel.AddChild(margin);

        var box = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        box.AddThemeConstantOverride("separation", 6);
        margin.AddChild(box);

        var title = SectionTitle("Spell Impact", EffectsSectionColor);
        box.AddChild(title);

        _spellImpactHeader = new RichTextLabel
        {
            BbcodeEnabled = true,
            FitContent = true,
            ScrollActive = false,
            ContextMenuEnabled = true,
            SelectionEnabled = true,
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            Text = "Select an item."
        };
        _spellImpactHeader.AddThemeFontSizeOverride("font_size", 15);
        box.AddChild(_spellImpactHeader);

        var scroll = new ScrollContainer
        {
            CustomMinimumSize = new Vector2(0f, 390f),
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        box.AddChild(scroll);

        var columns = new HBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ShrinkBegin
        };
        columns.AddThemeConstantOverride("separation", 44);
        scroll.AddChild(columns);

        _spellImpactLeftColumn = BuildSpellImpactColumn();
        _spellImpactRightColumn = BuildSpellImpactColumn();
        columns.AddChild(_spellImpactLeftColumn);
        columns.AddChild(_spellImpactRightColumn);

        return panel;
    }

    private static VBoxContainer BuildSpellImpactColumn()
    {
        var column = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ShrinkBegin
        };
        column.AddThemeConstantOverride("separation", 12);
        return column;
    }

    private static RichTextLabel AddPreviewTab(TabContainer tabs, string title)
    {
        var label = new RichTextLabel
        {
            Name = title,
            BbcodeEnabled = true,
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            ContextMenuEnabled = true,
            SelectionEnabled = true,
            ScrollActive = true,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        label.AddThemeFontSizeOverride("font_size", 14);
        tabs.AddChild(label);
        return label;
    }

    private void ConfigureSpreadsheetColumns()
    {
        if (_itemTable == null)
        {
            return;
        }

        for (var i = 0; i < Enum.GetValues<SpreadsheetColumn>().Length; i++)
        {
            var column = (SpreadsheetColumn)i;
            _itemTable.SetColumnTitle(i, SpreadsheetTitle(column));
            _itemTable.SetColumnTitleTooltipText(i, "Click to sort ascending/descending.");
            _itemTable.SetColumnClipContent(i, true);
            _itemTable.SetColumnExpand(i, false);
            _itemTable.SetColumnCustomMinimumWidth(i, ColumnWidth(column));
        }
    }

    private int ColumnWidth(SpreadsheetColumn column)
    {
        return _columnWidths.TryGetValue(column, out var width) ? width : DefaultColumnWidth(column);
    }

    private int DefaultColumnWidth(SpreadsheetColumn column)
    {
        return column switch
        {
            SpreadsheetColumn.Icon => _showIcons ? 58 : 28,
            SpreadsheetColumn.Number => 70,
            SpreadsheetColumn.StableId => 170,
            SpreadsheetColumn.Name => 220,
            SpreadsheetColumn.PoolWeights => 210,
            SpreadsheetColumn.NightMarketShare => 130,
            SpreadsheetColumn.Tags => 220,
            SpreadsheetColumn.UnlockAchievement => 180,
            SpreadsheetColumn.RevealedStats => 300,
            SpreadsheetColumn.RevealedBehavior => 320,
            SpreadsheetColumn.Telemetry => 160,
            SpreadsheetColumn.Path => 280,
            _ => 120
        };
    }

    private void BuildColumnResizeHandles()
    {
        if (_itemTable == null)
        {
            return;
        }

        foreach (var handle in _columnResizeHandles)
        {
            handle.QueueFree();
        }

        _columnResizeHandles.Clear();
        var columnCount = Enum.GetValues<SpreadsheetColumn>().Length;
        for (var column = 0; column < columnCount - 1; column++)
        {
            var resizeColumn = column;
            var handle = new ColorRect
            {
                Color = new Color(1f, 1f, 1f, 0.01f),
                MouseFilter = MouseFilterEnum.Stop,
                MouseDefaultCursorShape = CursorShape.Hsize,
                ZIndex = 128
            };
            handle.GuiInput += inputEvent => OnColumnResizeHandleInput(resizeColumn, inputEvent);
            _itemTable.AddChild(handle);
            _columnResizeHandles.Add(handle);
        }

        PositionColumnResizeHandles();
    }

    private void PositionColumnResizeHandles()
    {
        if (_itemTable == null)
        {
            return;
        }

        var boundary = 0f;
        for (var column = 0; column < _columnResizeHandles.Count; column++)
        {
            boundary += _itemTable.GetColumnWidth(column);
            var handle = _columnResizeHandles[column];
            handle.Position = new Vector2(boundary - ColumnResizeGrabPixels, 0f);
            handle.Size = new Vector2(ColumnResizeGrabPixels * 2f, Math.Max(1f, _itemTable.Size.Y));
        }
    }

    private void OnColumnResizeHandleInput(int column, InputEvent inputEvent)
    {
        if (_itemTable == null)
        {
            return;
        }

        if (inputEvent is InputEventMouseButton button && button.ButtonIndex == MouseButton.Left)
        {
            if (button.Pressed)
            {
                _resizingColumn = column;
                _resizeStartX = button.GlobalPosition.X;
                _resizeStartWidth = _itemTable.GetColumnWidth(column);
                _suppressNextSort = true;
                _itemTable.AcceptEvent();
            }
            else if (_resizingColumn == column)
            {
                _resizingColumn = -1;
                _itemTable.AcceptEvent();
            }

            return;
        }

        if (inputEvent is not InputEventMouseMotion motion || _resizingColumn != column)
        {
            return;
        }

        ResizeSpreadsheetColumn(column, _resizeStartWidth + (int)(motion.GlobalPosition.X - _resizeStartX));
        _itemTable.AcceptEvent();
    }

    private void ResizeSpreadsheetColumn(int column, int width)
    {
        if (_itemTable == null)
        {
            return;
        }

        var spreadsheetColumn = (SpreadsheetColumn)Math.Clamp(column, 0, Enum.GetValues<SpreadsheetColumn>().Length - 1);
        var clampedWidth = Math.Max(24, width);
        _columnWidths[spreadsheetColumn] = clampedWidth;
        _itemTable.SetColumnCustomMinimumWidth(column, clampedWidth);
        PositionColumnResizeHandles();
    }

    private string SpreadsheetTitle(SpreadsheetColumn column)
    {
        var title = column switch
        {
            SpreadsheetColumn.Icon => "Icon",
            SpreadsheetColumn.Number => "#",
            SpreadsheetColumn.StableId => "Stable ID",
            SpreadsheetColumn.Name => "Name",
            SpreadsheetColumn.PoolWeights => "Pool Weights",
            SpreadsheetColumn.NightMarketShare => "Night Market Share",
            SpreadsheetColumn.Tags => "Tags",
            SpreadsheetColumn.UnlockAchievement => "Unlock",
            SpreadsheetColumn.RevealedStats => "Stats",
            SpreadsheetColumn.RevealedBehavior => "Behavior",
            SpreadsheetColumn.Telemetry => "Telemetry",
            SpreadsheetColumn.Path => "Path",
            _ => column.ToString()
        };

        if (column != _sortColumn)
        {
            return title;
        }

        return _sortDescending ? $"{title} v" : $"{title} ^";
    }

    private static Control Labeled(string label, Control control, float minimumWidth = 0f)
    {
        var box = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        if (minimumWidth > 0f)
        {
            box.CustomMinimumSize = new Vector2(minimumWidth, 0f);
            box.SizeFlagsHorizontal = SizeFlags.ShrinkBegin;
        }

        box.AddThemeConstantOverride("separation", 2);
        var text = new Label { Text = label };
        text.AddThemeFontSizeOverride("font_size", 12);
        box.AddChild(text);
        control.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        box.AddChild(control);
        return box;
    }

    private static Control HorizontalFields(params Control[] fields)
    {
        var row = new HBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        row.AddThemeConstantOverride("separation", 8);
        foreach (var field in fields)
        {
            if (field.SizeFlagsHorizontal == 0)
            {
                field.SizeFlagsHorizontal = SizeFlags.ExpandFill;
            }

            row.AddChild(field);
        }

        return row;
    }

    private static Control SectionHeader(string text, Color accent)
    {
        var box = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        box.AddThemeConstantOverride("separation", 3);
        box.AddChild(SectionTitle(text, accent));
        box.AddChild(SectionRule(accent));
        return box;
    }

    private static Control ActionSectionHeader(string text, Control action, Color accent)
    {
        var box = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        box.AddThemeConstantOverride("separation", 3);

        var row = new HBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        row.AddThemeConstantOverride("separation", 8);
        var title = SectionTitle(text, accent);
        title.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        row.AddChild(title);
        row.AddChild(action);
        box.AddChild(row);
        box.AddChild(SectionRule(accent));
        return box;
    }

    private static Label SectionTitle(string text, Color accent)
    {
        var header = new Label { Text = text };
        header.AddThemeFontSizeOverride("font_size", 18);
        header.AddThemeColorOverride("font_color", accent);
        return header;
    }

    private static ColorRect SectionRule(Color accent)
    {
        return new ColorRect
        {
            Color = new Color(accent.R, accent.G, accent.B, 0.85f),
            CustomMinimumSize = new Vector2(0f, 2f),
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
    }

    private LineEdit BuildLine(string placeholder, bool liveChanges = true)
    {
        var line = new LineEdit { PlaceholderText = placeholder };
        if (liveChanges)
        {
            line.TextChanged += _ => OnFieldChanged();
        }

        return line;
    }

    private SpinBox BuildSpin(double min, double max, double step)
    {
        var spin = new SpinBox { MinValue = min, MaxValue = max, Step = step };
        spin.ValueChanged += _ => OnFieldChanged();
        return spin;
    }

    private TextEdit BuildText(float height)
    {
        var text = new TextEdit
        {
            CustomMinimumSize = new Vector2(0f, height),
            WrapMode = TextEdit.LineWrappingMode.Boundary
        };
        text.TextChanged += OnFieldChanged;
        return text;
    }

    private void RefreshItems()
    {
        PullFieldsIntoSelected();
        if (!FlushSelectedItemIfDirty("Reload save", refreshVisibleItems: false))
        {
            SetStatus("Reload blocked because the selected item could not be saved. Fix the item validation errors, then reload again.");
            return;
        }

        var selectedPath = _selected?.Path;
        var selectedId = _selected?.Item.Id;
        LogForensics("REFRESH_ITEMS_START", $"disk=[{BuildDiskItemSnapshot()}] selected={DescribeItemAsset(_selected)}");
        RefreshAchievementOptions();
        _items.Clear();
        var dir = DirAccess.Open(ItemDirectory);
        if (dir == null)
        {
            SetStatus($"Missing item directory: {ItemDirectory}");
            return;
        }

        dir.ListDirBegin();
        var itemFileCount = 0;
        var failedLoads = new List<string>();
        while (true)
        {
            var fileName = dir.GetNext();
            if (string.IsNullOrEmpty(fileName))
            {
                break;
            }

            if (dir.CurrentIsDir() || !fileName.EndsWith(".tres", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            itemFileCount++;
            var path = $"{ItemDirectory}/{fileName}";
            var item = LoadEditorResource<ItemDefinition>(path, ResourceLoader.CacheMode.Replace);
            if (item != null)
            {
                _items.Add(new ItemAsset(path, item));
            }
            else
            {
                failedLoads.Add(fileName);
            }
        }

        dir.ListDirEnd();
        if (_items.Count == 0 && itemFileCount > 0 && _startupRefreshAttempts < MaxStartupRefreshAttempts)
        {
            _startupRefreshAttempts++;
            SetStatus($"Found {itemFileCount} item files, but item scripts are not ready yet. Retrying load {_startupRefreshAttempts}/{MaxStartupRefreshAttempts}...");
            var tree = GetTree();
            if (tree != null)
            {
                tree.CreateTimer(0.25).Timeout += RefreshItems;
            }
            else
            {
                Callable.From(RefreshItems).CallDeferred();
            }

            return;
        }

        if (_items.Count > 0)
        {
            _startupRefreshAttempts = 0;
        }

        _items.Sort(CompareItems);
        RefreshFilterOptions();
        RefreshVisibleItems();
        SelectPreferredItemAfterRefresh(selectedPath, selectedId);
        var failedSummary = failedLoads.Count == 0 ? string.Empty : $" Failed to load: {string.Join(", ", failedLoads.Take(4))}{(failedLoads.Count > 4 ? "..." : string.Empty)}.";
        LogForensics("REFRESH_ITEMS_DONE", $"loaded={_items.Count}/{itemFileCount} failed=[{string.Join(", ", failedLoads)}] loadedItems=[{BuildLoadedItemSnapshot()}] catalog=[{BuildCatalogSnapshot()}]");
        SetStatus($"Loaded {_items.Count}/{itemFileCount} item resources. {BuildCatalogSyncSummary()}{failedSummary}");
        RunDeleteProbeIfRequested();
    }

    private void RefreshVisibleItems()
    {
        RefreshVisibleItemsCore(selectSpreadsheetRow: true);
    }

    private void RefreshVisibleItemsAfterBackgroundSave()
    {
        _syncingSelection = true;
        try
        {
            RefreshVisibleItemsCore(selectSpreadsheetRow: false);
        }
        finally
        {
            _syncingSelection = false;
        }
    }

    private void RefreshVisibleItemsCore(bool selectSpreadsheetRow)
    {
        _visibleItems.Clear();
        _itemList?.Clear();
        _itemTable?.Clear();
        var search = _search?.Text.Trim() ?? string.Empty;

        foreach (var asset in _items)
        {
            var item = asset.Item;
            if (!MatchesSearch(item, search))
            {
                continue;
            }

            _visibleItems.Add(asset);
        }

        SortVisibleItems();
        PopulateSpreadsheetRows();
        PopulateEditList();
        if (selectSpreadsheetRow)
        {
            SelectSpreadsheetRowBySelectedItem();
        }
    }

    private void SortVisibleItems()
    {
        IEnumerable<ItemAsset> ordered = _sortColumn switch
        {
            SpreadsheetColumn.StableId => _visibleItems.OrderBy(asset => asset.Item.Id, StringComparer.OrdinalIgnoreCase),
            SpreadsheetColumn.Name => _visibleItems.OrderBy(asset => asset.Item.DisplayName, StringComparer.OrdinalIgnoreCase),
            SpreadsheetColumn.PoolWeights => _visibleItems.OrderBy(asset => PoolWeight(asset.Item, ItemPoolIds.NightMarket)),
            SpreadsheetColumn.NightMarketShare => _visibleItems.OrderBy(asset => NightMarketShare(asset.Item)),
            SpreadsheetColumn.Tags => _visibleItems.OrderBy(asset => string.Join(",", asset.Item.Tags), StringComparer.OrdinalIgnoreCase),
            SpreadsheetColumn.UnlockAchievement => _visibleItems.OrderBy(asset => asset.Item.UnlockAchievementId, StringComparer.OrdinalIgnoreCase),
            SpreadsheetColumn.RevealedStats => _visibleItems.OrderBy(asset => asset.Item.RevealedStatText, StringComparer.OrdinalIgnoreCase),
            SpreadsheetColumn.RevealedBehavior => _visibleItems.OrderBy(asset => asset.Item.RevealedBehaviorText, StringComparer.OrdinalIgnoreCase),
            SpreadsheetColumn.Telemetry => _visibleItems.OrderBy(asset => asset.Item.PlayersPicked),
            SpreadsheetColumn.Path => _visibleItems.OrderBy(asset => asset.Path, StringComparer.OrdinalIgnoreCase),
            _ => _visibleItems.OrderBy(asset => EffectiveItemNumber(asset.Item))
        };

        if (_sortDescending)
        {
            ordered = ordered.Reverse();
        }
        var sorted = ordered.ToArray();
        _visibleItems.Clear();
        _visibleItems.AddRange(sorted);
    }

    private void PopulateSpreadsheetRows()
    {
        if (_itemTable == null)
        {
            return;
        }

        ConfigureSpreadsheetColumns();
        var root = _itemTable.CreateItem();
        foreach (var asset in _visibleItems)
        {
            var item = asset.Item;
            var row = _itemTable.CreateItem(root);
            if (_showIcons && item.Icon != null)
            {
                row.SetIcon((int)SpreadsheetColumn.Icon, item.Icon);
                row.SetIconMaxWidth((int)SpreadsheetColumn.Icon, 44);
            }

            row.SetText((int)SpreadsheetColumn.Number, NumberText(item));
            row.SetText((int)SpreadsheetColumn.StableId, item.Id);
            row.SetText((int)SpreadsheetColumn.Name, item.DisplayName);
            row.SetText((int)SpreadsheetColumn.PoolWeights, PoolWeightsText(item));
            row.SetText((int)SpreadsheetColumn.NightMarketShare, NightMarketShareText(item));
            row.SetText((int)SpreadsheetColumn.Tags, string.Join(", ", item.Tags));
            row.SetText((int)SpreadsheetColumn.UnlockAchievement, string.IsNullOrWhiteSpace(item.UnlockAchievementId) ? "-" : item.UnlockAchievementId);
            row.SetText((int)SpreadsheetColumn.RevealedStats, OneLineText(item.RevealedStatText));
            row.SetText((int)SpreadsheetColumn.RevealedBehavior, OneLineText(item.RevealedBehaviorText));
            row.SetText((int)SpreadsheetColumn.Telemetry, $"{item.PlayersPicked} picks | {item.WinRatePercent:0.##}% win");
            row.SetText((int)SpreadsheetColumn.Path, asset.Path);

            for (var column = 0; column < Enum.GetValues<SpreadsheetColumn>().Length; column++)
            {
                row.SetSelectable(column, true);
            }
        }
    }

    private void PopulateEditList()
    {
        if (_itemList == null)
        {
            return;
        }

        foreach (var asset in _visibleItems)
        {
            var item = asset.Item;
            _itemList.AddItem($"{NumberText(item)} {item.DisplayName}");
        }
    }

    private void OnSpreadsheetColumnClicked(long column, long mouseButtonIndex)
    {
        if (mouseButtonIndex != (long)MouseButton.Left)
        {
            return;
        }

        if (_suppressNextSort)
        {
            _suppressNextSort = false;
            return;
        }

        if (_resizingColumn >= 0)
        {
            return;
        }

        var clicked = (SpreadsheetColumn)Math.Clamp(column, 0, Enum.GetValues<SpreadsheetColumn>().Length - 1);
        if (_sortColumn == clicked)
        {
            _sortDescending = !_sortDescending;
        }
        else
        {
            _sortColumn = clicked;
            _sortDescending = false;
        }

        RefreshVisibleItemsPreservingSelection();
    }

    private void OnItemTableGuiInput(InputEvent inputEvent)
    {
        if (_itemTable == null)
        {
            return;
        }

        if (inputEvent is InputEventMouseButton button && button.ButtonIndex == MouseButton.Left)
        {
            if (button.Pressed)
            {
                var resizeColumn = FindResizeColumn(button.Position.X);
                if (resizeColumn >= 0)
                {
                    _resizingColumn = resizeColumn;
                    _resizeStartX = button.Position.X;
                    _resizeStartWidth = _itemTable.GetColumnWidth(resizeColumn);
                    _suppressNextSort = true;
                    _itemTable.AcceptEvent();
                }
            }
            else
            {
                _resizingColumn = -1;
            }

            return;
        }

        if (inputEvent is not InputEventMouseMotion motion)
        {
            return;
        }

        if (_resizingColumn >= 0)
        {
            ResizeSpreadsheetColumn(_resizingColumn, _resizeStartWidth + (int)(motion.Position.X - _resizeStartX));
            _itemTable.AcceptEvent();
        }
    }

    private int FindResizeColumn(float x)
    {
        if (_itemTable == null)
        {
            return -1;
        }

        var boundary = 0f;
        var lastResizableColumn = Enum.GetValues<SpreadsheetColumn>().Length - 2;
        for (var column = 0; column <= lastResizableColumn; column++)
        {
            boundary += _itemTable.GetColumnWidth(column);
            if (Math.Abs(x - boundary) <= ColumnResizeGrabPixels)
            {
                return column;
            }
        }

        return -1;
    }

    private void SelectSpreadsheetRow()
    {
        if (_syncingSelection)
        {
            return;
        }

        var selected = _itemTable?.GetSelected();
        if (selected == null)
        {
            return;
        }

        var id = selected.GetText((int)SpreadsheetColumn.StableId);
        var index = _visibleItems.FindIndex(asset => string.Equals(asset.Item.Id, id, StringComparison.OrdinalIgnoreCase));
        if (index >= 0)
        {
            _syncingSelection = true;
            try
            {
                SelectVisibleItem(index);
                _itemList?.Select(index);
            }
            finally
            {
                _syncingSelection = false;
            }
        }
    }

    private void SelectSpreadsheetRowBySelectedItem()
    {
        if (_itemTable == null || _selected == null)
        {
            return;
        }

        var root = _itemTable.GetRoot();
        var row = root?.GetFirstChild();
        while (row != null)
        {
            if (string.Equals(row.GetText((int)SpreadsheetColumn.StableId), _selected.Item.Id, StringComparison.OrdinalIgnoreCase))
            {
                row.Select((int)SpreadsheetColumn.Number);
                return;
            }

            row = row.GetNext();
        }
    }

    private void SelectFirstIfNeeded()
    {
        if (_selected != null && _items.Any(asset => ReferenceEquals(asset.Item, _selected.Item)))
        {
            return;
        }

        if (_visibleItems.Count == 0)
        {
            _selected = null;
            LoadSelectedIntoFields();
            return;
        }

        _itemList?.Select(0);
        SelectVisibleItem(0);
    }

    private void SelectPreferredItemAfterRefresh(string? selectedPath, string? selectedId)
    {
        var preferred = _visibleItems.FirstOrDefault(asset =>
                !string.IsNullOrWhiteSpace(selectedPath)
                && ResourcePathsEqual(asset.Path, selectedPath))
            ?? _visibleItems.FirstOrDefault(asset =>
                !string.IsNullOrWhiteSpace(selectedId)
                && string.Equals(asset.Item.Id, selectedId, StringComparison.OrdinalIgnoreCase));
        if (preferred == null)
        {
            SelectFirstIfNeeded();
            return;
        }

        _selected = preferred;
        LoadSelectedIntoFields();
        _syncingSelection = true;
        try
        {
            var index = _visibleItems.FindIndex(asset => ReferenceEquals(asset.Item, preferred.Item));
            if (index >= 0)
            {
                _itemList?.Select(index);
            }

            SelectSpreadsheetRowBySelectedItem();
        }
        finally
        {
            _syncingSelection = false;
        }
    }

    private void SelectVisibleItem(int index)
    {
        PullFieldsIntoSelected();
        FlushSelectedItemIfDirty("Selection-change save", refreshVisibleItems: false);
        if (index < 0 || index >= _visibleItems.Count)
        {
            return;
        }

        _selected = _visibleItems[index];
        LoadSelectedIntoFields();
        if (_syncingSelection)
        {
            return;
        }

        _syncingSelection = true;
        try
        {
            _itemList?.Select(index);
            SelectSpreadsheetRowBySelectedItem();
        }
        finally
        {
            _syncingSelection = false;
        }
    }

    private void LoadSelectedIntoFields()
    {
        _loadingFields = true;
        try
        {
            _stableIdEditDirty = false;
            var item = _selected?.Item;
            if (_itemNumber != null)
            {
                _itemNumber.Value = item?.ItemNumber > 0 ? item.ItemNumber : 0;
            }

            if (_id != null)
            {
                _id.Text = item?.Id ?? string.Empty;
            }

            if (_name != null)
            {
                _name.Text = item?.DisplayName ?? string.Empty;
            }

            if (_itemKind != null)
            {
                SelectEnumOption(_itemKind, item?.Kind ?? ItemKind.Passive);
            }

            RebuildPoolWeightRows();

            if (_unlimitedActivations != null)
            {
                _unlimitedActivations.ButtonPressed = item?.HasUnlimitedActivations ?? true;
            }

            if (_maxActivations != null)
            {
                _maxActivations.Value = item?.MaxActivations ?? 0;
            }

            RefreshActivationControls();
            if (_tags != null)
            {
                _tags.Text = item == null ? string.Empty : string.Join(", ", item.Tags);
            }

            _itemTagSelector?.Reload();
            _itemTagSelector?.SetSelectedSingleTag(string.Empty);

            if (_unlockAchievement != null)
            {
                SelectAchievementOption(item?.UnlockAchievementId ?? string.Empty);
            }

            if (_description != null)
            {
                _description.Text = item?.Description ?? string.Empty;
            }

            if (_hiddenDescription != null)
            {
                _hiddenDescription.Text = item?.HiddenDescription ?? string.Empty;
            }

            if (_revealedStatText != null)
            {
                _revealedStatText.Text = item?.RevealedStatText ?? string.Empty;
            }

            if (_revealedBehaviorText != null)
            {
                _revealedBehaviorText.Text = item?.RevealedBehaviorText ?? string.Empty;
            }

            if (_iconPreview != null)
            {
                _iconPreview.Texture = item?.Icon;
            }

            if (_enemyFormScenePath != null)
            {
                _enemyFormScenePath.Text = item?.EnemyFormPresentationScene?.ResourcePath ?? string.Empty;
            }

            if (_telemetryLabel != null)
            {
                _telemetryLabel.Text = item == null
                    ? "Players picked: 0 | Win rate: 0% | read-only telemetry"
                    : $"Players picked: {item.PlayersPicked} | Win rate: {item.WinRatePercent:0.##}% | read-only telemetry";
            }

            RebuildEffectRows();
            RebuildActiveEffectRows();
            RefreshPreviews();
        }
        finally
        {
            _loadingFields = false;
        }
    }

    private void PullFieldsIntoSelected(bool includeStableId = false, bool logDiagnostics = false)
    {
        if (_loadingFields || _selected == null)
        {
            return;
        }

        var item = _selected.Item;
        item.ItemNumber = _itemNumber == null ? item.ItemNumber : (int)_itemNumber.Value;
        if (includeStableId && _id != null)
        {
            item.Id = NormalizeId(_id.Text);
        }

        item.DisplayName = _name?.Text ?? item.DisplayName;
        item.Kind = _itemKind == null ? item.Kind : GetSelectedEnumValue(_itemKind, _itemKind.Selected, item.Kind);
        item.HasUnlimitedActivations = _unlimitedActivations?.ButtonPressed ?? item.HasUnlimitedActivations;
        item.MaxActivations = _maxActivations == null ? item.MaxActivations : (int)_maxActivations.Value;
        NormalizeActivationFields(item);
        item.UnlockAchievementId = SelectedAchievementId(item.UnlockAchievementId);
        item.Description = _description?.Text ?? item.Description;
        item.HiddenDescription = _hiddenDescription?.Text ?? item.HiddenDescription;
        item.RevealedStatText = _revealedStatText?.Text ?? item.RevealedStatText;
        item.RevealedBehaviorText = _revealedBehaviorText?.Text ?? item.RevealedBehaviorText;
        item.RevealedEffectText = ItemDefinition.CombineRevealedText(item.RevealedStatText, item.RevealedBehaviorText);
        item.EnemyFormPresentationScene = LoadOptionalPackedScene(_enemyFormScenePath?.Text);
        ReplaceTags(item, _tags?.Text ?? string.Empty);

        if (item.Effects.Count > 0)
        {
            foreach (var effect in item.Effects)
            {
                SanitizeEffectFields(effect);
            }
        }

        if (logDiagnostics)
        {
            LogForensics("PULL_FIELDS", BuildDiagnosticState("pull_fields"));
        }
    }

    private void OnStableIdTextChanged()
    {
        if (_loadingFields || _committingStableIdEdit)
        {
            return;
        }

        _stableIdEditDirty = true;
        _hasPendingChanges = true;
        _closeFlushAttempted = false;
        _autoSaveQueued = false;
        SetStatus("Stable ID rename pending. Press Enter or leave the field to save the rename.");
    }

    private void CommitStableIdEdit(string reason, bool markDirty)
    {
        if (_loadingFields || _committingStableIdEdit || _selected == null || _id == null)
        {
            return;
        }

        if (!_stableIdEditDirty)
        {
            return;
        }

        _committingStableIdEdit = true;
        try
        {
            var previousId = _selected.Item.Id;
            var committedId = NormalizeId(_id.Text);
            if (!string.Equals(_id.Text, committedId, StringComparison.Ordinal))
            {
                _id.Text = committedId;
            }

            _selected.Item.Id = committedId;
            _stableIdEditDirty = false;
            if (!string.Equals(previousId, committedId, StringComparison.OrdinalIgnoreCase))
            {
                _dirtyItemPaths.Add(_selected.Path);
                _hasPendingChanges = true;
                _closeFlushAttempted = false;
                RefreshFilterOptions();
                RefreshVisibleItemsPreservingSelection();
                SetStatus($"Stable ID rename queued: {previousId} -> {committedId}.");
                LogForensics("STABLE_ID_COMMITTED", $"reason={reason} previous={previousId} committed={committedId} {BuildDiagnosticState("stable_id_committed")}");
                if (markDirty)
                {
                    MarkSelectedDirty("stable_id_committed");
                }
            }
            else
            {
                _hasPendingChanges = _dirtyItemPaths.Count > 0;
                LogForensics("STABLE_ID_COMMIT_NOOP", $"reason={reason} id={committedId} {BuildDiagnosticState("stable_id_commit_noop")}");
            }
        }
        finally
        {
            _committingStableIdEdit = false;
        }
    }

    private void OnFieldChanged()
    {
        if (_loadingFields)
        {
            return;
        }

        PullFieldsIntoSelected();
        RefreshActivationControls();
        MarkSelectedDirty("field_changed");
    }

    private void RefreshActivationControls()
    {
        var item = _selected?.Item;
        var isActivatable = item?.Kind == ItemKind.Activatable;
        var hasUnlimited = item?.HasUnlimitedActivations ?? true;

        if (_unlimitedActivations != null)
        {
            _unlimitedActivations.Disabled = !isActivatable;
            _unlimitedActivations.ButtonPressed = hasUnlimited;
        }

        if (_maxActivations != null)
        {
            _maxActivations.Editable = isActivatable && !hasUnlimited;
            _maxActivations.Value = item?.MaxActivations ?? 0;
        }
    }

    private void NormalizeActivationFields(ItemDefinition item)
    {
        if (item.Kind != ItemKind.Activatable)
        {
            item.HasUnlimitedActivations = true;
            item.MaxActivations = 0;
            return;
        }

        if (item.HasUnlimitedActivations)
        {
            item.MaxActivations = 0;
        }
        else if (item.MaxActivations <= 0)
        {
            item.MaxActivations = 1;
        }
    }

    private void RebuildPoolWeightRows()
    {
        if (_poolWeightsBox == null)
        {
            return;
        }

        foreach (var child in _poolWeightsBox.GetChildren())
        {
            child.QueueFree();
        }

        var item = _selected?.Item;
        if (item == null)
        {
            return;
        }

        for (var i = 0; i < item.PoolWeights.Count; i++)
        {
            var poolWeight = item.PoolWeights[i];
            if (poolWeight != null)
            {
                _poolWeightsBox.AddChild(BuildPoolWeightRow(i, poolWeight));
            }
        }
    }

    private Control BuildPoolWeightRow(int index, ItemPoolWeightDefinition poolWeight)
    {
        var row = new HBoxContainer();
        row.AddThemeConstantOverride("separation", 6);

        var pool = new OptionButton
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        var poolValues = FillItemPoolOptions(pool, poolWeight.PoolId);
        pool.ItemSelected += selected =>
        {
            var selectedPool = SelectedStringOptionValue(poolValues, selected, poolWeight.PoolId);
            if (string.IsNullOrWhiteSpace(selectedPool))
            {
                return;
            }

            poolWeight.PoolId = selectedPool;
            MarkPoolWeightsChanged();
        };
        row.AddChild(pool);

        var weight = new SpinBox
        {
            MinValue = 0.01,
            MaxValue = 100000,
            Step = 0.01,
            Value = Math.Max(0.01f, poolWeight.Weight),
            CustomMinimumSize = new Vector2(120f, 0f)
        };
        weight.ValueChanged += changed =>
        {
            poolWeight.Weight = (float)changed;
            MarkPoolWeightsChanged();
        };
        row.AddChild(weight);

        var remove = new Button { Text = "Remove" };
        remove.Pressed += () =>
        {
            var item = _selected?.Item;
            if (item == null || index < 0 || index >= item.PoolWeights.Count)
            {
                return;
            }

            item.PoolWeights.RemoveAt(index);
            MarkPoolWeightsChanged(rebuildRows: true);
        };
        row.AddChild(remove);
        return row;
    }

    private void AddPoolWeight()
    {
        var item = _selected?.Item;
        if (item == null)
        {
            return;
        }

        var existing = item.PoolWeights
            .Where(poolWeight => poolWeight != null)
            .Select(poolWeight => poolWeight.PoolId)
            .ToHashSet(StringComparer.OrdinalIgnoreCase);
        var poolId = ItemPoolIds.All.FirstOrDefault(pool => !existing.Contains(pool));
        if (string.IsNullOrWhiteSpace(poolId))
        {
            SetStatus("All known item pools are already assigned to this item.");
            return;
        }

        item.PoolWeights.Add(new ItemPoolWeightDefinition
        {
            PoolId = poolId,
            Weight = 1f
        });
        MarkPoolWeightsChanged(rebuildRows: true);
    }

    private void MarkPoolWeightsChanged(bool rebuildRows = false)
    {
        if (_loadingFields)
        {
            return;
        }

        PullFieldsIntoSelected();
        if (rebuildRows)
        {
            RebuildPoolWeightRows();
        }

        RefreshVisibleItemsPreservingSelection();
        RefreshPreviews();
    }

    private void RebuildEffectRows()
    {
        if (_effectsBox == null)
        {
            return;
        }

        foreach (var child in _effectsBox.GetChildren())
        {
            child.QueueFree();
        }

        var item = _selected?.Item;
        if (item == null)
        {
            return;
        }

        for (var i = 0; i < item.Effects.Count; i++)
        {
            _effectsBox.AddChild(BuildEffectRow(i, item.Effects[i]));
        }
    }

    private Control BuildEffectRow(int index, ItemEffectSpec effect)
    {
        SanitizeEffectFields(effect);
        var panel = new PanelContainer();
        var margin = new MarginContainer();
        margin.AddThemeConstantOverride("margin_left", 6);
        margin.AddThemeConstantOverride("margin_top", 6);
        margin.AddThemeConstantOverride("margin_right", 6);
        margin.AddThemeConstantOverride("margin_bottom", 6);
        panel.AddChild(margin);

        var grid = new GridContainer { Columns = 2 };
        grid.AddThemeConstantOverride("h_separation", 6);
        grid.AddThemeConstantOverride("v_separation", 4);
        margin.AddChild(grid);

        var kind = new OptionButton();
        FillEnumOptions<ItemEffectKind>(kind);
        SelectEnumOption(kind, effect.Kind);
        kind.ItemSelected += selected =>
        {
            effect.Kind = GetSelectedEnumValue(kind, (int)selected, effect.Kind);
            ApplyEffectDefaults(effect);
            PullFieldsIntoSelected();
            RebuildEffectRows();
            RefreshPreviews();
        };
        grid.AddChild(new Label { Text = "Effect" });
        grid.AddChild(kind);

        if (EffectUsesStat(effect.Kind))
        {
            var stat = new OptionButton();
            var statValues = FillStatOptions(stat, effect.StatId);
            stat.ItemSelected += selected =>
            {
                effect.StatId = SelectedStringOptionValue(statValues, selected, effect.StatId);
                PullFieldsIntoSelected();
                RefreshPreviews();
            };
            grid.AddChild(new Label { Text = "Stat" });
            grid.AddChild(stat);
        }

        if (EffectUsesValue(effect.Kind))
        {
            var value = new SpinBox
            {
                MinValue = EffectValueMin(effect.Kind),
                MaxValue = EffectValueMax(effect.Kind),
                Step = effect.Kind is ItemEffectKind.AddSpellCount or ItemEffectKind.KeepItIn or ItemEffectKind.PreventNextPlayerHealthLoss ? 1 : 0.01,
                Value = effect.Value
            };
            value.ValueChanged += changed =>
            {
                effect.Value = (float)changed;
                PullFieldsIntoSelected();
                RefreshPreviews();
            };
            grid.AddChild(new Label { Text = EffectValueLabel(effect.Kind) });
            grid.AddChild(value);
        }
        else
        {
            grid.AddChild(new Label { Text = "Amount" });
            grid.AddChild(new Label { Text = FixedValueLabel(effect.Kind), AutowrapMode = TextServer.AutowrapMode.WordSmart });
        }

        if (EffectUsesSecondaryValue(effect.Kind))
        {
            var secondary = new SpinBox
            {
                MinValue = SecondaryEffectValueMin(effect.Kind),
                MaxValue = SecondaryEffectValueMax(effect.Kind),
                Step = effect.Kind is ItemEffectKind.FaultyFocus or ItemEffectKind.HomingSpells ? 1 : 0.05,
                Value = effect.SecondaryValue
            };
            secondary.ValueChanged += changed =>
            {
                effect.SecondaryValue = (float)changed;
                PullFieldsIntoSelected();
                RefreshPreviews();
            };
            grid.AddChild(new Label { Text = SecondaryEffectValueLabel(effect.Kind) });
            grid.AddChild(secondary);
        }

        if (EffectUsesTertiaryValue(effect.Kind))
        {
            var tertiary = new SpinBox
            {
                MinValue = TertiaryEffectValueMin(effect.Kind),
                MaxValue = TertiaryEffectValueMax(effect.Kind),
                Step = 0.1,
                Value = effect.TertiaryValue
            };
            tertiary.ValueChanged += changed =>
            {
                effect.TertiaryValue = (float)changed;
                PullFieldsIntoSelected();
                RefreshPreviews();
            };
            grid.AddChild(new Label { Text = TertiaryEffectValueLabel(effect.Kind) });
            grid.AddChild(tertiary);
        }

        if (EffectUsesChance(effect.Kind))
        {
            var chance = new SpinBox { MinValue = 0, MaxValue = 100, Step = 0.1, Value = effect.ChancePercent };
            chance.ValueChanged += changed =>
            {
                effect.ChancePercent = (float)changed;
                PullFieldsIntoSelected();
                RefreshPreviews();
            };
            grid.AddChild(new Label { Text = "Proc Chance %" });
            grid.AddChild(chance);
        }

        if (EffectUsesTarget(effect.Kind))
        {
            var targetPanel = new VBoxContainer();
            targetPanel.AddThemeConstantOverride("separation", 6);
            var target = new LineEdit
            {
                Text = effect.TargetTag,
                PlaceholderText = EffectTargetPlaceholder(effect.Kind),
                Editable = false,
                SizeFlagsHorizontal = SizeFlags.ExpandFill
            };
            targetPanel.AddChild(target);
            var targetSelector = new GameplayTagSelector
            {
                MultiSelect = false,
                IncludeClearOption = true,
                CustomMinimumSize = new Vector2(0f, 140f),
                SizeFlagsHorizontal = SizeFlags.ExpandFill
            };
            targetSelector.SelectionChanged += () =>
            {
                var tag = targetSelector.SelectedSingleTag();
                target.Text = tag;
                effect.TargetTag = tag;
                PullFieldsIntoSelected();
                RefreshPreviews();
            };
            targetPanel.AddChild(targetSelector);
            Callable.From(() => targetSelector.SetSelectedSingleTag(effect.TargetTag)).CallDeferred();
            grid.AddChild(new Label { Text = "Target Tag" });
            grid.AddChild(targetPanel);
        }
        else
        {
            grid.AddChild(new Label { Text = "Target" });
            grid.AddChild(new Label { Text = FixedTargetLabel(effect.Kind), AutowrapMode = TextServer.AutowrapMode.WordSmart });
        }

        var output = new Label
        {
            Text = $"Effect: {ItemEffectCompiler.Describe(effect)}",
            AutowrapMode = TextServer.AutowrapMode.WordSmart
        };
        grid.AddChild(new Label { Text = string.Empty });
        grid.AddChild(output);

        var remove = new Button { Text = "Remove Effect" };
        remove.Pressed += () =>
        {
            var item = _selected?.Item;
            if (item == null || index < 0 || index >= item.Effects.Count)
            {
                return;
            }

            item.Effects.RemoveAt(index);
            PullFieldsIntoSelected();
            RebuildEffectRows();
            RefreshPreviews();
        };
        grid.AddChild(new Label { Text = string.Empty });
        grid.AddChild(remove);

        return panel;
    }

    private void RebuildActiveEffectRows()
    {
        if (_activeEffectsBox == null)
        {
            return;
        }

        foreach (var child in _activeEffectsBox.GetChildren())
        {
            child.QueueFree();
        }

        var item = _selected?.Item;
        if (item == null || item.Kind != ItemKind.Activatable)
        {
            return;
        }

        for (var i = 0; i < item.ActiveEffects.Count; i++)
        {
            _activeEffectsBox.AddChild(BuildActiveEffectRow(i, item.ActiveEffects[i]));
        }
    }

    private Control BuildActiveEffectRow(int index, EffectSpec effect)
    {
        var panel = new PanelContainer();
        var margin = new MarginContainer();
        margin.AddThemeConstantOverride("margin_left", 6);
        margin.AddThemeConstantOverride("margin_top", 6);
        margin.AddThemeConstantOverride("margin_right", 6);
        margin.AddThemeConstantOverride("margin_bottom", 6);
        panel.AddChild(margin);

        var grid = new GridContainer { Columns = 2 };
        grid.AddThemeConstantOverride("h_separation", 6);
        grid.AddThemeConstantOverride("v_separation", 4);
        margin.AddChild(grid);

        var type = new OptionButton();
        var effectTypes = FillStringOptionsPreservingSelected(type, ActiveEffectIds.All, effect.EffectType);
        type.ItemSelected += selected =>
        {
            effect.EffectType = SelectedStringOptionValue(effectTypes, selected, effect.EffectType);
            PullFieldsIntoSelected();
            RefreshPreviews();
        };
        grid.AddChild(new Label { Text = "Executor" });
        grid.AddChild(type);

        var value = new SpinBox { MinValue = -10000, MaxValue = 10000, Step = 0.01, Value = effect.Value };
        value.ValueChanged += changed =>
        {
            effect.Value = (float)changed;
            PullFieldsIntoSelected();
            RefreshPreviews();
        };
        grid.AddChild(new Label { Text = "Value" });
        grid.AddChild(value);

        var radius = new SpinBox { MinValue = 0, MaxValue = 1000, Step = 0.01, Value = effect.Radius };
        radius.ValueChanged += changed =>
        {
            effect.Radius = (float)changed;
            PullFieldsIntoSelected();
            RefreshPreviews();
        };
        grid.AddChild(new Label { Text = "Radius" });
        grid.AddChild(radius);

        var duration = new SpinBox { MinValue = 0, MaxValue = 1000, Step = 0.01, Value = effect.DurationSeconds };
        duration.ValueChanged += changed =>
        {
            effect.DurationSeconds = (float)changed;
            PullFieldsIntoSelected();
            RefreshPreviews();
        };
        grid.AddChild(new Label { Text = "Duration" });
        grid.AddChild(duration);

        var remove = new Button { Text = "Remove Active Effect" };
        remove.Pressed += () =>
        {
            var item = _selected?.Item;
            if (item == null || index < 0 || index >= item.ActiveEffects.Count)
            {
                return;
            }

            item.ActiveEffects.RemoveAt(index);
            PullFieldsIntoSelected();
            RebuildActiveEffectRows();
            RefreshPreviews();
        };
        grid.AddChild(new Label { Text = string.Empty });
        grid.AddChild(remove);
        return panel;
    }

    private void AddEffect()
    {
        var item = _selected?.Item;
        if (item == null)
        {
            return;
        }

        item.Effects.Add(new ItemEffectSpec
        {
            Kind = ItemEffectKind.AddMultiplyStat,
            StatId = "damage",
            Value = 1.1f,
            ChancePercent = 100f
        });
        PullFieldsIntoSelected();
        RebuildEffectRows();
        RefreshPreviews();
    }

    private void AddActiveEffect()
    {
        var item = _selected?.Item;
        if (item == null)
        {
            return;
        }

        item.Kind = ItemKind.Activatable;
        item.ActiveEffects.Add(new EffectSpec
        {
            EffectType = ActiveEffectIds.MagicBall,
            DurationScaling = DurationScalingMode.None
        });
        PullFieldsIntoSelected();
        LoadSelectedIntoFields();
        RefreshPreviews();
    }

    private void CreateBlankItem()
    {
        try
        {
            var number = NextItemNumber();
            var item = new ItemDefinition
            {
                ItemNumber = number,
                DisplayName = $"New Item {number:000}",
                Description = "Designer notes.",
                HiddenDescription = "Its power is not yet known.",
                RevealedStatText = "+10% Damage",
                RevealedBehaviorText = string.Empty,
                RevealedEffectText = "+10% Damage",
            };
            item.Id = CreateUniqueItemId(item.DisplayName, number, item);
            item.PoolWeights.Add(new ItemPoolWeightDefinition { PoolId = ItemPoolIds.NightMarket, Weight = 1f });
            item.Effects.Add(new ItemEffectSpec { Kind = ItemEffectKind.AddMultiplyStat, StatId = "damage", Value = 1.1f, ChancePercent = 100f });
            SaveNewItem(item);
        }
        catch (Exception exception)
        {
            LogForensics("CREATE_BLANK_EXCEPTION", exception.ToString());
            SetStatus($"Create blank failed: {exception.Message}");
        }
    }

    private void DuplicateSelectedItem()
    {
        var selected = _selected;
        if (selected == null)
        {
            return;
        }

        PullFieldsIntoSelected();
        var number = NextItemNumber();
        var clone = CloneItem(selected.Item);
        clone.ItemNumber = number;
        clone.DisplayName = $"{selected.Item.DisplayName} Copy";
        clone.Id = CreateUniqueItemId(selected.Item.DisplayName, number, clone);
        SaveNewItem(clone);
    }

    private void SaveNewItem(ItemDefinition item)
    {
        item.Id = NormalizeId(item.Id);
        if (!IsItemIdAvailable(item.Id, item))
        {
            item.Id = CreateUniqueItemId(item.DisplayName, item.ItemNumber, item);
        }

        var path = PathForItemId(item.Id);
        LogForensics("CREATE_NEW_BEGIN", $"path={path} item={BuildItemDebugSnapshot(item)} disk=[{BuildDiskItemSnapshot()}]");
        EnsureItemTagsInCatalog(item);
        var validation = ValidatePendingCatalog(item);
        if (validation.HasErrors)
        {
            SetStatus($"Create blocked: {FormatValidationErrors(validation, item.Id)}");
            LogForensics("CREATE_NEW_VALIDATION_BLOCKED", FormatValidationErrors(validation, item.Id));
            return;
        }

        var error = SaveItemResource(item, path);
        LogForensics("CREATE_NEW_RESOURCE_DONE", $"result={error} file={BuildFileMetadata(path)} item={BuildItemDebugSnapshot(item)}");
        if (error != Error.Ok)
        {
            SetStatus($"Could not save {path}: {error}");
            return;
        }

        if (!VerifySavedItem(item, path, out var verifyMessage))
        {
            SetStatus($"Could not verify {path}: {verifyMessage}");
            LogForensics("CREATE_NEW_VERIFY_FAILED", $"path={path} {verifyMessage}");
            return;
        }

        _items.Add(new ItemAsset(path, item));
        _items.Sort(CompareItems);
        RefreshFilterOptions();
        RefreshVisibleItems();
        _selected = _items.FirstOrDefault(asset => ReferenceEquals(asset.Item, item));
        LoadSelectedIntoFields();
        var catalogError = SyncCatalog(false);
        LogForensics("CREATE_NEW_CATALOG_DONE", $"result={catalogError} selected={DescribeItemAsset(_selected)} catalog=[{BuildCatalogSnapshot()}] disk=[{BuildDiskItemSnapshot()}]");
        if (catalogError == Error.Ok)
        {
            _hasPendingChanges = false;
        }

        SetStatus(catalogError == Error.Ok
            ? $"Created {path}. Item list and gameplay catalog updated."
            : $"Created {path}, but catalog sync failed: {catalogError}.");
    }

    public bool TryFlushPendingChangesBeforePlay(out string message)
    {
        message = string.Empty;
        if (_selected == null)
        {
            return true;
        }

        PullFieldsIntoSelected();
        if (!HasPendingChanges() && !SelectedItemNeedsPersistence(_selected, out _))
        {
            return true;
        }

        _autoSaveQueued = false;
        return SaveSelectedItemCore("Pre-play save", refreshVisibleItems: true, out message);
    }

    public bool TryFlushPendingChangesBeforeEditorClose(out string message)
    {
        message = string.Empty;
        LogForensics("CLOSE_SAVE_REQUEST", BuildDiagnosticState("close_save_request"));
        if (_selected == null)
        {
            ClearStalePendingStateWithoutSelectedItem();
            LogForensics("CLOSE_SAVE_SKIPPED", "no selected item");
            WriteSaveDiagnosticReport("CLOSE_SAVE_SKIPPED", BuildDiagnosticState("close_save_skipped_no_selected"), result: true);
            return true;
        }

        PullFieldsIntoSelected();
        if (!HasPendingChanges() && !SelectedItemNeedsPersistence(_selected, out _))
        {
            message = "Editor-close save skipped because the item editor had no pending changes.";
            LogForensics("CLOSE_SAVE_SKIPPED_NO_PENDING", BuildDiagnosticState("close_save_skipped_no_pending"));
            WriteSaveDiagnosticReport("CLOSE_SAVE_SKIPPED_NO_PENDING", $"{BuildDiagnosticState("close_save_skipped_no_pending")}\nmessage={message}", result: true);
            return true;
        }

        if (_closeFlushAttempted)
        {
            LogForensics("CLOSE_SAVE_RETRY_WITH_PENDING", BuildDiagnosticState("close_save_retry_with_pending"));
        }

        _closeFlushAttempted = true;
        _autoSaveQueued = false;
        LogForensics("CLOSE_SAVE_READY", BuildDiagnosticState("close_save_ready"));
        var result = SaveSelectedItemCore("Editor-close save", refreshVisibleItems: false, out message);
        LogForensics("CLOSE_SAVE_DONE", $"result={result} message={message} selected={DescribeItemAsset(_selected)} disk=[{BuildDiskItemSnapshot()}] catalog=[{BuildCatalogSnapshot()}]");
        WriteSaveDiagnosticReport("CLOSE_SAVE_DONE", $"{BuildDiagnosticState("close_save_done")}\nmessage={message}", result);
        if (result)
        {
            ScanEditorFileSystem();
        }

        return result;
    }

    public bool HasPendingChanges()
    {
        if (_selected == null && _dirtyItemPaths.Count == 0)
        {
            return false;
        }

        return _hasPendingChanges || _stableIdEditDirty || _dirtyItemPaths.Count > 0 || _autoSaveQueued;
    }

    private void ClearStalePendingStateWithoutSelectedItem()
    {
        if (_selected != null || _dirtyItemPaths.Count > 0)
        {
            return;
        }

        _autoSaveQueued = false;
        _hasPendingChanges = false;
        _closeFlushAttempted = false;
    }

    private void SaveSelectedItem()
    {
        if (SaveSelectedItemCore("Save", refreshVisibleItems: true, out var message))
        {
            SetStatus(message);
        }
    }

    private bool FlushSelectedItemIfDirty(string label, bool refreshVisibleItems)
    {
        var selected = _selected;
        if (selected == null || !SelectedItemNeedsPersistence(selected, out _))
        {
            return true;
        }

        _autoSaveQueued = false;
        return SaveSelectedItemCore(label, refreshVisibleItems, out _);
    }

    private bool SelectedItemNeedsPersistence(ItemAsset selected, out string reason)
    {
        if (_dirtyItemPaths.Contains(selected.Path))
        {
            reason = "dirty_path";
            return true;
        }

        if (_hasPendingChanges)
        {
            reason = "pending_flag";
            return true;
        }

        if (_stableIdEditDirty)
        {
            reason = "stable_id_edit_dirty";
            return true;
        }

        if (_autoSaveQueued)
        {
            reason = "autosave_queued";
            return true;
        }

        if (!VerifySavedItem(selected.Item, selected.Path, out var verifyMessage))
        {
            reason = $"disk_mismatch:{verifyMessage}";
            LogForensics("SAVE_NEEDED_BY_DISK_COMPARE", $"selected={DescribeItemAsset(selected)} reason={reason}");
            return true;
        }

        reason = "clean";
        return false;
    }

    private bool SaveSelectedItemCore(string label, bool refreshVisibleItems, out string message)
    {
        var selected = _selected;
        if (selected == null)
        {
            message = "No selected item.";
            WriteSaveDiagnosticReport($"{label.ToUpperInvariant()}_NO_SELECTION", message, result: true);
            return true;
        }

        LogForensics("SAVE_BEGIN", $"label={label} refreshVisibleItems={refreshVisibleItems} {BuildDiagnosticState("save_begin")}");
        CommitStableIdEdit($"{label}_save", markDirty: false);
        PullFieldsIntoSelected(logDiagnostics: true);
        EnsureItemTagsInCatalog(selected.Item);
        var validation = ValidatePendingCatalog(selected.Item);
        if (validation.HasErrors)
        {
            message = $"{label} blocked: {FormatValidationErrors(validation, selected.Item.Id)}";
            SetStatus(message);
            WriteSaveDiagnosticReport($"{label.ToUpperInvariant()}_VALIDATION_BLOCKED", $"{BuildDiagnosticState("save_validation_blocked")}\nvalidation={FormatValidationIssues(validation)}\nmessage={message}", result: false);
            return false;
        }

        LogForensics("SAVE_VALIDATION_OK", $"label={label} selected={DescribeItemAsset(selected)} validation={FormatValidationIssues(validation)}");
        var targetPath = PathForItemId(selected.Item.Id);
        var moved = !ResourcePathsEqual(selected.Path, targetPath);
        if (moved && ResourcePathExists(targetPath))
        {
            message = $"{label} blocked: {targetPath} already exists for stable ID '{selected.Item.Id}'.";
            SetStatus(message);
            WriteSaveDiagnosticReport($"{label.ToUpperInvariant()}_PATH_EXISTS", $"{BuildDiagnosticState("save_path_exists")}\nmessage={message}", result: false);
            return false;
        }

        var error = SaveItemResource(selected.Item, targetPath);
        LogForensics("SAVE_RESOURCE_DONE", $"label={label} result={error} file={BuildFileMetadata(targetPath)} selected={DescribeItemAsset(selected)} targetPath={targetPath}");
        if (error != Error.Ok)
        {
            message = $"{label} failed: {error}";
            SetStatus(message);
            WriteSaveDiagnosticReport($"{label.ToUpperInvariant()}_RESOURCE_SAVE_FAILED", $"{BuildDiagnosticState("save_resource_failed")}\nresourceSaveResult={error}\nmessage={message}", result: false);
            return false;
        }

        if (!VerifySavedItem(selected.Item, targetPath, out var verifyMessage))
        {
            message = $"{label} verification failed for {targetPath}: {verifyMessage}";
            SetStatus(message);
            LogForensics("SAVE_VERIFY_FAILED", $"label={label} path={targetPath} {verifyMessage}");
            WriteSaveDiagnosticReport($"{label.ToUpperInvariant()}_VERIFY_FAILED", $"{BuildDiagnosticState("save_verify_failed")}\nmessage={message}", result: false);
            return false;
        }

        var updated = selected with { Path = targetPath };
        ReplaceLoadedItem(selected, updated);
        _selected = updated;

        if (moved)
        {
            var deleteError = DeleteResourcePath(selected.Path);
            LogForensics("SAVE_RENAMED_PATH", $"label={label} old={selected.Path} new={targetPath} deleteOld={deleteError}");
            if (deleteError != Error.Ok && ResourcePathExists(selected.Path))
            {
                message = $"{label} saved {targetPath}, but could not remove old file {selected.Path}: {deleteError}.";
                SetStatus(message);
                WriteSaveDiagnosticReport($"{label.ToUpperInvariant()}_OLD_PATH_DELETE_FAILED", $"{BuildDiagnosticState("save_old_path_delete_failed")}\nmessage={message}", result: false);
                return false;
            }
            _dirtyItemPaths.Remove(selected.Path);
            ScanEditorFileSystem();
        }

        RefreshFilterOptions();
        if (refreshVisibleItems)
        {
            RefreshVisibleItemsPreservingSelection();
        }

        var catalogError = SyncCatalog(false);
        LogForensics("SAVE_CATALOG_DONE", $"label={label} result={catalogError} catalogFile={BuildFileMetadata(CatalogPath)} catalog=[{BuildCatalogSnapshot()}]");
        if (catalogError != Error.Ok)
        {
            message = $"{label} wrote {selected.Path}, but catalog sync failed: {catalogError}.";
            SetStatus(message);
            WriteSaveDiagnosticReport($"{label.ToUpperInvariant()}_CATALOG_SAVE_FAILED", $"{BuildDiagnosticState("save_catalog_failed")}\ncatalogSaveResult={catalogError}\nmessage={message}", result: false);
            return false;
        }

        _dirtyItemPaths.Remove(targetPath);
        _hasPendingChanges = _dirtyItemPaths.Count > 0;
        _autoSaveQueued = false;
        message = $"{label} saved {targetPath}. Item list and gameplay catalog updated.";
        WriteSaveDiagnosticReport($"{label.ToUpperInvariant()}_SUCCESS", $"{BuildDiagnosticState("save_success")}\nmessage={message}", result: true);
        return true;
    }

    private static Error SaveItemResource(ItemDefinition item, string path)
    {
        var error = ResourceSaver.Save(
            item,
            path,
            ResourceSaver.SaverFlags.ChangePath | ResourceSaver.SaverFlags.ReplaceSubresourcePaths);
        if (error == Error.Ok)
        {
            if (!ResourcePathsEqual(item.ResourcePath, path))
            {
                item.TakeOverPath(path);
            }
        }

        return error;
    }

    private void ReplaceLoadedItem(ItemAsset oldAsset, ItemAsset newAsset)
    {
        var index = _items.FindIndex(asset =>
            ReferenceEquals(asset.Item, oldAsset.Item)
            || ResourcePathsEqual(asset.Path, oldAsset.Path));
        if (index >= 0)
        {
            _items[index] = newAsset;
            ReplaceVisibleItem(oldAsset, newAsset);
            return;
        }

        _items.Add(newAsset);
        ReplaceVisibleItem(oldAsset, newAsset);
    }

    private void ReplaceVisibleItem(ItemAsset oldAsset, ItemAsset newAsset)
    {
        var index = _visibleItems.FindIndex(asset =>
            ReferenceEquals(asset.Item, oldAsset.Item)
            || ResourcePathsEqual(asset.Path, oldAsset.Path));
        if (index >= 0)
        {
            _visibleItems[index] = newAsset;
        }
    }

    private string CreateUniqueItemId(string displayName, int itemNumber, ItemDefinition? owner)
    {
        var baseName = StripTrailingItemNumber(StripCopySuffixes(displayName), itemNumber);
        var baseId = NormalizeOptionalId(baseName);
        if (string.IsNullOrWhiteSpace(baseId))
        {
            baseId = $"designer_{Math.Max(1, itemNumber):000}";
        }

        var numbered = $"{baseId}_{Math.Max(1, itemNumber):000}";
        if (IsItemIdAvailable(numbered, owner))
        {
            return numbered;
        }

        for (var suffix = 2; suffix < 1000; suffix++)
        {
            var candidate = $"{numbered}_{suffix:00}";
            if (IsItemIdAvailable(candidate, owner))
            {
                return candidate;
            }
        }

        return $"{numbered}_{DateTimeOffset.Now.ToUnixTimeSeconds()}";
    }

    private bool IsItemIdAvailable(string itemId, ItemDefinition? owner)
    {
        var normalized = NormalizeId(itemId);
        return _items.All(asset => ReferenceEquals(asset.Item, owner)
            || !string.Equals(NormalizeId(asset.Item.Id), normalized, StringComparison.OrdinalIgnoreCase));
    }

    private static string StripCopySuffixes(string value)
    {
        var cleaned = (value ?? string.Empty).Trim();
        while (cleaned.EndsWith(" Copy", StringComparison.OrdinalIgnoreCase))
        {
            cleaned = cleaned[..^5].TrimEnd();
        }

        return cleaned;
    }

    private static string StripTrailingItemNumber(string value, int itemNumber)
    {
        var cleaned = (value ?? string.Empty).Trim();
        if (itemNumber <= 0)
        {
            return cleaned;
        }

        var number = itemNumber.ToString("000", CultureInfo.InvariantCulture);
        foreach (var separator in new[] { " ", "_", "-" })
        {
            var suffix = $"{separator}{number}";
            if (cleaned.EndsWith(suffix, StringComparison.OrdinalIgnoreCase))
            {
                return cleaned[..^suffix.Length].TrimEnd(' ', '_', '-');
            }
        }

        return cleaned;
    }

    private static string PathForItemId(string itemId)
    {
        return $"{ItemDirectory}/item_{NormalizeId(itemId)}.tres";
    }

    private static bool ResourcePathExists(string resourcePath)
    {
        try
        {
            return System.IO.File.Exists(ProjectSettings.GlobalizePath(resourcePath));
        }
        catch
        {
            return false;
        }
    }

    private static bool ResourcePathsEqual(string left, string right)
    {
        return string.Equals(
            NormalizeAbsolutePath(ProjectSettings.GlobalizePath(left)),
            NormalizeAbsolutePath(ProjectSettings.GlobalizePath(right)),
            StringComparison.OrdinalIgnoreCase);
    }

    private static Error DeleteResourcePath(string resourcePath)
    {
        try
        {
            var root = NormalizeAbsolutePath(ProjectSettings.GlobalizePath(ItemDirectory));
            var absolute = NormalizeAbsolutePath(ProjectSettings.GlobalizePath(resourcePath));
            if (!IsInsideDirectory(absolute, root) || !System.IO.File.Exists(absolute))
            {
                return Error.Ok;
            }

            return DirAccess.RemoveAbsolute(absolute);
        }
        catch
        {
            return Error.Failed;
        }
    }

    private static bool VerifySavedItem(ItemDefinition expected, string path, out string message)
    {
        message = string.Empty;
        ItemDefinition? loaded;
        try
        {
            loaded = ResourceLoader.Load(path, string.Empty, ResourceLoader.CacheMode.Ignore) as ItemDefinition;
        }
        catch (Exception exception)
        {
            message = $"reload exception: {exception.Message}";
            return false;
        }

        if (loaded == null)
        {
            message = "reload returned null";
            return false;
        }

        try
        {
            var expectedFingerprint = BuildPersistenceFingerprint(expected);
            var loadedFingerprint = BuildPersistenceFingerprint(loaded);
            if (string.Equals(expectedFingerprint, loadedFingerprint, StringComparison.Ordinal))
            {
                return true;
            }

            message = $"fingerprint mismatch expected='{SafeOneLine(expectedFingerprint)}' loaded='{SafeOneLine(loadedFingerprint)}'";
            return false;
        }
        finally
        {
            DetachVerificationItem(loaded);
        }
    }

    private static string BuildPersistenceFingerprint(ItemDefinition item)
    {
        return string.Join('\n', new[]
        {
            $"number={item.ItemNumber}",
            $"id={item.Id}",
            $"name={item.DisplayName}",
            $"kind={item.Kind}",
            $"unlimited={item.HasUnlimitedActivations}",
            $"maxActivations={item.MaxActivations}",
            $"unlock={item.UnlockAchievementId}",
            $"flavor={item.IsFlavorOnly}",
            $"icon={item.Icon != null}",
            $"description={item.Description}",
            $"hidden={item.HiddenDescription}",
            $"revealedStats={item.RevealedStatText}",
            $"revealedBehavior={item.RevealedBehaviorText}",
            $"revealed={item.RevealedEffectText}",
            $"tags={string.Join("|", item.Tags)}",
            $"pools={string.Join("|", item.PoolWeights.Where(pool => pool != null).Select(pool => $"{pool.PoolId}:{FloatText(pool.Weight)}"))}",
            $"effects={string.Join("|", item.Effects.Where(effect => effect != null).Select(effect => $"{effect.Kind}:{effect.StatId}:{FloatText(effect.Value)}:{FloatText(effect.SecondaryValue)}:{FloatText(effect.TertiaryValue)}:{FloatText(effect.ChancePercent)}:{effect.Priority}:{effect.TargetTag}"))}",
            $"activeEffects={string.Join("|", item.ActiveEffects.Where(effect => effect != null).Select(effect => $"{effect.EffectType}:{FloatText(effect.Value)}:{FloatText(effect.Radius)}:{FloatText(effect.DurationSeconds)}:{effect.DurationScaling}:{effect.TargetStatId}"))}"
        });
    }

    private static string FloatText(float value)
    {
        return value.ToString("R", CultureInfo.InvariantCulture);
    }

    private void SyncCatalog()
    {
        _ = SyncCatalog(true);
    }

    private Error SyncCatalog(bool updateStatus)
    {
        PullFieldsIntoSelected();
        LogForensics("SYNC_CATALOG_START", $"updateStatus={updateStatus} items=[{BuildLoadedItemSnapshot()}] disk=[{BuildDiskItemSnapshot()}]");
        var catalog = LoadEditorResource<ContentCatalog>(CatalogPath);
        if (catalog == null)
        {
            if (updateStatus)
            {
                SetStatus($"Could not load catalog {CatalogPath}.");
            }
            LogForensics("SYNC_CATALOG_FAILED", $"catalogLoadFailed path={CatalogPath}");
            return Error.Failed;
        }

        catalog.Items.Clear();
        foreach (var asset in _items.OrderBy(asset => asset.Item.ItemNumber <= 0 ? int.MaxValue : asset.Item.ItemNumber).ThenBy(asset => asset.Item.DisplayName))
        {
            catalog.Items.Add(asset.Item);
        }

        var validation = new ContentValidator().Validate(catalog);
        if (validation.HasErrors)
        {
            if (updateStatus)
            {
                SetStatus($"Catalog sync blocked: {FormatValidationErrors(validation, _selected?.Item.Id)}");
            }

            LogForensics("SYNC_CATALOG_BLOCKED", FormatValidationErrors(validation, _selected?.Item.Id));
            return Error.InvalidData;
        }

        var error = ResourceSaver.Save(catalog, CatalogPath);
        LogForensics("SYNC_CATALOG_DONE", $"result={error} catalog=[{BuildCatalogSnapshot()}] disk=[{BuildDiskItemSnapshot()}]");
        if (updateStatus)
        {
            SetStatus(error == Error.Ok ? $"Catalog synced with {_items.Count} items." : $"Catalog sync failed: {error}");
        }

        return error;
    }

    private void RefreshNumbers()
    {
        PullFieldsIntoSelected();
        var ordered = _items
            .OrderBy(asset => asset.Item.ItemNumber <= 0 ? int.MaxValue : asset.Item.ItemNumber)
            .ThenBy(asset => asset.Item.DisplayName, StringComparer.OrdinalIgnoreCase)
            .ToArray();

        for (var i = 0; i < ordered.Length; i++)
        {
            ordered[i].Item.ItemNumber = i + 1;
            SaveItemResource(ordered[i].Item, ordered[i].Path);
        }

        _items.Sort(CompareItems);
        RefreshVisibleItemsPreservingSelection();
        LoadSelectedIntoFields();
        SetStatus($"Refreshed item numbers 1..{ordered.Length}.");
    }

    private void RequestDeleteSelectedItem()
    {
        if (_selected == null)
        {
            return;
        }

        if (_deleteConfirm != null)
        {
            _deleteConfirm.DialogText = $"Delete {_selected.Item.DisplayName}?\n{_selected.Path}";
            _deleteConfirm.PopupCentered();
        }
    }

    private void RunDeleteProbeIfRequested()
    {
        if (_deleteProbeRan)
        {
            return;
        }

        var targetPath = OS.GetEnvironment("TLM_ITEM_DELETE_PROBE");
        if (string.IsNullOrWhiteSpace(targetPath))
        {
            return;
        }

        _deleteProbeRan = true;
        var selected = _items.FirstOrDefault(asset => string.Equals(asset.Path, targetPath, StringComparison.OrdinalIgnoreCase));
        LogForensics("DELETE_PROBE_REQUEST", $"target={targetPath} found={selected != null} loaded=[{BuildLoadedItemSnapshot()}]");
        if (selected == null)
        {
            return;
        }

        _selected = selected;
        DeleteSelectedItem();
    }

    private void DeleteSelectedItem()
    {
        var selected = _selected;
        if (selected == null)
        {
            return;
        }

        var root = NormalizeAbsolutePath(ProjectSettings.GlobalizePath(ItemDirectory));
        var absolute = NormalizeAbsolutePath(ProjectSettings.GlobalizePath(selected.Path));
        LogForensics("DELETE_REQUEST", $"selected={DescribeItemAsset(selected)} root={root} absolute={absolute} exists={System.IO.File.Exists(absolute)} disk=[{BuildDiskItemSnapshot()}] catalog=[{BuildCatalogSnapshot()}]");
        if (!IsInsideDirectory(absolute, root))
        {
            LogForensics("DELETE_REFUSED", $"outsideItemDirectory selected={DescribeItemAsset(selected)} root={root} absolute={absolute}");
            SetStatus($"Refused to delete outside {ItemDirectory}.");
            return;
        }

        if (!System.IO.File.Exists(absolute))
        {
            LogForensics("DELETE_FAILED", $"missingOnDisk selected={DescribeItemAsset(selected)} absolute={absolute}");
            SetStatus($"Delete failed: {selected.Path} does not exist on disk. Reloading item list.");
            RefreshItems();
            return;
        }

        Error deleteError;
        try
        {
            deleteError = DirAccess.RemoveAbsolute(absolute);
        }
        catch (Exception exception)
        {
            LogForensics("DELETE_EXCEPTION", $"selected={DescribeItemAsset(selected)} exception={exception}");
            SetStatus($"Delete failed for {selected.Path}: {exception.Message}");
            RefreshItems();
            return;
        }

        if (deleteError != Error.Ok || System.IO.File.Exists(absolute))
        {
            LogForensics("DELETE_FAILED", $"selected={DescribeItemAsset(selected)} result={deleteError} existsAfter={System.IO.File.Exists(absolute)} disk=[{BuildDiskItemSnapshot()}]");
            SetStatus($"Delete failed for {selected.Path}: {deleteError}.");
            RefreshItems();
            return;
        }

        LogForensics("DELETE_FILE_REMOVED", $"selected={DescribeItemAsset(selected)} result={deleteError} existsAfter={System.IO.File.Exists(absolute)} disk=[{BuildDiskItemSnapshot()}]");
        var previousResourcePath = selected.Item.ResourcePath;
        selected.Item.TakeOverPath(selected.Path);
        selected.Item.SetPathCache(string.Empty);
        LogForensics("DELETE_RESOURCE_DETACHED", $"deleted={selected.Path} previousResourcePath={previousResourcePath} currentResourcePath={selected.Item.ResourcePath}");
        _deletedPathsThisSession.Add(selected.Path);
        _items.Remove(selected);
        _selected = null;
        RefreshFilterOptions();
        RefreshVisibleItems();
        SelectFirstIfNeeded();
        var catalogError = SyncCatalog(false);
        ScanEditorFileSystem();
        LogForensics("DELETE_DONE", $"deleted={selected.Path} catalogResult={catalogError} disk=[{BuildDiskItemSnapshot()}] catalog=[{BuildCatalogSnapshot()}] loaded=[{BuildLoadedItemSnapshot()}]");
        SetStatus(catalogError == Error.Ok
            ? $"Deleted {selected.Path}. Item list and gameplay catalog updated."
            : $"Deleted {selected.Path}, but catalog sync failed: {catalogError}.");
    }

    private void AssignIconFromPath(string path)
    {
        var selected = _selected;
        if (selected == null)
        {
            SetStatus("Select an item before assigning an icon.");
            return;
        }

        var image = new Image();
        var loadPath = path.StartsWith("res://", StringComparison.OrdinalIgnoreCase)
            ? ProjectSettings.GlobalizePath(path)
            : path;
        var error = image.Load(loadPath);
        if (error != Error.Ok)
        {
            SetStatus($"Could not load icon image '{path}': {error}");
            return;
        }

        var resized = false;
        if (image.GetWidth() != RequiredIconSize || image.GetHeight() != RequiredIconSize)
        {
            image.Resize(RequiredIconSize, RequiredIconSize, Image.Interpolation.Lanczos);
            resized = true;
        }

        selected.Item.Icon = ImageTexture.CreateFromImage(image);
        if (_iconPreview != null)
        {
            _iconPreview.Texture = selected.Item.Icon;
        }

        RefreshPreviews();
        SetStatus(resized ? $"Icon assigned and resized to {RequiredIconSize}x{RequiredIconSize}." : "Icon assigned.");
    }

    private void RefreshPreviews()
    {
        RefreshSpellImpactPreview();
        MarkSelectedDirty("preview_refresh");
    }

    private void RefreshSpellImpactPreview()
    {
        if (_spellImpactHeader == null || _spellImpactLeftColumn == null || _spellImpactRightColumn == null)
        {
            return;
        }

        ClearChildren(_spellImpactLeftColumn);
        ClearChildren(_spellImpactRightColumn);

        var item = _selected?.Item;
        if (item == null)
        {
            _spellImpactHeader.Text = "Select an item.";
            return;
        }

        var catalog = LoadEditorResource<ContentCatalog>(CatalogPath, ResourceLoader.CacheMode.Ignore);
        if (catalog == null)
        {
            _spellImpactHeader.Text = $"Could not load {CatalogPath}.";
            return;
        }

        var modifiers = ItemEffectCompiler.BuildModifierSpecs(item.Effects)
            .Where(modifier => modifier != null)
            .ToArray();
        var spells = catalog.Spells
            .Where(spell => spell != null)
            .OrderBy(spell => spell.DisplayName, StringComparer.OrdinalIgnoreCase)
            .ToArray();

        _spellImpactHeader.Text =
            $"[b]Selected Item:[/b] {Escape(item.DisplayName)}\n" +
            $"[b]Compiled Modifiers:[/b] {modifiers.Length}";

        if (spells.Length == 0)
        {
            _spellImpactLeftColumn.AddChild(BuildSpellImpactBlock("No spells are registered in the content catalog."));
        }
        else
        {
            var splitIndex = (int)MathF.Ceiling(spells.Length / 2f);
            for (var i = 0; i < spells.Length; i++)
            {
                var spellLines = new List<string>();
                AppendSpellImpact(spellLines, spells[i], modifiers);
                var targetColumn = i < splitIndex ? _spellImpactLeftColumn : _spellImpactRightColumn;
                targetColumn.AddChild(BuildSpellImpactBlock(string.Join('\n', spellLines)));
            }
        }
    }

    private static void ClearChildren(Node node)
    {
        foreach (var child in node.GetChildren())
        {
            child.QueueFree();
        }
    }

    private static RichTextLabel BuildSpellImpactBlock(string text)
    {
        var label = new RichTextLabel
        {
            BbcodeEnabled = true,
            FitContent = true,
            ScrollActive = false,
            ContextMenuEnabled = true,
            SelectionEnabled = true,
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ShrinkBegin,
            Text = text
        };
        label.AddThemeFontSizeOverride("font_size", 15);
        return label;
    }

    private static void AppendSpellImpact(List<string> lines, SpellDefinition spell, IReadOnlyList<ModifierSpec> modifiers)
    {
        var tags = SpellTags(spell);
        var damageTags = DamageSourceTags(spell, tags);
        var stats = new List<SpellPreviewStat>();

        var fireRate = MathF.Max(0.05f, ResolvePreviewStat("fire_rate", 1f, tags, modifiers));
        AddPreviewStat(stats, "Cooldown", MathF.Max(0.05f, spell.CooldownSeconds), MathF.Max(0.05f, spell.CooldownSeconds / fireRate), "s");

        var spellCount = 1 + Math.Clamp((int)MathF.Floor(ResolvePreviewStat("spell_count", 0f, tags, modifiers) + 0.001f), 0, 12);
        AddPreviewStat(stats, "Casts", 1f, spellCount, string.Empty, integer: true);

        foreach (var effect in spell.Effects.Where(effect => effect != null))
        {
            AppendEffectStats(stats, spell, effect, tags, damageTags, modifiers);
        }

        var changed = stats.Where(IsChanged).ToArray();
        lines.Add($"[b]{Escape(spell.DisplayName)}[/b]");
        if (changed.Length == 0)
        {
            lines.Add("  [color=#777777]No changes from this item.[/color]");
        }
        else
        {
            foreach (var stat in changed)
            {
                lines.Add($"  {FormatChangedStat(stat)}");
            }

            var affectedBy = ApplicableModifierSummary(modifiers, tags, damageTags);
            if (!string.IsNullOrWhiteSpace(affectedBy))
            {
                lines.Add($"  [color=#777777]Affected by: {Escape(affectedBy)}[/color]");
            }
        }

        lines.Add(string.Empty);
    }

    private static void AppendEffectStats(
        List<SpellPreviewStat> stats,
        SpellDefinition spell,
        EffectSpec effect,
        IReadOnlyList<string> tags,
        IReadOnlyList<string> damageTags,
        IReadOnlyList<ModifierSpec> modifiers)
    {
        var targetStat = string.IsNullOrWhiteSpace(effect.TargetStatId) ? "damage" : effect.TargetStatId;
        if (effect.Value != 0f)
        {
            AddPreviewStat(stats, string.Equals(targetStat, "damage", StringComparison.OrdinalIgnoreCase) ? "Damage" : "Power", effect.Value, MathF.Max(0f, ResolvePreviewStat(targetStat, effect.Value, damageTags, modifiers)));
        }

        if (effect.Radius > 0f)
        {
            AddPreviewStat(stats, RadiusLabel(effect.EffectType), effect.Radius, MathF.Max(0.05f, ResolvePreviewStat("radius", effect.Radius, tags, modifiers)), "m");
        }

        if (effect.DurationSeconds > 0f && UsesDurationAsDuration(effect.EffectType))
        {
            var duration = effect.DurationScaling == DurationScalingMode.PlayerDuration
                ? MathF.Max(0f, ResolvePreviewStat("duration", effect.DurationSeconds, tags, modifiers))
                : MathF.Max(0f, effect.DurationSeconds);
            AddPreviewStat(stats, "Duration", effect.DurationSeconds, duration, "s");
        }

        AppendDeliveryStats(stats, effect, tags, modifiers);
        AppendSpecialSpellStats(stats, spell, tags, modifiers);
    }

    private static void AppendDeliveryStats(
        List<SpellPreviewStat> stats,
        EffectSpec effect,
        IReadOnlyList<string> tags,
        IReadOnlyList<ModifierSpec> modifiers)
    {
        if (IsProjectileEffect(effect.EffectType))
        {
            var baseSpeed = string.Equals(effect.EffectType, "frost_projectile_damage", StringComparison.OrdinalIgnoreCase) ? 24f : 18f;
            var speed = MathF.Max(0.1f, ResolvePreviewStat("projectile_speed", baseSpeed, tags, modifiers));
            AddPreviewStat(stats, "Projectile Speed", baseSpeed, speed, "m/s");

            var baseTravel = effect.DurationSeconds * baseSpeed;
            var travel = MathF.Max(0.1f, ResolvePreviewStat("range", effect.DurationSeconds * speed, tags, modifiers));
            AddPreviewStat(stats, "Travel", baseTravel, travel, "m");

            var pierce = ResolvePreviewStat("pierce", 0f, tags, modifiers) > 0f ? 1f : 0f;
            AddPreviewStat(stats, "Pierce", 0f, pierce, string.Empty, integer: true, hideWhenZero: true);

            var split = Math.Clamp((int)MathF.Floor(ResolvePreviewStat("projectile_split", 0f, tags, modifiers) + 0.001f), 0, 4);
            AddPreviewStat(stats, "Split Depth", 0f, split, string.Empty, integer: true, hideWhenZero: true);
            return;
        }

        if (string.Equals(effect.EffectType, "hitscan_beam_damage", StringComparison.OrdinalIgnoreCase))
        {
            AddPreviewStat(stats, "Beam Range", effect.DurationSeconds, MathF.Max(1f, ResolvePreviewStat("range", effect.DurationSeconds, tags, modifiers)), "m");
            return;
        }

        var basePlacementRange = BasePlacementRange(effect.EffectType);
        if (basePlacementRange > 0f)
        {
            AddPreviewStat(stats, "Placement Range", basePlacementRange, MathF.Max(0.1f, ResolvePreviewStat("range", basePlacementRange, tags, modifiers)), "m");
        }

        if (string.Equals(effect.EffectType, "raise_dead_summon", StringComparison.OrdinalIgnoreCase))
        {
            var baseSearch = MathF.Max(0.05f, ResolvePreviewStat("radius", effect.Radius, tags, modifiers));
            var search = MathF.Max(0.1f, ResolvePreviewStat("range", baseSearch, tags, modifiers));
            AddPreviewStat(stats, "Corpse Search", effect.Radius, search, "m");
            AddPreviewStat(stats, "Summon Aggro", 26f, MathF.Max(0.1f, ResolvePreviewStat("range", 26f, tags, modifiers)), "m");
        }
    }

    private static void AppendSpecialSpellStats(
        List<SpellPreviewStat> stats,
        SpellDefinition spell,
        IReadOnlyList<string> tags,
        IReadOnlyList<ModifierSpec> modifiers)
    {
        var homing = MathF.Max(0f, ResolvePreviewStat("homing_strength", 0f, tags, modifiers));
        AddPreviewStat(stats, "Homing Strength", 0f, homing, hideWhenZero: true);
        if (homing > 0f)
        {
            AddPreviewStat(stats, "Homing Cone", 0f, ResolvePreviewStat("homing_angle_degrees", 0f, tags, modifiers), "deg", hideWhenZero: true);
            AddPreviewStat(stats, "Homing Acquire", 0f, ResolvePreviewStat("homing_acquire_radius", 0f, tags, modifiers), "m", hideWhenZero: true);
        }

        var orbit = MathF.Max(0f, ResolvePreviewStat("spell_orbit_strength", 0f, tags, modifiers));
        AddPreviewStat(stats, "Orbit Strength", 0f, orbit, hideWhenZero: true);
        if (orbit > 0f)
        {
            AddPreviewStat(stats, "Orbit Radius", 0f, ResolvePreviewStat("spell_orbit_radius", 0f, tags, modifiers), "m", hideWhenZero: true);
            AddPreviewStat(stats, "Orbit Speed", 0f, ResolvePreviewStat("spell_orbit_angular_speed", 0f, tags, modifiers), "rad/s", hideWhenZero: true);
        }

        if (SpellHasTag(spell, "spell.behavior.summon"))
        {
            AddPreviewStat(stats, "Summon Damage Scale", 1f, ResolvePreviewStat("summon_damage", 1f, DamageSourceTags(spell, tags), modifiers), "x");
        }
    }

    private static void AddPreviewStat(
        List<SpellPreviewStat> stats,
        string label,
        float baseValue,
        float finalValue,
        string suffix = "",
        bool integer = false,
        bool hideWhenZero = false)
    {
        stats.Add(new SpellPreviewStat(label, baseValue, finalValue, suffix, integer, hideWhenZero));
    }

    private static string ApplicableModifierSummary(
        IReadOnlyList<ModifierSpec> modifiers,
        IReadOnlyList<string> tags,
        IReadOnlyList<string> damageTags)
    {
        var summaries = modifiers
            .Where(modifier => ModifierMatchesTags(modifier.TargetTag, tags) || ModifierMatchesTags(modifier.TargetTag, damageTags))
            .Select(FormatModifierSummary)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToArray();
        return string.Join(", ", summaries);
    }

    private static string FormatModifierSummary(ModifierSpec modifier)
    {
        return modifier.Operation switch
        {
            DataModifierOp.FlatAdd => $"{FormatSignedPreview(modifier.Value)} {modifier.StatId}",
            DataModifierOp.Multiplicative => $"{FormatSignedPercent((modifier.Value - 1f) * 100f)} {modifier.StatId}",
            DataModifierOp.AdditivePercent => $"{FormatSignedPercent(modifier.Value * 100f)} {modifier.StatId}",
            DataModifierOp.MinClamp => $"{modifier.StatId} min {FormatPreviewNumber(modifier.Value)}",
            DataModifierOp.MaxClamp => $"{modifier.StatId} max {FormatPreviewNumber(modifier.Value)}",
            DataModifierOp.Override => $"{modifier.StatId} = {FormatPreviewNumber(modifier.Value)}",
            _ => $"{modifier.Operation} {modifier.StatId} {FormatPreviewNumber(modifier.Value)}"
        };
    }

    private static bool IsChanged(SpellPreviewStat stat)
    {
        return Math.Abs(stat.FinalValue - stat.BaseValue) > 0.001f;
    }

    private static string FormatChangedStat(SpellPreviewStat stat)
    {
        var change = stat.BaseValue == 0f
            ? string.Empty
            : $" ({FormatSignedPercent(((stat.FinalValue / stat.BaseValue) - 1f) * 100f)})";
        return $"[color=#c38ee0]{Escape(stat.Label)}[/color] {FormatStatValue(stat, stat.BaseValue)} -> [b]{FormatStatValue(stat, stat.FinalValue)}[/b]{change}";
    }

    private static string FormatStatValue(SpellPreviewStat stat, float value)
    {
        return stat.Integer
            ? MathF.Round(value).ToString("0", CultureInfo.InvariantCulture) + stat.Suffix
            : FormatPreviewNumber(value) + stat.Suffix;
    }

    private static string FormatSignedPreview(float value)
    {
        return value >= 0f ? $"+{FormatPreviewNumber(value)}" : FormatPreviewNumber(value);
    }

    private static string FormatSignedPercent(float value)
    {
        return value >= 0f ? $"+{FormatPreviewNumber(value)}%" : $"{FormatPreviewNumber(value)}%";
    }

    private static float ResolvePreviewStat(
        string statId,
        float baseValue,
        IReadOnlyList<string> targetTags,
        IEnumerable<ModifierSpec> modifiers)
    {
        var applicable = modifiers
            .Where(modifier => string.Equals(modifier.StatId, statId, StringComparison.OrdinalIgnoreCase))
            .Where(modifier => ModifierMatchesTags(modifier.TargetTag, targetTags))
            .OrderBy(modifier => modifier.Priority)
            .ThenBy(modifier => modifier.Operation)
            .ToArray();

        var value = baseValue;
        foreach (var modifier in applicable)
        {
            value = modifier.Operation switch
            {
                DataModifierOp.FlatAdd => value + modifier.Value,
                DataModifierOp.AdditivePercent => value + (baseValue * modifier.Value),
                DataModifierOp.Multiplicative => value * modifier.Value,
                DataModifierOp.MinClamp => MathF.Max(value, modifier.Value),
                DataModifierOp.MaxClamp => MathF.Min(value, modifier.Value),
                DataModifierOp.Override => modifier.Value,
                _ => value
            };
        }

        return value;
    }

    private static bool ModifierMatchesTags(string targetTag, IReadOnlyList<string> sourceTags)
    {
        var target = GameplayTagPath.Normalize(targetTag);
        return string.IsNullOrWhiteSpace(target)
            || sourceTags.Any(source => GameplayTagPath.IsSameOrParentOf(target, source));
    }

    private static IReadOnlyList<string> SpellTags(SpellDefinition spell)
    {
        return spell.Tags
            .Select(GameplayTagPath.Normalize)
            .Where(tag => !string.IsNullOrWhiteSpace(tag))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToArray();
    }

    private static IReadOnlyList<string> DamageSourceTags(SpellDefinition spell, IReadOnlyList<string> spellTags)
    {
        if (!SpellHasTag(spell, "spell.behavior.summon"))
        {
            return spellTags;
        }

        return spellTags
            .Concat(new[] { "combat.source.summon" })
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToArray();
    }

    private static bool SpellHasTag(SpellDefinition spell, string targetTag)
    {
        return SpellTags(spell).Any(tag => GameplayTagPath.IsSameOrParentOf(targetTag, tag));
    }

    private static bool IsProjectileEffect(string effectType)
    {
        return string.Equals(effectType, "projectile_damage", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effectType, "frost_projectile_damage", StringComparison.OrdinalIgnoreCase);
    }

    private static bool UsesDurationAsDuration(string effectType)
    {
        return !IsProjectileEffect(effectType)
            && !string.Equals(effectType, "hitscan_beam_damage", StringComparison.OrdinalIgnoreCase);
    }

    private static float BasePlacementRange(string effectType)
    {
        return effectType switch
        {
            "firewall_hazard" => 7f,
            "blizzard_aoe" => 10f,
            "tornado_vortex" => 3f,
            "raise_dead_summon" => 0f,
            _ => 0f
        };
    }

    private static string RadiusLabel(string effectType)
    {
        return effectType switch
        {
            "firewall_hazard" => "Wall Half-Length",
            "hitscan_beam_damage" => "Beam Width",
            "raise_dead_summon" => "Base Corpse Search",
            _ => "Radius"
        };
    }

    private static string FormatPreviewNumber(float value)
    {
        return value.ToString("0.##", CultureInfo.InvariantCulture);
    }

    private void MarkSelectedDirty(string reason)
    {
        if (_loadingFields || _savingFields || _selected == null)
        {
            return;
        }

        if (_stableIdEditDirty)
        {
            _hasPendingChanges = true;
            _autoSaveQueued = false;
            SetStatus("Stable ID rename pending. Press Enter or leave the field to queue the rename.");
            return;
        }

        _dirtyItemPaths.Add(_selected.Path);
        _hasPendingChanges = true;
        _closeFlushAttempted = false;
        _autoSaveQueued = false;
        SetStatus("Unsaved item changes. Press Save Item, or they will be saved before Play, selection change, reload, or editor close.");
    }

    private void RefreshValidationPreview()
    {
        var catalog = BuildCatalogForValidation();
        var result = catalog == null ? null : new ContentValidator().Validate(catalog);
        if (_validationPreview == null)
        {
            return;
        }

        if (result == null)
        {
            _validationPreview.Text = $"Could not load {CatalogPath}.";
            return;
        }

        var itemId = _selected?.Item.Id ?? string.Empty;
        var relevant = result.Issues
            .Where(issue => string.IsNullOrWhiteSpace(itemId) || issue.Message.Contains($"'{itemId}'", StringComparison.OrdinalIgnoreCase))
            .ToArray();
        var lines = new List<string>
        {
            result.HasErrors ? "[b]Catalog: ERRORS[/b]" : "[b]Catalog: OK[/b]",
            BuildCatalogSyncSummary(),
            string.Empty,
            $"[b]Selected item issues:[/b] {relevant.Length}"
        };

        foreach (var issue in relevant.Take(24))
        {
            lines.Add($"{issue.Severity}: {Escape(issue.Code)} - {Escape(issue.Message)}");
        }

        if (relevant.Length == 0)
        {
            lines.Add("none");
        }

        _validationPreview.Text = string.Join('\n', lines);
    }

    private ContentCatalog? BuildCatalogForValidation()
    {
        var catalog = LoadEditorResource<ContentCatalog>(CatalogPath);
        if (catalog == null)
        {
            return null;
        }

        catalog.Items.Clear();
        foreach (var asset in _items)
        {
            catalog.Items.Add(asset.Item);
        }

        return catalog;
    }

    private ValidationResult ValidatePendingCatalog(ItemDefinition candidate)
    {
        var catalog = LoadEditorResource<ContentCatalog>(CatalogPath) ?? new ContentCatalog();
        catalog.Items.Clear();
        foreach (var asset in _items)
        {
            if (ReferenceEquals(asset.Item, candidate))
            {
                continue;
            }

            catalog.Items.Add(asset.Item);
        }

        catalog.Items.Add(candidate);
        return FilterSaveValidationToCandidate(new ContentValidator().Validate(catalog), candidate.Id);
    }

    private static ValidationResult FilterSaveValidationToCandidate(ValidationResult result, string candidateId)
    {
        var filtered = ValidationResult.Valid();
        foreach (var issue in result.Issues)
        {
            if (IsRelevantSaveValidationIssue(issue, candidateId))
            {
                filtered.Add(issue.Severity, issue.Code, issue.Message, issue.Path);
            }
        }

        return filtered;
    }

    private static bool IsRelevantSaveValidationIssue(ValidationIssue issue, string candidateId)
    {
        if (issue.Code.StartsWith("gameplay_tags.", StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        var normalizedId = NormalizeId(candidateId);
        if (issue.Code.StartsWith("item.", StringComparison.OrdinalIgnoreCase))
        {
            return string.IsNullOrWhiteSpace(normalizedId)
                || issue.Message.Contains($"'{normalizedId}'", StringComparison.OrdinalIgnoreCase);
        }

        return issue.Code.Equals("content.duplicate_id", StringComparison.OrdinalIgnoreCase)
            && issue.Message.Contains("item", StringComparison.OrdinalIgnoreCase)
            && issue.Message.Contains($"'{normalizedId}'", StringComparison.OrdinalIgnoreCase);
    }

    private static string FormatValidationErrors(ValidationResult result, string? selectedItemId)
    {
        var errors = result.Issues
            .Where(issue => issue.Severity == ValidationSeverity.Error)
            .Where(issue => string.IsNullOrWhiteSpace(selectedItemId)
                || issue.Message.Contains($"'{selectedItemId}'", StringComparison.OrdinalIgnoreCase)
                || !issue.Code.StartsWith("item.", StringComparison.OrdinalIgnoreCase))
            .Take(4)
            .Select(issue => $"{issue.Code}: {issue.Message}")
            .ToArray();
        return errors.Length == 0 ? "catalog has validation errors" : string.Join(" | ", errors);
    }

    private string BuildModifierPreview(ItemDefinition item)
    {
        var modifiers = ItemEffectCompiler.BuildModifierSpecs(item.Effects);
        var lines = new List<string> { $"[b]Compiled Modifiers:[/b] {modifiers.Count}" };
        foreach (var modifier in modifiers)
        {
            lines.Add($"{Escape(modifier.StatId)} {modifier.Operation} {FloatText(modifier.Value)} target={Escape(EmptyAsAll(modifier.TargetTag))}");
        }

        if (modifiers.Count == 0)
        {
            lines.Add("none");
        }

        lines.Add(string.Empty);
        lines.Add($"[b]Designer Effects:[/b] {item.Effects.Count}");
        foreach (var effect in item.Effects)
        {
            lines.Add(Escape(ItemEffectCompiler.Describe(effect)));
        }

        return string.Join('\n', lines);
    }

    private string BuildProcPreview(ItemDefinition item)
    {
        var procs = ItemEffectCompiler.BuildProcSpecs(item.Effects);
        var lines = new List<string> { $"[b]Compiled Procs:[/b] {procs.Count}" };
        foreach (var proc in procs)
        {
            lines.Add($"{proc.Trigger} {Escape(proc.EffectType)} chance={FloatText(proc.Chance)} value={FloatText(proc.Value)} target={Escape(EmptyAsAll(proc.TargetTag))}");
        }

        if (procs.Count == 0)
        {
            lines.Add("none");
        }

        lines.Add(string.Empty);
        lines.Add("Proc budget and recursion caps are runtime-owned by ProcSystem.");
        return string.Join('\n', lines);
    }

    private string BuildLootPreview(ItemDefinition item)
    {
        var total = TotalPoolWeight(ItemPoolIds.NightMarket);
        var nightMarketWeight = PoolWeight(item, ItemPoolIds.NightMarket);
        var chance = total <= 0f ? 0f : MathF.Max(0f, nightMarketWeight) * 100f / total;
        return
            $"[b]Pool Weights:[/b] {Escape(PoolWeightsText(item))}\n" +
            $"[b]Night Market Weight:[/b] {nightMarketWeight:0.##}\n" +
            $"[b]Night Market Share:[/b] {chance:0.##}% of current candidate weight before achievement filters\n" +
            $"[b]Night Market Total Weight:[/b] {total:0.##}\n" +
            $"[b]Unlock Achievement:[/b] {Escape(string.IsNullOrWhiteSpace(item.UnlockAchievementId) ? "-" : item.UnlockAchievementId)}";
    }

    private string BuildSynergyPreview(ItemDefinition item)
    {
        var tags = item.Tags.ToHashSet(StringComparer.OrdinalIgnoreCase);
        var lines = new List<string> { "[b]Shared Tags[/b]" };
        foreach (var asset in _items.Where(asset => !ReferenceEquals(asset.Item, item)))
        {
            var shared = asset.Item.Tags.Where(tag => tags.Contains(tag)).ToArray();
            if (shared.Length > 0)
            {
                lines.Add($"{Escape(asset.Item.DisplayName)}: {Escape(string.Join(", ", shared))}");
            }
        }

        if (lines.Count == 1)
        {
            lines.Add("none");
        }

        lines.Add(string.Empty);
        lines.Add("[b]Effect Targets[/b]");
        var targets = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var modifier in ItemEffectCompiler.BuildModifierSpecs(item.Effects))
        {
            targets.Add(EmptyAsAll(modifier.TargetTag));
        }

        foreach (var proc in ItemEffectCompiler.BuildProcSpecs(item.Effects))
        {
            targets.Add(EmptyAsAll(proc.TargetTag));
        }

        lines.Add(targets.Count == 0 ? "all" : Escape(string.Join(", ", targets.OrderBy(value => value))));
        return string.Join('\n', lines);
    }

    private string BuildCatalogSyncSummary()
    {
        var catalog = LoadEditorResource<ContentCatalog>(CatalogPath);
        if (catalog == null)
        {
            return "Catalog missing.";
        }

        var catalogIds = catalog.Items.Select(item => item.Id).ToHashSet(StringComparer.OrdinalIgnoreCase);
        var fileIds = _items.Select(asset => asset.Item.Id).ToHashSet(StringComparer.OrdinalIgnoreCase);
        var missingInCatalog = fileIds.Count(id => !catalogIds.Contains(id));
        var staleInCatalog = catalogIds.Count(id => !fileIds.Contains(id));
        return missingInCatalog == 0 && staleInCatalog == 0
            ? "Catalog is synced."
            : $"Catalog warning: {missingInCatalog} resource items not cataloged, {staleInCatalog} stale catalog refs. Save an item to resync the catalog.";
    }

    private void SetPreviewText(string text)
    {
        if (_cardPreview != null)
        {
            _cardPreview.Text = text;
        }

        if (_statPreview != null)
        {
            _statPreview.Text = text;
        }

        if (_procPreview != null)
        {
            _procPreview.Text = text;
        }

        if (_lootPreview != null)
        {
            _lootPreview.Text = text;
        }

        if (_synergyPreview != null)
        {
            _synergyPreview.Text = text;
        }

        if (_validationPreview != null)
        {
            _validationPreview.Text = text;
        }
    }

    private void RefreshFilterOptions()
    {
        _itemTagSelector?.Reload();
    }

    private void AddSelectedTagToCurrentItem()
    {
        if (_tags == null || _itemTagSelector == null)
        {
            return;
        }

        var tag = _itemTagSelector.SelectedSingleTag();
        if (string.IsNullOrWhiteSpace(tag))
        {
            SetStatus("Select a gameplay tag from the catalog tree first.");
            return;
        }

        var tags = ParseTags(_tags.Text);
        if (!tags.Contains(tag, StringComparer.OrdinalIgnoreCase))
        {
            tags.Add(tag);
            EnsureTagsInCatalog(new[] { tag });
            _tags.Text = string.Join(", ", tags);
        }

        OnFieldChanged();
        RefreshVisibleItemsPreservingSelection();
    }

    private void EnsureItemTagsInCatalog(ItemDefinition item)
    {
        var tags = new List<string>();
        tags.AddRange(item.Tags);
        foreach (var effect in item.Effects)
        {
            tags.Add(effect.TargetTag);
            foreach (var compiled in ItemEffectCompiler.Compile(effect))
            {
                tags.Add(compiled.TargetTag);
            }
        }

        EnsureTagsInCatalog(tags);
    }

    private static void EnsureTagsInCatalog(IEnumerable<string> tags)
    {
        var catalog = GameplayTagCatalogUtility.LoadDefaultCatalog();
        var changed = false;
        foreach (var tag in tags)
        {
            var normalized = GameplayTagPath.Normalize(tag);
            if (!GameplayTagPath.IsValid(normalized) || catalog.Contains(normalized))
            {
                continue;
            }

            catalog.EnsureTag(normalized);
            changed = true;
        }

        if (!changed)
        {
            return;
        }

        ResourceSaver.Save(catalog, GameplayTagCatalogUtility.DefaultCatalogPath);
        EditorInterface.Singleton.GetResourceFilesystem()?.Scan();
    }

    private static List<string> ParseTags(string tags)
    {
        var parsed = new List<string>();
        foreach (var tag in tags.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
        {
            var normalized = NormalizeOptionalTag(tag);
            if (!string.IsNullOrWhiteSpace(normalized) && !parsed.Contains(normalized, StringComparer.OrdinalIgnoreCase))
            {
                parsed.Add(normalized);
            }
        }

        return parsed;
    }

    private void RefreshAchievementOptions()
    {
        var selectedId = SelectedAchievementId(string.Empty);
        _achievementOptions.Clear();
        _achievementOptions.Add(new AchievementOption(string.Empty, "None"));

        const string achievementDirectory = "res://data/achievements";
        var loaded = new List<AchievementOption>();
        var dir = DirAccess.Open(achievementDirectory);
        if (dir != null)
        {
            dir.ListDirBegin();
            while (true)
            {
                var fileName = dir.GetNext();
                if (string.IsNullOrEmpty(fileName))
                {
                    break;
                }

                if (dir.CurrentIsDir() || !fileName.EndsWith(".tres", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                var path = $"{achievementDirectory}/{fileName}";
                var achievement = LoadEditorResource<AchievementDefinition>(path);
                if (achievement == null || string.IsNullOrWhiteSpace(achievement.Id))
                {
                    continue;
                }

                var name = string.IsNullOrWhiteSpace(achievement.DisplayName) ? achievement.Id : achievement.DisplayName;
                loaded.Add(new AchievementOption(achievement.Id, $"{name} ({achievement.Id})"));
            }

            dir.ListDirEnd();
        }

        _achievementOptions.AddRange(loaded.OrderBy(option => option.Label, StringComparer.OrdinalIgnoreCase));

        if (_unlockAchievement == null)
        {
            return;
        }

        RebuildAchievementOptionButton();

        SelectAchievementOption(selectedId);
    }

    private void SelectAchievementOption(string achievementId)
    {
        if (_unlockAchievement == null || _achievementOptions.Count == 0)
        {
            return;
        }

        var removedUnresolvedOptions = _achievementOptions.RemoveAll(option => option.IsUnresolved) > 0;
        var normalized = NormalizeOptionalId(achievementId);
        var index = _achievementOptions.FindIndex(option => string.Equals(option.Id, normalized, StringComparison.OrdinalIgnoreCase));
        if (index < 0 && !string.IsNullOrWhiteSpace(normalized))
        {
            _achievementOptions.Add(new AchievementOption(normalized, $"{normalized}{UnresolvedOptionSuffix}", true));
            index = _achievementOptions.Count - 1;
            removedUnresolvedOptions = true;
        }

        if (removedUnresolvedOptions)
        {
            RebuildAchievementOptionButton();
        }

        _unlockAchievement.Select(index >= 0 ? index : 0);
    }

    private void RebuildAchievementOptionButton()
    {
        if (_unlockAchievement == null)
        {
            return;
        }

        _unlockAchievement.Clear();
        foreach (var option in _achievementOptions)
        {
            _unlockAchievement.AddItem(option.Label);
        }
    }

    private string SelectedAchievementId(string fallback)
    {
        if (_unlockAchievement == null || _achievementOptions.Count == 0)
        {
            return NormalizeOptionalId(fallback);
        }

        var index = Math.Clamp(_unlockAchievement.Selected, 0, _achievementOptions.Count - 1);
        return _achievementOptions[index].Id;
    }

    private void RefreshVisibleItemsPreservingSelection()
    {
        var selectedId = _selected?.Item.Id;
        RefreshVisibleItems();
        if (selectedId == null || _itemList == null)
        {
            return;
        }

        for (var i = 0; i < _visibleItems.Count; i++)
        {
            if (string.Equals(_visibleItems[i].Item.Id, selectedId, StringComparison.OrdinalIgnoreCase))
            {
                _itemList.Select(i);
                break;
            }
        }
    }

    private static void ApplyEffectDefaults(ItemEffectSpec effect)
    {
        switch (effect.Kind)
        {
            case ItemEffectKind.AddMultiplyStat:
                effect.StatId = string.IsNullOrWhiteSpace(effect.StatId) ? "damage" : effect.StatId;
                effect.Value = effect.Value <= 1f ? 1.15f : effect.Value;
                break;
            case ItemEffectKind.AddSpellCount:
                effect.Value = effect.Value <= 0f ? 1f : MathF.Floor(effect.Value);
                break;
            case ItemEffectKind.KeepItIn:
                effect.Value = effect.Value <= 0f ? 4f : MathF.Floor(effect.Value);
                effect.SecondaryValue = effect.SecondaryValue <= 0f ? 1f : effect.SecondaryValue;
                break;
            case ItemEffectKind.AbyssalRing:
                effect.Value = effect.Value <= 0f ? 20f : effect.Value;
                effect.SecondaryValue = effect.SecondaryValue <= 0f ? 1.25f : effect.SecondaryValue;
                break;
            case ItemEffectKind.FaultyFocus:
                effect.Value = effect.Value <= 0f || effect.Value > 1f ? 0.5f : effect.Value;
                effect.SecondaryValue = effect.SecondaryValue < 1f ? 4f : MathF.Floor(effect.SecondaryValue);
                break;
            case ItemEffectKind.PreventNextPlayerHealthLoss:
                effect.Value = effect.Value < 1f ? 1f : MathF.Floor(effect.Value);
                break;
            case ItemEffectKind.HomingSpells:
                effect.Value = effect.Value <= 0f ? 1f : effect.Value;
                effect.SecondaryValue = effect.SecondaryValue <= 1f || effect.SecondaryValue > 180f ? 0f : effect.SecondaryValue;
                effect.TertiaryValue = effect.TertiaryValue < 0f ? 0f : effect.TertiaryValue;
                break;
            case ItemEffectKind.RemoveMultiplyStat:
                effect.StatId = string.IsNullOrWhiteSpace(effect.StatId) ? "damage" : effect.StatId;
                effect.Value = effect.Value <= 0f || effect.Value >= 1f ? 0.1f : effect.Value;
                break;
            case ItemEffectKind.FreezeWithFrostDamage:
            case ItemEffectKind.ImmolateWithFireDamage:
                effect.ChancePercent = effect.ChancePercent <= 0f ? 20f : effect.ChancePercent;
                effect.Value = effect.Value <= 0f ? 1f : effect.Value;
                break;
            case ItemEffectKind.EnemiesExplodeOnDeath:
            case ItemEffectKind.BurningGroundOnEnemyDeath:
                effect.ChancePercent = effect.ChancePercent <= 0f ? 25f : effect.ChancePercent;
                effect.Value = effect.Value <= 0f ? 8f : effect.Value;
                break;
            default:
                effect.Value = effect.Value == 0f ? 1f : effect.Value;
                break;
        }

        SanitizeEffectFields(effect);
    }

    private static void SanitizeEffectFields(ItemEffectSpec effect)
    {
        if (!Enum.IsDefined(effect.Kind))
        {
            return;
        }

        if (!EffectUsesStat(effect.Kind))
        {
            effect.StatId = string.Empty;
        }

        if (!EffectUsesChance(effect.Kind))
        {
            effect.ChancePercent = 0f;
        }

        if (!EffectUsesTarget(effect.Kind))
        {
            effect.TargetTag = string.Empty;
        }

        if (!EffectUsesValue(effect.Kind))
        {
            effect.Value = 1f;
        }

        if (!EffectUsesSecondaryValue(effect.Kind))
        {
            effect.SecondaryValue = 1f;
        }

        if (!EffectUsesTertiaryValue(effect.Kind))
        {
            effect.TertiaryValue = 0f;
        }
    }

    private static bool EffectUsesStat(ItemEffectKind kind)
    {
        return ItemEffectCompiler.RequiresFreeStat(kind);
    }

    private static bool EffectUsesValue(ItemEffectKind kind)
    {
        return kind is not ItemEffectKind.AddProjectilePierce
            and not ItemEffectKind.SplitProjectile;
    }

    private static bool EffectUsesSecondaryValue(ItemEffectKind kind)
    {
        return kind is ItemEffectKind.KeepItIn
            or ItemEffectKind.AbyssalRing
            or ItemEffectKind.FaultyFocus
            or ItemEffectKind.HomingSpells
            or ItemEffectKind.OrbitingSpells;
    }

    private static bool EffectUsesTertiaryValue(ItemEffectKind kind)
    {
        return kind is ItemEffectKind.HomingSpells
            or ItemEffectKind.OrbitingSpells;
    }

    private static bool EffectUsesChance(ItemEffectKind kind)
    {
        return kind is ItemEffectKind.FreezeWithFrostDamage
            or ItemEffectKind.ImmolateWithFireDamage
            or ItemEffectKind.EnemiesExplodeOnDeath
            or ItemEffectKind.BurningGroundOnEnemyDeath;
    }

    private static bool EffectUsesTarget(ItemEffectKind kind)
    {
        return ItemEffectCompiler.RequiresFreeStat(kind)
            || kind is ItemEffectKind.EnemiesExplodeOnDeath
                or ItemEffectKind.BurningGroundOnEnemyDeath
                or ItemEffectKind.HomingSpells
                or ItemEffectKind.OrbitingSpells;
    }

    private static string EffectValueLabel(ItemEffectKind kind)
    {
        return kind switch
        {
            ItemEffectKind.RemoveMultiplyStat => "Reduction",
            ItemEffectKind.AddSpellCount => "Extra Spell Count",
            ItemEffectKind.KeepItIn => "Extra Spell Count",
            ItemEffectKind.AbyssalRing => "Damage Multiplier",
            ItemEffectKind.FaultyFocus => "Child Damage Multiplier",
            ItemEffectKind.PreventNextPlayerHealthLoss => "Protected Hits",
            ItemEffectKind.HomingSpells => "Turn Strength",
            ItemEffectKind.OrbitingSpells => "Orbit Strength",
            ItemEffectKind.FreezeWithFrostDamage or ItemEffectKind.ImmolateWithFireDamage => "Duration",
            ItemEffectKind.EnemiesExplodeOnDeath or ItemEffectKind.BurningGroundOnEnemyDeath => "Power",
            _ => "Multiplier"
        };
    }

    private static double EffectValueMin(ItemEffectKind kind)
    {
        if (kind == ItemEffectKind.KeepItIn)
        {
            return 1;
        }

        if (kind == ItemEffectKind.AbyssalRing)
        {
            return 0.1;
        }

        if (kind == ItemEffectKind.FaultyFocus)
        {
            return 0.01;
        }

        if (kind == ItemEffectKind.PreventNextPlayerHealthLoss)
        {
            return 1;
        }

        if (kind == ItemEffectKind.HomingSpells)
        {
            return 0.1;
        }

        if (kind == ItemEffectKind.OrbitingSpells)
        {
            return 0.1;
        }

        return kind is ItemEffectKind.AddSpellCount
            or ItemEffectKind.RemoveMultiplyStat
            ? 0
            : -1000;
    }

    private static double EffectValueMax(ItemEffectKind kind)
    {
        if (kind == ItemEffectKind.KeepItIn)
        {
            return 12;
        }

        if (kind == ItemEffectKind.AbyssalRing)
        {
            return 1000;
        }

        if (kind == ItemEffectKind.FaultyFocus)
        {
            return 1;
        }

        if (kind == ItemEffectKind.PreventNextPlayerHealthLoss)
        {
            return 99;
        }

        if (kind == ItemEffectKind.HomingSpells)
        {
            return 10;
        }

        if (kind == ItemEffectKind.OrbitingSpells)
        {
            return 10;
        }

        return kind is ItemEffectKind.RemoveMultiplyStat
            ? 1
            : 1000;
    }

    private static string SecondaryEffectValueLabel(ItemEffectKind kind)
    {
        return kind switch
        {
            ItemEffectKind.KeepItIn => "Base Charge Seconds",
            ItemEffectKind.AbyssalRing => "Base Charge Seconds",
            ItemEffectKind.FaultyFocus => "Max Generations",
            ItemEffectKind.HomingSpells => "Detection Cone Angle (deg, 0 = default)",
            ItemEffectKind.OrbitingSpells => "Orbit Radius (m, 0 = default)",
            _ => "Secondary Value"
        };
    }

    private static double SecondaryEffectValueMin(ItemEffectKind kind)
    {
        if (kind == ItemEffectKind.FaultyFocus)
        {
            return 1;
        }

        if (kind == ItemEffectKind.HomingSpells || kind == ItemEffectKind.OrbitingSpells)
        {
            return 0;
        }

        return kind is ItemEffectKind.KeepItIn or ItemEffectKind.AbyssalRing ? 0.05 : -1000;
    }

    private static double SecondaryEffectValueMax(ItemEffectKind kind)
    {
        if (kind == ItemEffectKind.FaultyFocus)
        {
            return 8;
        }

        if (kind == ItemEffectKind.HomingSpells)
        {
            return 180;
        }

        if (kind == ItemEffectKind.OrbitingSpells)
        {
            return 20;
        }

        return kind is ItemEffectKind.KeepItIn or ItemEffectKind.AbyssalRing ? 10 : 1000;
    }

    private static string TertiaryEffectValueLabel(ItemEffectKind kind)
    {
        return kind switch
        {
            ItemEffectKind.HomingSpells => "Detection Width (m, 0 = default)",
            ItemEffectKind.OrbitingSpells => "Angular Speed (rad/s, 0 = default)",
            _ => "Third Value"
        };
    }

    private static double TertiaryEffectValueMin(ItemEffectKind kind)
    {
        return kind is ItemEffectKind.HomingSpells or ItemEffectKind.OrbitingSpells ? 0 : -1000;
    }

    private static double TertiaryEffectValueMax(ItemEffectKind kind)
    {
        return kind is ItemEffectKind.HomingSpells ? 100 : kind == ItemEffectKind.OrbitingSpells ? 12 : 1000;
    }

    private static string FixedValueLabel(ItemEffectKind kind)
    {
        return kind switch
        {
            ItemEffectKind.AddProjectilePierce => "Pierce enabled",
            ItemEffectKind.SplitProjectile => "+1 split projectile",
            _ => "Fixed by effect"
        };
    }

    private static string EffectTargetPlaceholder(ItemEffectKind kind)
    {
        return kind switch
        {
            ItemEffectKind.EnemiesExplodeOnDeath or ItemEffectKind.BurningGroundOnEnemyDeath => "empty = all kills",
            ItemEffectKind.HomingSpells => "empty = all homing-compatible spells, or use spell.delivery.projectile / beam / area",
            ItemEffectKind.OrbitingSpells => "empty = all orbit-compatible spells, or use spell.delivery.projectile / beam / area",
            _ => "empty = all matching effects"
        };
    }

    private static string FixedTargetLabel(ItemEffectKind kind)
    {
        return kind switch
        {
            ItemEffectKind.AddProjectilePierce => "all projectile spells",
            ItemEffectKind.SplitProjectile => "spell.delivery.projectile",
            ItemEffectKind.FreezeWithFrostDamage => "combat.damage.frost",
            ItemEffectKind.ImmolateWithFireDamage => "combat.damage.fire",
            ItemEffectKind.AddSpellCount => "all spells",
            ItemEffectKind.KeepItIn => "all compatible spells",
            ItemEffectKind.AbyssalRing => "all compatible spells",
            ItemEffectKind.FaultyFocus => "all spell hits",
            ItemEffectKind.PreventNextPlayerHealthLoss => "player health damage",
            ItemEffectKind.HomingSpells => "all homing-compatible spells",
            ItemEffectKind.OrbitingSpells => "all orbit-compatible spells",
            _ => "fixed by effect"
        };
    }

    private static ItemDefinition CloneItem(ItemDefinition source)
    {
        var item = new ItemDefinition
        {
            ItemNumber = source.ItemNumber,
            Id = source.Id,
            DisplayName = source.DisplayName,
            Description = source.Description,
            HiddenDescription = source.HiddenDescription,
            RevealedStatText = source.RevealedStatText,
            RevealedBehaviorText = source.RevealedBehaviorText,
            RevealedEffectText = source.RevealedEffectText,
            Icon = source.Icon,
            EnemyFormPresentationScene = source.EnemyFormPresentationScene,
            UnlockAchievementId = source.UnlockAchievementId,
            IsFlavorOnly = source.IsFlavorOnly,
            Kind = source.Kind,
            HasUnlimitedActivations = source.HasUnlimitedActivations,
            MaxActivations = source.MaxActivations
        };

        foreach (var tag in source.Tags)
        {
            item.Tags.Add(tag);
        }

        foreach (var poolWeight in source.PoolWeights)
        {
            if (poolWeight == null)
            {
                continue;
            }

            item.PoolWeights.Add(new ItemPoolWeightDefinition
            {
                PoolId = poolWeight.PoolId,
                Weight = poolWeight.Weight
            });
        }

        foreach (var effect in source.Effects)
        {
            item.Effects.Add(new ItemEffectSpec
            {
                Kind = effect.Kind,
                StatId = effect.StatId,
                Value = effect.Value,
                SecondaryValue = effect.SecondaryValue,
                TertiaryValue = effect.TertiaryValue,
                ChancePercent = effect.ChancePercent,
                Priority = effect.Priority,
                TargetTag = effect.TargetTag
            });
        }

        foreach (var effect in source.ActiveEffects)
        {
            item.ActiveEffects.Add(new EffectSpec
            {
                EffectType = effect.EffectType,
                Value = effect.Value,
                Radius = effect.Radius,
                DurationSeconds = effect.DurationSeconds,
                DurationScaling = effect.DurationScaling,
                TargetStatId = effect.TargetStatId
            });
        }

        return item;
    }

    private int NextItemNumber()
    {
        return _items.Count == 0 ? 1 : _items.Max(asset => Math.Max(0, asset.Item.ItemNumber)) + 1;
    }

    private static int CompareItems(ItemAsset left, ItemAsset right)
    {
        var leftNumber = left.Item.ItemNumber <= 0 ? int.MaxValue : left.Item.ItemNumber;
        var rightNumber = right.Item.ItemNumber <= 0 ? int.MaxValue : right.Item.ItemNumber;
        var number = leftNumber.CompareTo(rightNumber);
        return number != 0 ? number : string.Compare(left.Item.DisplayName, right.Item.DisplayName, StringComparison.OrdinalIgnoreCase);
    }

    private static void FillEnumOptions<TEnum>(OptionButton option)
        where TEnum : struct, Enum
    {
        option.Clear();
        foreach (var value in Enum.GetValues<TEnum>())
        {
            option.AddItem(value.ToString(), Convert.ToInt32(value, CultureInfo.InvariantCulture));
        }
    }

    private static void SelectEnumOption<TEnum>(OptionButton option, TEnum value)
        where TEnum : struct, Enum
    {
        var id = Convert.ToInt32(value, CultureInfo.InvariantCulture);
        for (var i = 0; i < option.ItemCount; i++)
        {
            if (option.GetItemId(i) == id)
            {
                option.Select(i);
                return;
            }
        }

        option.AddItem($"{value}{UnresolvedOptionSuffix}", id);
        option.Select(option.ItemCount - 1);
    }

    private static TEnum GetSelectedEnumValue<TEnum>(OptionButton option, int index, TEnum fallback)
        where TEnum : struct, Enum
    {
        if (index < 0 || index >= option.ItemCount)
        {
            return fallback;
        }

        var id = option.GetItemId(index);
        return Enum.IsDefined(typeof(TEnum), id)
            ? (TEnum)Enum.ToObject(typeof(TEnum), id)
            : fallback;
    }

    private static IReadOnlyList<string> FillStatOptions(OptionButton option, string selected)
    {
        return FillStringOptionsPreservingSelected(option, KnownStats, selected);
    }

    private static IReadOnlyList<string> FillItemPoolOptions(OptionButton option, string selected)
    {
        return FillStringOptionsPreservingSelected(option, ItemPoolIds.All, selected);
    }

    private static IReadOnlyList<string> FillStringOptionsPreservingSelected(OptionButton option, IEnumerable<string> knownValues, string selected)
    {
        option.Clear();
        var values = new List<string>();
        var selectedValue = selected?.Trim() ?? string.Empty;
        var selectedIndex = -1;

        foreach (var dropdownOption in AuthoringDropdownOptions.BuildPreservingSelected(knownValues, selectedValue))
        {
            values.Add(dropdownOption.Value);
            option.AddItem(dropdownOption.Label);
            if (string.Equals(dropdownOption.Value, selectedValue, StringComparison.OrdinalIgnoreCase))
            {
                selectedIndex = values.Count - 1;
            }
        }

        if (option.ItemCount > 0)
        {
            option.Select(selectedIndex >= 0 ? selectedIndex : 0);
        }

        return values;
    }

    private static string SelectedStringOptionValue(IReadOnlyList<string> values, long selectedIndex, string fallback)
    {
        var index = (int)selectedIndex;
        return index >= 0 && index < values.Count ? values[index] : fallback?.Trim() ?? string.Empty;
    }

    private static bool MatchesSearch(ItemDefinition item, string search)
    {
        if (string.IsNullOrWhiteSpace(search))
        {
            return true;
        }

        return item.Id.Contains(search, StringComparison.OrdinalIgnoreCase)
            || item.DisplayName.Contains(search, StringComparison.OrdinalIgnoreCase)
            || item.Tags.Any(tag => tag.Contains(search, StringComparison.OrdinalIgnoreCase));
    }

    private static void ReplaceTags(ItemDefinition item, string tags)
    {
        item.Tags.Clear();
        foreach (var tag in ParseTags(tags))
        {
            item.Tags.Add(tag);
        }
    }

    private static string NormalizeId(string value)
    {
        return NormalizeToken(value, "item", allowDots: false);
    }

    private static string NormalizeOptionalId(string value)
    {
        return NormalizeToken(value, string.Empty, allowDots: false);
    }

    private static string NormalizeOptionalTag(string value)
    {
        return NormalizeToken(value, string.Empty, allowDots: true);
    }

    private static string NormalizeToken(string value, string fallback, bool allowDots)
    {
        var normalized = new string((value ?? string.Empty)
            .Trim()
            .ToLowerInvariant()
            .Select(character => char.IsLetterOrDigit(character) || character == '_' || allowDots && character == '.'
                ? character
                : '_')
            .ToArray());
        while (normalized.Contains("__", StringComparison.Ordinal))
        {
            normalized = normalized.Replace("__", "_", StringComparison.Ordinal);
        }

        while (normalized.Contains("..", StringComparison.Ordinal))
        {
            normalized = normalized.Replace("..", ".", StringComparison.Ordinal);
        }

        normalized = normalized.Trim('_', '.');
        return string.IsNullOrWhiteSpace(normalized) ? fallback : normalized;
    }

    private static string NumberText(ItemDefinition item)
    {
        return item.ItemNumber <= 0 ? "###" : item.ItemNumber.ToString("000");
    }

    private static string PoolWeightsText(ItemDefinition item)
    {
        return item.PoolWeights.Count == 0
            ? "-"
            : string.Join(", ", item.PoolWeights
                .Where(poolWeight => poolWeight != null)
                .Select(poolWeight => $"{poolWeight.PoolId}:{WeightText(poolWeight.Weight)}"));
    }

    private static float PoolWeight(ItemDefinition item, string poolId)
    {
        foreach (var poolWeight in item.PoolWeights)
        {
            if (poolWeight != null && string.Equals(poolWeight.PoolId, poolId, StringComparison.OrdinalIgnoreCase))
            {
                return poolWeight.Weight;
            }
        }

        return 0f;
    }

    private static string WeightText(float weight)
    {
        return weight.ToString("0.##", CultureInfo.InvariantCulture);
    }

    private float NightMarketShare(ItemDefinition item)
    {
        var total = TotalPoolWeight(ItemPoolIds.NightMarket);
        return total <= 0f ? 0f : MathF.Max(0f, PoolWeight(item, ItemPoolIds.NightMarket)) * 100f / total;
    }

    private string NightMarketShareText(ItemDefinition item)
    {
        return $"{NightMarketShare(item):0.##}%";
    }

    private float TotalPoolWeight(string poolId)
    {
        return _items
            .Sum(asset => MathF.Max(0f, PoolWeight(asset.Item, poolId)));
    }

    private static int EffectiveItemNumber(ItemDefinition item)
    {
        return item.ItemNumber <= 0 ? int.MaxValue : item.ItemNumber;
    }

    private static string ItemEffectsSummary(ItemDefinition item)
    {
        return item.Effects.Count == 0 ? "-" : string.Join(" | ", item.Effects.Select(ItemEffectCompiler.Describe));
    }

    private static string EmptyAsAll(string value)
    {
        return string.IsNullOrWhiteSpace(value) ? "all" : value;
    }

    private static string Escape(string text)
    {
        return (text ?? string.Empty).Replace("[", "(").Replace("]", ")");
    }

    private void SetStatus(string text)
    {
        if (_status?.Text == text)
        {
            return;
        }

        if (_status != null)
        {
            _status.Text = text;
        }

        GD.Print($"TLM Item Authoring: {text}");
    }

    private static bool TryExtractFirstImagePath(Variant data, out string path)
    {
        path = string.Empty;
        var paths = new List<string>();
        if (data.VariantType == Variant.Type.Dictionary)
        {
            var dictionary = data.AsGodotDictionary();
            if (dictionary.ContainsKey("files"))
            {
                AddVariantPaths(dictionary["files"], paths);
            }

            if (dictionary.ContainsKey("paths"))
            {
                AddVariantPaths(dictionary["paths"], paths);
            }
        }
        else
        {
            AddVariantPaths(data, paths);
        }

        path = paths.FirstOrDefault(IsImagePath) ?? string.Empty;
        return !string.IsNullOrWhiteSpace(path);
    }

    private static void AddVariantPaths(Variant value, List<string> paths)
    {
        if (value.VariantType == Variant.Type.PackedStringArray)
        {
            foreach (var entry in value.AsStringArray())
            {
                paths.Add(entry);
            }
        }
        else if (value.VariantType == Variant.Type.Array)
        {
            foreach (var entry in value.AsGodotArray())
            {
                paths.Add(entry.ToString());
            }
        }
        else
        {
            paths.Add(value.ToString());
        }
    }

    private static bool IsImagePath(string path)
    {
        return path.EndsWith(".png", StringComparison.OrdinalIgnoreCase)
            || path.EndsWith(".jpg", StringComparison.OrdinalIgnoreCase)
            || path.EndsWith(".jpeg", StringComparison.OrdinalIgnoreCase)
            || path.EndsWith(".webp", StringComparison.OrdinalIgnoreCase);
    }

    private static string NormalizeAbsolutePath(string path)
    {
        return System.IO.Path.GetFullPath(path)
            .TrimEnd(System.IO.Path.DirectorySeparatorChar, System.IO.Path.AltDirectorySeparatorChar);
    }

    private static bool IsInsideDirectory(string absolutePath, string absoluteDirectory)
    {
        var normalizedDirectory = absoluteDirectory
            .TrimEnd(System.IO.Path.DirectorySeparatorChar, System.IO.Path.AltDirectorySeparatorChar)
            + System.IO.Path.DirectorySeparatorChar;
        return absolutePath.StartsWith(normalizedDirectory, StringComparison.OrdinalIgnoreCase);
    }

    private static void ScanEditorFileSystem()
    {
        var filesystem = EditorInterface.Singleton.GetResourceFilesystem();
        if (filesystem == null || filesystem.IsScanning() || filesystem.IsImporting())
        {
            return;
        }

        filesystem.Scan();
    }

    private void LogForensics(string stage, string details)
    {
        var eventId = ++_diagnosticEventIndex;
        var message = $"TLM Item Authoring Forensics event={eventId} [{stage}]: {details}";
        GD.Print(message);

        try
        {
            var path = ProjectSettings.GlobalizePath(ForensicLogPath);
            var directory = System.IO.Path.GetDirectoryName(path);
            if (!string.IsNullOrWhiteSpace(directory))
            {
                System.IO.Directory.CreateDirectory(directory);
            }

            System.IO.File.AppendAllText(path, $"{DateTimeOffset.Now:O} {message}{System.Environment.NewLine}");
            WriteCrashBreadcrumb(eventId, stage, details);
        }
        catch (Exception exception)
        {
            GD.PushWarning($"TLM Item Authoring: Could not write forensic log: {exception.Message}");
        }
    }

    private void WriteCrashBreadcrumb(int eventId, string stage, string details)
    {
        var breadcrumb =
            $"TLM Item Authoring Crash Breadcrumb\n" +
            $"event={eventId}\n" +
            $"time={DateTimeOffset.Now:O}\n" +
            $"session={_diagnosticSessionId}\n" +
            $"processId={System.Environment.ProcessId}\n" +
            $"managedThreadId={System.Environment.CurrentManagedThreadId}\n" +
            $"stage={stage}\n" +
            $"details={SafeOneLine(Truncate(details, 6000))}\n" +
            BuildCompactBreadcrumbState();

        WriteReportFile(CrashBreadcrumbPath, breadcrumb, append: false);
    }

    private void WriteSaveDiagnosticReport(string stage, string details, bool result)
    {
        var report =
            $"TLM Item Authoring Save Diagnostic\n" +
            $"stage={stage}\n" +
            $"result={result}\n" +
            $"time={DateTimeOffset.Now:O}\n" +
            $"session={_diagnosticSessionId}\n" +
            $"saveMode=manual_explicit\n" +
            $"{details}\n";

        WriteReportFile(LastSaveReportPath, report, append: false);
        var timestamp = DateTimeOffset.Now.ToString("yyyyMMdd_HHmmss_fff", CultureInfo.InvariantCulture);
        WriteReportFile($"{SaveReportDirectory}/item_authoring_save_{timestamp}_{SanitizeReportName(stage)}.log", report, append: false);
        LogForensics("SAVE_DIAGNOSTIC_REPORT", $"stage={stage} result={result} latest={LastSaveReportPath}");
    }

    private static void WriteReportFile(string resourcePath, string text, bool append)
    {
        try
        {
            var path = ProjectSettings.GlobalizePath(resourcePath);
            var directory = System.IO.Path.GetDirectoryName(path);
            if (!string.IsNullOrWhiteSpace(directory))
            {
                System.IO.Directory.CreateDirectory(directory);
            }

            if (append)
            {
                System.IO.File.AppendAllText(path, text);
            }
            else
            {
                System.IO.File.WriteAllText(path, text);
            }
        }
        catch (Exception exception)
        {
            GD.PushWarning($"TLM Item Authoring: Could not write diagnostic report '{resourcePath}': {exception.Message}");
        }
    }

    private string BuildDiagnosticState(string reason)
    {
        return string.Join('\n', new[]
        {
            $"reason={reason}",
            $"pending={_hasPendingChanges}",
            $"stableIdEditDirty={_stableIdEditDirty}",
            $"dirtyItems=[{string.Join(", ", _dirtyItemPaths)}]",
            $"autoSaveQueued={_autoSaveQueued}",
            $"loadingFields={_loadingFields}",
            $"savingFields={_savingFields}",
            $"saveMode=manual_explicit",
            $"selected={DescribeItemAsset(_selected)}",
            $"selectedItem={BuildItemDebugSnapshot(_selected?.Item)}",
            $"uiFields={BuildUiFieldSnapshot()}",
            $"selectedFile={(_selected == null ? "none" : BuildFileMetadata(_selected.Path))}",
            $"catalogFile={BuildFileMetadata(CatalogPath)}",
            $"loadedItems=[{BuildLoadedItemSnapshot()}]",
            $"diskItems=[{BuildDiskItemSnapshot()}]",
            $"catalogItems=[{BuildCatalogSnapshot()}]"
        });
    }

    private string BuildCompactBreadcrumbState()
    {
        return string.Join('\n', new[]
        {
            $"pending={_hasPendingChanges}",
            $"stableIdEditDirty={_stableIdEditDirty}",
            $"dirtyItems=[{string.Join(", ", _dirtyItemPaths)}]",
            $"autoSaveQueued={_autoSaveQueued}",
            $"loadingFields={_loadingFields}",
            $"savingFields={_savingFields}",
            $"saveMode=manual_explicit",
            $"selected={DescribeItemAsset(_selected)}",
            $"selectedItem={BuildItemDebugSnapshot(_selected?.Item)}",
            $"uiFields={BuildUiFieldSnapshot()}",
            $"selectedFile={(_selected == null ? "none" : BuildFileMetadata(_selected.Path))}",
            $"catalogFile={BuildFileMetadata(CatalogPath)}",
            string.Empty
        });
    }

    private string BuildUiFieldSnapshot()
    {
        return
            $"number={_itemNumber?.Value.ToString("0.###", CultureInfo.InvariantCulture) ?? "null"} " +
            $"id='{SafeOneLine(_id?.Text ?? string.Empty)}' " +
            $"name='{SafeOneLine(_name?.Text ?? string.Empty)}' " +
            $"kindIndex={_itemKind?.Selected.ToString(CultureInfo.InvariantCulture) ?? "null"} " +
            $"tags='{SafeOneLine(_tags?.Text ?? string.Empty)}' " +
            $"hidden={TextSummary(_hiddenDescription?.Text ?? string.Empty)} " +
            $"description={TextSummary(_description?.Text ?? string.Empty)} " +
            $"revealedStats={TextSummary(_revealedStatText?.Text ?? string.Empty)} " +
            $"revealedBehavior={TextSummary(_revealedBehaviorText?.Text ?? string.Empty)}";
    }

    private static string BuildItemDebugSnapshot(ItemDefinition? item)
    {
        if (item == null)
        {
            return "none";
        }

        return
            $"id='{SafeOneLine(item.Id)}' " +
            $"number={item.ItemNumber} " +
            $"name='{SafeOneLine(item.DisplayName)}' " +
            $"kind={item.Kind} " +
            $"resourcePath='{SafeOneLine(item.ResourcePath)}' " +
            $"tags='{SafeOneLine(string.Join(",", item.Tags))}' " +
            $"pools='{SafeOneLine(string.Join(",", item.PoolWeights.Where(pool => pool != null).Select(pool => $"{pool.PoolId}:{pool.Weight:0.###}")))}' " +
            $"hidden={TextSummary(item.HiddenDescription)} " +
            $"description={TextSummary(item.Description)} " +
            $"revealedStats={TextSummary(item.RevealedStatText)} " +
            $"revealedBehavior={TextSummary(item.RevealedBehaviorText)} " +
            $"revealed={TextSummary(item.RevealedEffectText)} " +
            $"effects='{SafeOneLine(string.Join(" | ", item.Effects.Select(DescribeEffectForDiagnostics)))}' " +
            $"activeEffects='{SafeOneLine(string.Join(" | ", item.ActiveEffects.Select(effect => $"{effect.EffectType}:{effect.Value:0.###}:{effect.Radius:0.###}:{effect.DurationSeconds:0.###}")))}'";
    }

    private static string DescribeEffectForDiagnostics(ItemEffectSpec? effect)
    {
        if (effect == null)
        {
            return "null";
        }

        return $"{effect.Kind}:stat={effect.StatId}:value={effect.Value:0.###}:secondary={effect.SecondaryValue:0.###}:tertiary={effect.TertiaryValue:0.###}:chance={effect.ChancePercent:0.###}:target={effect.TargetTag}";
    }

    private static string BuildFileMetadata(string resourcePath)
    {
        try
        {
            var path = ProjectSettings.GlobalizePath(resourcePath);
            var file = new System.IO.FileInfo(path);
            return file.Exists
                ? $"resource='{resourcePath}' absolute='{SafeOneLine(file.FullName)}' exists=true length={file.Length} lastWriteUtc={file.LastWriteTimeUtc:O} readOnly={file.IsReadOnly}"
                : $"resource='{resourcePath}' absolute='{SafeOneLine(file.FullName)}' exists=false";
        }
        catch (Exception exception)
        {
            return $"resource='{resourcePath}' metadataError='{SafeOneLine(exception.Message)}'";
        }
    }

    private static string FormatValidationIssues(ValidationResult result)
    {
        return result.Issues.Count == 0
            ? "none"
            : SafeOneLine(string.Join(" || ", result.Issues.Take(12).Select(issue => $"{issue.Severity}:{issue.Code}:{issue.Message}")));
    }

    private static string TextSummary(string value)
    {
        return $"len={value.Length} text='{SafeOneLine(Truncate(value, 180))}'";
    }

    private static string OneLineText(string value)
    {
        var normalized = value
            .Replace("\r\n", "\n", StringComparison.Ordinal)
            .Replace('\r', '\n')
            .Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        var text = string.Join(" | ", normalized).Trim();
        return string.IsNullOrWhiteSpace(text) ? "-" : text;
    }

    private static string SafeOneLine(string value)
    {
        return value.Replace("\r", "\\r").Replace("\n", "\\n").Replace("'", "\\'");
    }

    private static string Truncate(string value, int maxLength)
    {
        return value.Length <= maxLength ? value : value[..maxLength] + "...";
    }

    private static string SanitizeReportName(string value)
    {
        var characters = value.Select(character => char.IsLetterOrDigit(character) ? character : '_').ToArray();
        return new string(characters).Trim('_').ToLowerInvariant();
    }

    private string BuildLoadedItemSnapshot()
    {
        return string.Join(" | ", _items.Select(DescribeItemAsset));
    }

    private string BuildCatalogSnapshot()
    {
        var catalog = LoadEditorResource<ContentCatalog>(CatalogPath);
        if (catalog == null)
        {
            return "catalog=null";
        }

        return string.Join(" | ", catalog.Items.Select(item => $"{item.Id}@{item.ResourcePath}"));
    }

    private static string BuildDiskItemSnapshot()
    {
        try
        {
            var root = ProjectSettings.GlobalizePath(ItemDirectory);
            if (!System.IO.Directory.Exists(root))
            {
                return $"missing:{root}";
            }

            return string.Join(" | ", System.IO.Directory
                .GetFiles(root, "*.tres")
                .OrderBy(path => path, StringComparer.OrdinalIgnoreCase)
                .Select(path =>
                {
                    var info = new System.IO.FileInfo(path);
                    return $"{info.Name}:created={info.CreationTime:HH:mm:ss.fff}:modified={info.LastWriteTime:HH:mm:ss.fff}:bytes={info.Length}";
                }));
        }
        catch (Exception exception)
        {
            return $"snapshot-error:{exception.Message}";
        }
    }

    private static string DescribeItemAsset(ItemAsset? asset)
    {
        if (asset == null)
        {
            return "null";
        }

        return $"{asset.Item.Id}@path={asset.Path}@resourcePath={asset.Item.ResourcePath}@name={asset.Item.DisplayName}";
    }

    private static T? LoadEditorResource<T>(
        string path,
        ResourceLoader.CacheMode cacheMode = ResourceLoader.CacheMode.Reuse)
        where T : Resource
    {
        try
        {
            return ResourceLoader.Load(path, string.Empty, cacheMode) as T;
        }
        catch (Exception exception)
        {
            GD.PushWarning($"TLM Item Authoring: Could not load {path} as {typeof(T).Name}: {exception.Message}");
            return null;
        }
    }

    private static PackedScene? LoadOptionalPackedScene(string? path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            return null;
        }

        return LoadEditorResource<PackedScene>(path.Trim(), ResourceLoader.CacheMode.Replace);
    }

    private static void DetachVerificationItem(ItemDefinition? item)
    {
        if (item == null)
        {
            return;
        }

        item.SetPathCache(string.Empty);
        foreach (var poolWeight in item.PoolWeights)
        {
            poolWeight?.SetPathCache(string.Empty);
        }

        foreach (var effect in item.Effects)
        {
            effect?.SetPathCache(string.Empty);
        }

        foreach (var effect in item.ActiveEffects)
        {
            effect?.SetPathCache(string.Empty);
        }
    }

    private sealed record ItemAsset(string Path, ItemDefinition Item);
    private sealed record AchievementOption(string Id, string Label, bool IsUnresolved = false);
}
#endif
