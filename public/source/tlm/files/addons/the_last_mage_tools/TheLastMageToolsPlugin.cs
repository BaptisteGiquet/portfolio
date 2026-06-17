#if TOOLS
#nullable enable
using Godot;
using TheLastMage.EditorTools;

[Tool]
public partial class TheLastMageToolsPlugin : EditorPlugin
{
    private EditorDock? _dock;
    private ItemAuthoringDock? _dockContent;
    private EditorDock? _tagDock;
    private GameplayTagEditorDock? _tagDockContent;
    private EditorDock? _balanceDock;
    private BalanceLabDock? _balanceDockContent;
    private EditorDock? _localizationDock;
    private LocalizationDock? _localizationDockContent;
    private AcceptDialog? _playGuardDialog;
    private string _lastExternalSaveFailure = string.Empty;

    public override void _EnterTree()
    {
        CleanupStaleEditorUi();
        _dockContent = new ItemAuthoringDock();
        _dock = new EditorDock
        {
            Name = "TLMItemAuthoringDock",
            Title = "TLM Items",
            LayoutKey = "tlm_item_authoring",
            DefaultSlot = EditorDock.DockSlot.RightUr,
            IconName = "ResourcePreloader",
            Global = true
        };
        _dock.AddChild(_dockContent);
        AddDock(_dock);
        _tagDockContent = new GameplayTagEditorDock();
        _tagDock = new EditorDock
        {
            Name = "TLMGameplayTagEditorDock",
            Title = "TLM Tags",
            LayoutKey = "tlm_gameplay_tags",
            DefaultSlot = EditorDock.DockSlot.RightBl,
            IconName = "ClassList",
            Global = true
        };
        _tagDock.AddChild(_tagDockContent);
        AddDock(_tagDock);
        _balanceDockContent = new BalanceLabDock();
        _balanceDock = new EditorDock
        {
            Name = "TLMBalanceLabDock",
            Title = "TLM Balance",
            LayoutKey = "tlm_balance_lab",
            DefaultSlot = EditorDock.DockSlot.RightBr,
            IconName = "Curve"
        };
        _balanceDock.AddChild(_balanceDockContent);
        AddDock(_balanceDock);
        _localizationDockContent = new LocalizationDock();
        _localizationDock = new EditorDock
        {
            Name = "TLMLocalizationDock",
            Title = "TLM Localization",
            LayoutKey = "tlm_localization",
            DefaultSlot = EditorDock.DockSlot.RightBr,
            IconName = "Translation"
        };
        _localizationDock.AddChild(_localizationDockContent);
        AddDock(_localizationDock);
        _playGuardDialog = new AcceptDialog
        {
            Name = "TLMItemAuthoringPlayGuardDialog",
            Title = "TLM Editor Changes"
        };
        EditorInterface.Singleton.GetBaseControl()?.AddChild(_playGuardDialog);
    }

    public override void _ExitTree()
    {
        FlushEditorToolPendingChanges("editor shutdown");

        if (_playGuardDialog != null)
        {
            _playGuardDialog.QueueFree();
            _playGuardDialog = null;
        }

        if (_dock != null)
        {
            RemoveDock(_dock);
            _dock.QueueFree();
        }

        if (_tagDock != null)
        {
            RemoveDock(_tagDock);
            _tagDock.QueueFree();
        }

        if (_balanceDock != null)
        {
            RemoveDock(_balanceDock);
            _balanceDock.QueueFree();
        }

        if (_localizationDock != null)
        {
            RemoveDock(_localizationDock);
            _localizationDock.QueueFree();
        }

        _localizationDockContent = null;
        _localizationDock = null;
        _balanceDockContent = null;
        _balanceDock = null;
        _tagDockContent = null;
        _tagDock = null;
        _dockContent = null;
        _dock = null;
    }

    public override void _Notification(int what)
    {
        if (what == NotificationWMCloseRequest)
        {
            FlushEditorToolPendingChanges("window close");
        }
    }

    public override void _ApplyChanges()
    {
        FlushEditorToolPendingChanges("apply changes");
    }

    public override string _GetUnsavedStatus(string forScene)
    {
        if (!string.IsNullOrWhiteSpace(forScene) || !HasPendingEditorToolChanges())
        {
            return string.Empty;
        }

        return string.IsNullOrWhiteSpace(_lastExternalSaveFailure)
            ? "TLM editor tools have pending item or gameplay tag resource changes. Save them before closing the editor?"
            : $"TLM editor tools still have unsaved changes because the last save failed:\n{_lastExternalSaveFailure}\n\nCancel closing and fix the save problem.";
    }

    public override void _SaveExternalData()
    {
        FlushEditorToolPendingChanges("external data save");
    }

