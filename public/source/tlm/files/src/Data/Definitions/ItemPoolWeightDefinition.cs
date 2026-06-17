using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class ItemPoolWeightDefinition : Resource
{
    [Export] public string PoolId { get; set; } = ItemPoolIds.NightMarket;

    [Export(PropertyHint.Range, "0,100000,0.01")] public float Weight { get; set; } = 1f;
}
