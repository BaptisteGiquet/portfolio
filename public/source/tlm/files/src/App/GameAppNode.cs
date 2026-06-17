using Godot;
using TheLastMage.Controls;
using TheLastMage.Data.Definitions;
using TheLastMage.Localization;

namespace TheLastMage.App;

public partial class GameAppNode : Node
{
    private const string CatalogPath = "res://data/catalogs/content_catalog.tres";

    public override void _Ready()
    {
        InputBindingService.EnsureDefaultActions();
        var catalog = ResourceLoader.Load<ContentCatalog>(CatalogPath) ?? new ContentCatalog();
        LocalizationService.Current.LoadEnglish(catalog, LocalizationService.Current.CurrentLocale);
        GD.Print("The Last Mage app spine loaded.");
    }

    public override void _Input(InputEvent @event)
    {
        InputPromptService.ObserveInputEvent(@event);
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (@event is InputEventKey { Pressed: true, Echo: false, Keycode: Key.Escape })
        {
            GetTree().Quit();
        }
    }
}
