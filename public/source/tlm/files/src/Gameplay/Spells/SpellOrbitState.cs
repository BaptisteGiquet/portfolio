using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Spells;

public struct SpellOrbitState
{
    public bool Enabled;
    public float AgeSeconds;
    public float AngleRadians;
    public float AngularSpeedRadians;
    public float Radius;
    public float Height;
    public float Phase;
    public float RadialAmplitude;
    public float RadialFrequency;
    public float SecondaryRadialAmplitude;
    public float SecondaryRadialFrequency;
    public float VerticalAmplitude;
    public float VerticalFrequency;
    public float EllipseStrength;
    public float WobbleStrength;

    public Vector3 PositionAt(Vector3 playerPosition)
    {
        if (!Enabled)
        {
            return playerPosition;
        }

        var wobble = MathF.Sin(AgeSeconds * RadialFrequency * 0.73f + Phase * 0.61f) * WobbleStrength;
        var angle = AngleRadians + wobble;
        var radial =
            Radius
            + MathF.Sin(AgeSeconds * RadialFrequency + Phase) * RadialAmplitude
            + MathF.Sin(AgeSeconds * SecondaryRadialFrequency + Phase * 1.71f) * SecondaryRadialAmplitude;
        radial = MathF.Max(0.35f, radial);
        var ellipse = 1f + MathF.Sin(AgeSeconds * 0.91f + Phase * 0.37f) * EllipseStrength;
        var y = Height + MathF.Sin(AgeSeconds * VerticalFrequency + Phase * 1.19f) * VerticalAmplitude;

        return playerPosition + new Vector3(
            MathF.Sin(angle) * radial,
            y,
            -MathF.Cos(angle) * radial * MathF.Max(0.25f, ellipse));
    }

    public Vector3 Advance(Vector3 playerPosition, float delta, out Vector3 velocity)
    {
        var previous = PositionAt(playerPosition);
        AgeSeconds += delta;
        AngleRadians += AngularSpeedRadians * delta;
        var next = PositionAt(playerPosition);
        velocity = delta > 0f ? (next - previous) / delta : Vector3.Zero;
        return next;
    }
}

public static class SpellOrbit
{
    public static SpellOrbitState Create(
        RunContext context,
        IReadOnlyCollection<TagId> sourceTags,
        ContentId sourceId,
        EntityId casterId,
        Vector3 origin,
        Vector3 direction,
        int salt,
        float referenceSpeed = 8f)
    {
        var strength = PlayerAttributeResolver.ResolveSpellOrbitStrength(context, sourceTags);
        if (strength <= 0f)
        {
            return default;
        }

        var seed = Hash(sourceId.Value, casterId.Value, salt);
        var normalizedDirection = direction.LengthSquared() <= 0.0001f ? Vector3.Forward : direction.Normalized();
        var angle = MathF.Atan2(normalizedDirection.X, -normalizedDirection.Z);
        var radius = PlayerAttributeResolver.ResolveSpellOrbitRadius(
            context,
            sourceTags,
            3.2f + MathF.Min(2.2f, strength * 0.7f));
        var defaultAngularSpeed = 0.95f + MathF.Min(1.6f, referenceSpeed * 0.045f) + strength * 0.18f;
        var angularSpeed = PlayerAttributeResolver.ResolveSpellOrbitAngularSpeed(context, sourceTags, defaultAngularSpeed);
        var sign = (seed & 1) == 0 ? 1f : -1f;
        var playerPosition = context.State.Player.Position;
        var height = Math.Clamp(origin.Y - playerPosition.Y, 0.45f, 1.35f);
        var phase = Unit(seed >> 8) * MathF.Tau;

        return new SpellOrbitState
        {
            Enabled = true,
            AngleRadians = angle + Unit(seed >> 16) * MathF.Tau * 0.35f,
            AngularSpeedRadians = angularSpeed * sign,
            Radius = radius,
            Height = height,
            Phase = phase,
            RadialAmplitude = radius * Math.Clamp(0.22f + strength * 0.07f, 0.2f, 0.55f),
            RadialFrequency = 1.15f + Unit(seed >> 24) * 1.35f + strength * 0.08f,
            SecondaryRadialAmplitude = radius * Math.Clamp(0.08f + strength * 0.04f, 0.08f, 0.28f),
            SecondaryRadialFrequency = 2.1f + Unit(seed >> 12) * 2.3f,
            VerticalAmplitude = Math.Clamp(0.12f + strength * 0.08f, 0.12f, 0.45f),
            VerticalFrequency = 1.6f + Unit(seed >> 20) * 1.7f,
            EllipseStrength = Math.Clamp(0.18f + strength * 0.05f, 0.18f, 0.42f),
            WobbleStrength = Math.Clamp(0.18f + strength * 0.07f, 0.18f, 0.5f)
        };
    }

    private static int Hash(string sourceId, int casterId, int salt)
    {
        unchecked
        {
            var hash = 17;
            for (var i = 0; i < sourceId.Length; i++)
            {
                hash = hash * 31 + sourceId[i];
            }

            hash = hash * 31 + casterId;
            hash = hash * 31 + salt;
            hash ^= hash << 13;
            hash ^= hash >> 17;
            hash ^= hash << 5;
            return hash;
        }
    }

    private static float Unit(int value)
    {
        unchecked
        {
            return ((uint)value & 0xFFFF) / 65535f;
        }
    }
}
