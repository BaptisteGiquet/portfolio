using TheLastMage.Data.Runtime;
using TheLastMage.Foundation.Events;
using TheLastMage.Foundation.Pooling;
using TheLastMage.Foundation.Random;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Commands;
using TheLastMage.Save;

namespace TheLastMage.Gameplay.Run;

public sealed class RunContext
{
    public required ContentRegistry Content { get; init; }

    public required RunState State { get; init; }

    public required GameEventBus Events { get; init; }

    public required CommandDispatcher Commands { get; init; }

    public required PoolHub Pools { get; init; }

    public required RandomService Random { get; init; }

    public required ProfileSaveDto Profile { get; init; }

    public required Action<ProfileSaveDto> SaveProfile { get; init; }

    public required SettingsSaveDto Settings { get; init; }

    public required Action<SettingsSaveDto> SaveSettings { get; init; }

    public required ItemKnowledgeService ItemKnowledge { get; init; }

    public required IReadOnlyList<IGameSystem> Systems { get; init; }

    public TSystem GetSystem<TSystem>()
        where TSystem : class, IGameSystem
    {
        foreach (var system in Systems)
        {
            if (system is TSystem typed)
            {
                return typed;
            }
        }

        throw new InvalidOperationException($"RunContext does not contain system {typeof(TSystem).Name}.");
    }

    public bool TryGetSystem<TSystem>(out TSystem? typedSystem)
        where TSystem : class, IGameSystem
    {
        foreach (var system in Systems)
        {
            if (system is TSystem typed)
            {
                typedSystem = typed;
                return true;
            }
        }

        typedSystem = null;
        return false;
    }
}
