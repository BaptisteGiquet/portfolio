using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class FeedbackProfile : ContentDefinition
{
    [Export] public PackedScene? VfxScene { get; set; }

    [Export] public AudioStream? Audio { get; set; }
}
