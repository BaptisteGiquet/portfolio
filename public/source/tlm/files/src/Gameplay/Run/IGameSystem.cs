namespace TheLastMage.Gameplay.Run;

public interface IGameSystem
{
    void Initialize(RunContext context);

    void Tick(float delta);

    void FixedTick(float delta);

    void Shutdown();
}
