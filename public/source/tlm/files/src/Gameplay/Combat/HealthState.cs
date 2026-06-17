namespace TheLastMage.Gameplay.Combat;

public sealed class HealthState
{
    public HealthState(float maxHealth)
    {
        MaxHealth = MathF.Max(1f, maxHealth);
        CurrentHealth = MaxHealth;
    }

    public float MaxHealth { get; private set; }

    public float CurrentHealth { get; private set; }

    public bool IsDead => CurrentHealth <= 0f;

    public float ApplyDamage(float amount)
    {
        if (IsDead)
        {
            return 0f;
        }

        var applied = MathF.Max(0f, amount);
        CurrentHealth = MathF.Max(0f, CurrentHealth - applied);
        return applied;
    }

    public float Repair(float amount)
    {
        if (IsDead)
        {
            return 0f;
        }

        var applied = MathF.Max(0f, amount);
        var previous = CurrentHealth;
        CurrentHealth = MathF.Min(MaxHealth, CurrentHealth + applied);
        return CurrentHealth - previous;
    }

    public void SetMaxHealth(float maxHealth, bool healByIncrease)
    {
        var previousMax = MaxHealth;
        MaxHealth = MathF.Max(1f, maxHealth);
        if (healByIncrease && MaxHealth > previousMax)
        {
            CurrentHealth += MaxHealth - previousMax;
        }

        CurrentHealth = Math.Clamp(CurrentHealth, 0f, MaxHealth);
    }

    public void SetHealth(float currentHealth, float maxHealth)
    {
        MaxHealth = MathF.Max(1f, maxHealth);
        CurrentHealth = Math.Clamp(currentHealth, 0f, MaxHealth);
    }
}