    public override bool _Build()
    {
        if (!HasPendingEditorToolChanges())
        {
            return true;
        }

        var messages = new List<string>();
        var itemMessage = string.Empty;
        if (_dockContent != null
            && _dockContent.HasPendingChanges()
            && !_dockContent.TryFlushPendingChangesBeforePlay(out itemMessage))
        {
            ShowPlayGuardPopup(
                "The game was not started because the item editor has unsaved changes that could not be saved.\n\n" +
                itemMessage);
            return false;
        }

        if (!string.IsNullOrWhiteSpace(itemMessage))
        {
            messages.Add(itemMessage);
        }

        var tagMessage = string.Empty;
        if (_tagDockContent != null
            && _tagDockContent.HasPendingChanges()
            && !_tagDockContent.TryFlushPendingChangesBeforePlay(out tagMessage))
        {
            ShowPlayGuardPopup(
                "The game was not started because the gameplay tag editor has unsaved changes that could not be saved.\n\n" +
                tagMessage);
            return false;
        }

        if (!string.IsNullOrWhiteSpace(tagMessage))
        {
            messages.Add(tagMessage);
        }

        ShowPlayGuardPopup(messages.Count == 0
            ? "Pending TLM editor changes were saved before starting the game."
            : string.Join("\n", messages));
        return true;
    }

    private static void CleanupStaleEditorUi()
    {
        var baseControl = EditorInterface.Singleton.GetBaseControl();
        if (baseControl == null)
        {
            return;
        }

        RemoveStaleNodesRecursive(baseControl);
    }

    private static void RemoveStaleNodesRecursive(Node node)
    {
        foreach (var child in node.GetChildren())
        {
            if (child.Name == "TLMItemAuthoringDock"
                || child.Name == "TLMItemAuthoringDockContent"
                || child.Name == "TLMGameplayTagEditorDock"
                || child.Name == "TLMGameplayTagEditorDockContent"
                || child.Name == "TLMBalanceLabDock"
                || child.Name == "TLMBalanceLabDockContent"
                || child.Name == "TLMLocalizationDock"
                || child.Name == "TLMLocalizationDockContent")
            {
                node.RemoveChild(child);
                child.QueueFree();
                continue;
            }

            RemoveStaleNodesRecursive(child);
        }
    }

    private void ShowPlayGuardPopup(string message)
    {
        if (_playGuardDialog == null)
        {
            GD.PushWarning($"TLM Editor Tools: {message}");
            return;
        }

        _playGuardDialog.DialogText = message;
        _playGuardDialog.PopupCentered(new Vector2I(560, 180));
    }

    private bool HasPendingEditorToolChanges()
    {
        return _dockContent?.HasPendingChanges() == true || _tagDockContent?.HasPendingChanges() == true;
    }

    private void FlushEditorToolPendingChanges(string reason)
    {
        var failures = new List<string>();
        if (!FlushItemEditorPendingChanges(reason, out var itemFailure))
        {
            failures.Add(itemFailure);
        }

        if (!FlushGameplayTagEditorPendingChanges(reason, out var tagFailure))
        {
            failures.Add(tagFailure);
        }

        _lastExternalSaveFailure = failures.Count == 0 ? string.Empty : string.Join("\n", failures);
    }

    private bool FlushItemEditorPendingChanges(string reason, out string failure)
    {
        failure = string.Empty;
        if (_dockContent == null || !_dockContent.HasPendingChanges())
        {
            return true;
        }

        if (_dockContent.TryFlushPendingChangesBeforeEditorClose(out var message))
        {
            if (!string.IsNullOrWhiteSpace(message))
            {
                GD.Print($"TLM Item Authoring: {message}");
            }

            return true;
        }

        failure = string.IsNullOrWhiteSpace(message)
            ? $"Could not save pending item editor changes during {reason}."
            : message;
        GD.PushWarning($"TLM Item Authoring: Could not save pending item editor changes during {reason}. {failure}");
        return false;
    }

    private bool FlushGameplayTagEditorPendingChanges(string reason, out string failure)
    {
        failure = string.Empty;
        if (_tagDockContent == null || !_tagDockContent.HasPendingChanges())
        {
            return true;
        }

        if (_tagDockContent.TryFlushPendingChangesBeforeEditorClose(out var message))
        {
            if (!string.IsNullOrWhiteSpace(message))
            {
                GD.Print($"TLM Tags: {message}");
            }

            return true;
        }

        failure = string.IsNullOrWhiteSpace(message)
            ? $"Could not save pending gameplay tag editor changes during {reason}."
            : message;
        GD.PushWarning($"TLM Tags: Could not save pending gameplay tag editor changes during {reason}. {failure}");
        return false;
    }
}
#endif
