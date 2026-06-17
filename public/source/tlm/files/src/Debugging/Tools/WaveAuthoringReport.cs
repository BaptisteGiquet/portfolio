using System.Text;
using TheLastMage.Data.Runtime;

namespace TheLastMage.Debugging.Tools;

public static class WaveAuthoringReport
{
    public static string Build(ContentRegistry content)
    {
        var builder = new StringBuilder();
        builder.AppendLine("Wave Authoring Report");
        foreach (var wave in content.Waves.Values.OrderBy(wave => wave.DayIndex))
        {
            builder.AppendLine(
                $"{wave.Id}: day={wave.DayIndex} duration={wave.DurationSeconds:0.#}s target={wave.TargetActiveEnemies} intensity={wave.Intensity:0.##}");
            builder.AppendLine($"  lanes={string.Join(", ", wave.SpawnLanes.Select(lane => $"{lane.Id}:{Format(lane.SpawnPosition)}->{Format(lane.ApproachPosition)} w={lane.Weight:0.##}"))}");
            builder.AppendLine($"  groups={string.Join(", ", wave.SpawnGroups.Select(group => $"{group.EnemyId} x{group.Count} start={group.StartTimeSeconds:0.#}s interval={group.SpawnIntervalSeconds:0.##}s mult={group.IntensityMultiplier:0.##}"))}");
        }

        return builder.ToString();
    }

    private static string Format(Godot.Vector3 value)
    {
        return $"({value.X:0.#},{value.Y:0.#},{value.Z:0.#})";
    }
}
