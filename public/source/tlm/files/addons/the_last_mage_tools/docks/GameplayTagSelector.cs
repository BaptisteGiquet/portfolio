#if TOOLS
#nullable enable
using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Tags;
using TheLastMage.Foundation;

namespace TheLastMage.EditorTools;

[Tool]
public partial class GameplayTagSelector : VBoxContainer
{
    private readonly Dictionary<string, TreeItem> _itemsByPath = new(StringComparer.OrdinalIgnoreCase);
    private LineEdit? _search;
    private Tree? _tree;
    private bool _suppress;

    public bool MultiSelect { get; set; } = true;

    public bool IncludeClearOption { get; set; }

    public event Action? SelectionChanged;

    public override void _Ready()
    {
        AddThemeConstantOverride("separation", 3);
        _search = new LineEdit
        {
            PlaceholderText = "Search gameplay tags",
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        _search.TextChanged += _ => Reload();
        AddChild(_search);

        _tree = new Tree
        {
            HideRoot = true,
            CustomMinimumSize = new Vector2(0f, 150f),
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            SelectMode = MultiSelect ? Tree.SelectModeEnum.Multi : Tree.SelectModeEnum.Single
        };
        _tree.ItemSelected += OnTreeSelectionChanged;
        AddChild(_tree);
        Reload();
    }

    public IReadOnlyList<string> SelectedTags()
    {
        if (_tree == null)
        {
            return Array.Empty<string>();
        }

        var selected = new List<string>();
        foreach (var pair in _itemsByPath)
        {
            if (pair.Value.IsSelected(0))
            {
                selected.Add(pair.Key);
            }
        }

        selected.Sort(StringComparer.OrdinalIgnoreCase);
        return selected;
    }

    public string SelectedSingleTag()
    {
        return SelectedTags().FirstOrDefault() ?? string.Empty;
    }

    public void SetSelectedTags(IEnumerable<string> tags)
    {
        _suppress = true;
        try
        {
            var normalized = tags
                .Select(GameplayTagPath.Normalize)
                .Where(GameplayTagPath.IsValid)
                .ToHashSet(StringComparer.OrdinalIgnoreCase);

            foreach (var pair in _itemsByPath)
            {
                if (normalized.Contains(pair.Key))
                {
                    pair.Value.Select(0);
                }
                else
                {
                    pair.Value.Deselect(0);
                }
            }
        }
        finally
        {
            _suppress = false;
        }
    }

    public void SetSelectedSingleTag(string tag)
    {
        SetSelectedTags(string.IsNullOrWhiteSpace(tag) ? Array.Empty<string>() : new[] { tag });
    }

    public void Reload()
    {
        if (_tree == null)
        {
            return;
        }

        _tree.SelectMode = MultiSelect ? Tree.SelectModeEnum.Multi : Tree.SelectModeEnum.Single;
        _tree.Clear();
        _itemsByPath.Clear();
        var root = _tree.CreateItem();
        var catalog = GameplayTagCatalogUtility.LoadDefaultCatalog();
        var search = _search?.Text.Trim() ?? string.Empty;

        if (IncludeClearOption && string.IsNullOrWhiteSpace(search))
        {
            var clear = _tree.CreateItem(root);
            clear.SetText(0, "all / no target tag");
            _itemsByPath[string.Empty] = clear;
        }

        foreach (var tag in catalog.OrderedTags())
        {
            var path = GameplayTagPath.Normalize(tag.Path);
            if (!string.IsNullOrWhiteSpace(search)
                && !path.Contains(search, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            var parentPath = GameplayTagPath.GetParent(path);
            var parent = !string.IsNullOrWhiteSpace(parentPath) && _itemsByPath.TryGetValue(parentPath, out var parentItem)
                ? parentItem
                : root;

            var item = _tree.CreateItem(parent);
            item.SetText(0, path);
            item.SetMetadata(0, path);
            if (tag.IsDeprecated)
            {
                item.SetCustomColor(0, Colors.Orange);
            }

            _itemsByPath[path] = item;
        }
    }

    private void OnTreeSelectionChanged()
    {
        if (_suppress)
        {
            return;
        }

        if (!MultiSelect && _tree?.GetSelected() is { } selected)
        {
            var path = selected.GetMetadata(0).AsString();
            foreach (var pair in _itemsByPath)
            {
                if (string.Equals(pair.Key, path, StringComparison.OrdinalIgnoreCase))
                {
                    pair.Value.Select(0);
                }
                else
                {
                    pair.Value.Deselect(0);
                }
            }
        }

        SelectionChanged?.Invoke();
    }
}
#endif
