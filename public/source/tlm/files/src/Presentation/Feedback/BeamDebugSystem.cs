using Godot;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Presentation.Feedback;

public sealed class BeamDebugSystem : IGameSystem
{
    private Node3D? _root;
    private RunContext? _context;
    private SubscriptionToken _beamToken;

    public BeamDebugSystem(Node3D? root)
    {
        _root = root;
    }

    public void Initialize(RunContext context)
    {
        _context = context;
        _beamToken = context.Events.Subscribe<BeamFiredEvent>(OnBeamFired);
    }

    public void Tick(float delta)
    {
    }

    public void FixedTick(float delta)
    {
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            _context.Events.Unsubscribe(_beamToken);
        }

        _context = null;
        _root = null;
    }

    private void OnBeamFired(BeamFiredEvent gameEvent)
    {
        if (_root == null)
        {
            return;
        }

        var node = new BeamDebugNode { Name = $"Beam_{gameEvent.SpellId}" };
        _root.AddChild(node);
        node.Bind(gameEvent.Start, gameEvent.End, gameEvent.Radius);
    }
}
