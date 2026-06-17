using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Gameplay.Commands;
using TheLastMage.Gameplay.Run;
using TheLastMage.Localization;

namespace TheLastMage.UI;

public partial class MarketPanel : Control
{
    private readonly Label[] _offerLabels = new Label[3];
    private readonly Label[] _offerTagLabels = new Label[3];
    private readonly Button[] _chooseButtons = new Button[3];

    private RunControllerNode? _runController;
    private Label? _headerLabel;
    private Label? _buildLabel;
    private Button? _rerollButton;
    private bool _wasVisible;
    private bool _capturedInput;

    public override void _Ready()
    {
        MouseFilter = MouseFilterEnum.Stop;
        AnchorLeft = 0.34f;
        AnchorTop = 0.14f;
        AnchorRight = 0.72f;
        AnchorBottom = 0.62f;
        OffsetLeft = 0f;
        OffsetTop = 0f;
        OffsetRight = 0f;
        OffsetBottom = 0f;

        var panel = UiStyle.CreatePanel("MarketChoicePanel");
        panel.SetAnchorsPreset(LayoutPreset.FullRect);
        AddChild(panel);

        var margin = UiStyle.AddMargin(panel, 12);
        var root = new VBoxContainer();
        root.AddThemeConstantOverride("separation", 8);
        margin.AddChild(root);

        _headerLabel = UiStyle.CreateLabel(fontSize: 18, color: UiStyle.PanelAccent);
        root.AddChild(_headerLabel);

        for (var i = 0; i < _offerLabels.Length; i++)
        {
            var rowPanel = UiStyle.CreatePanel($"MarketOffer{i + 1}");
            rowPanel.CustomMinimumSize = new Vector2(0f, 82f);
            root.AddChild(rowPanel);

            var rowMargin = UiStyle.AddMargin(rowPanel, 8);
            var row = new HBoxContainer();
            row.AddThemeConstantOverride("separation", 8);
            rowMargin.AddChild(row);

            var copy = new VBoxContainer
            {
                SizeFlagsHorizontal = SizeFlags.ExpandFill
            };
            copy.AddThemeConstantOverride("separation", 3);
            row.AddChild(copy);

            _offerLabels[i] = UiStyle.CreateLabel(fontSize: 13);
            _offerLabels[i].SizeFlagsHorizontal = SizeFlags.ExpandFill;
            copy.AddChild(_offerLabels[i]);

            _offerTagLabels[i] = UiStyle.CreateLabel(fontSize: 11, color: UiStyle.TextMuted);
            copy.AddChild(_offerTagLabels[i]);

            var offerIndex = i;
            _chooseButtons[i] = UiStyle.CreateButton(LocalizationService.Current.Text("ui.market.choose"));
            _chooseButtons[i].CustomMinimumSize = new Vector2(88f, 30f);
            _chooseButtons[i].Pressed += () => ChooseOffer(offerIndex);
            row.AddChild(_chooseButtons[i]);
        }

        _rerollButton = UiStyle.CreateButton(LocalizationService.Current.Text("ui.market.reroll"));
        _rerollButton.Pressed += Reroll;
        root.AddChild(_rerollButton);

        _buildLabel = UiStyle.CreateLabel(fontSize: 12, color: UiStyle.TextMuted);
        root.AddChild(_buildLabel);
    }

    public override void _Process(double delta)
    {
        _runController ??= GetTree().Root.FindChild("RunControllerNode", true, false) as RunControllerNode;
        var context = _runController?.Context;
        var shouldBeVisible = context?.State.CurrentPhase == RunPhase.NightMarket
            && !context.State.Market.ChoiceCompletedThisNight;
        Visible = shouldBeVisible;
        if (context == null || !Visible)
        {
            ReleaseInputCapture(context);
            _wasVisible = false;
            return;
        }

        context.State.IsDebugUiInputCaptured = true;
        _capturedInput = true;
        UpdateView(context);
        if (!_wasVisible)
        {
            GrabInitialFocus();
        }

        _wasVisible = true;
    }

    private void ReleaseInputCapture(RunContext? context)
    {
        if (!_capturedInput)
        {
            return;
        }

        if (context != null)
        {
            context.State.IsDebugUiInputCaptured = false;
        }

        _capturedInput = false;
    }

    private void UpdateView(RunContext context)
    {
        if (_headerLabel != null)
        {
            _headerLabel.Text = LocalizationService.Current.Text("ui.market.header", context.State.DayIndex);
        }

        var offers = RunViewModels.BuildMarketOffers(context);
        for (var i = 0; i < _offerLabels.Length; i++)
        {
            var offer = offers[i];
            _chooseButtons[i].Disabled = !offer.CanChoose;
            _offerLabels[i].Text = LocalizationService.Current.Text("ui.market.offer", offer.DisplayName, offer.BodyText);
            _offerTagLabels[i].Text = LocalizationService.Current.Text("ui.market.tags", offer.TagText);
        }

        if (_buildLabel != null)
        {
            _buildLabel.Text = BuildModifierText(context);
        }
    }

    private void GrabInitialFocus()
    {
        foreach (var button in _chooseButtons)
        {
            if (!button.Disabled && button.Visible)
            {
                button.GrabFocus();
                return;
            }
        }

        if (_rerollButton is { Disabled: false, Visible: true })
        {
            _rerollButton.GrabFocus();
        }
    }

    private static string BuildModifierText(RunContext context)
    {
        if (context.State.Build.Items.Count == 0 && context.State.Build.ActivatableItem == null)
        {
            return string.Join('\n', new[]
            {
                LocalizationService.Current.Text("ui.market.modifier_sources"),
                LocalizationService.Current.Text("ui.market.no_acquired_items")
            });
        }

        var lines = new List<string> { LocalizationService.Current.Text("ui.market.modifier_sources") };
        foreach (var stack in context.State.Build.Items)
        {
            var item = context.Content.GetItem(stack.ItemId);
            if (!string.Equals(item.Kind, nameof(ItemKind.Passive), StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            foreach (var modifier in item.Modifiers)
            {
                var tagText = modifier.TargetTag.IsValid
                    ? LocalizationService.Current.Text("ui.market.modifier_tag", modifier.TargetTag)
                    : string.Empty;
                lines.Add(LocalizationService.Current.Text(
                    "ui.market.modifier_line",
                    LocalizationService.Current.ContentName("item", item.Id, item.DisplayName),
                    stack.Count,
                    modifier.Operation,
                    modifier.Value,
                    modifier.StatId,
                    tagText));
            }
        }

        if (context.State.Build.ActivatableItem != null)
        {
            var active = context.State.Build.ActivatableItem;
            var item = context.Content.GetItem(active.ItemId);
            var usesText = active.HasUnlimitedActivations
                ? LocalizationService.Current.Text("ui.common.unlimited")
                : LocalizationService.Current.Text("ui.market.activatable_limited", active.RemainingActivations, active.MaxActivations);
            lines.Add(LocalizationService.Current.Text(
                "ui.market.activatable_line",
                LocalizationService.Current.ContentName("item", item.Id, item.DisplayName),
                usesText));
        }

        return string.Join('\n', lines);
    }

    private void ChooseOffer(int offerIndex)
    {
        var context = _runController?.Context;
        if (context == null)
        {
            return;
        }

        context.Commands.TryExecute(new ChooseMarketOfferCommand(offerIndex), context, out _);
    }

    private void Reroll()
    {
        var context = _runController?.Context;
        if (context == null)
        {
            return;
        }

        context.Commands.TryExecute(new RerollOffersCommand(), context, out _);
    }
}
