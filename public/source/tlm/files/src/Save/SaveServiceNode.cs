using Godot;
using System.Text.Json;
using TheLastMage.Controls;
using TheLastMage.Foundation;
using TheLastMage.Localization;

namespace TheLastMage.Save;

public partial class SaveServiceNode : Node
{
    private const string ProfileSlotDirectory = "user://profiles";
    private const string SettingsPath = "user://settings.json";
    public const int ProfileSlotCount = 5;

    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true
    };

    public ProfileSaveDto Profile { get; private set; } = new();

    public SettingsSaveDto Settings { get; private set; } = new();

    public int ActiveProfileSlot { get; private set; }

    public override void _Ready()
    {
        InputBindingService.EnsureDefaultActions();
        ProfileDefaultsService.EnsureCurrent(Profile);
        Settings = LoadSettings();
        InputBindingService.ApplySavedBindings(Settings);
        LocalizationService.Current.SetLocale(Settings.Locale);
    }

    public ProfileSaveDto LoadProfile()
    {
        return ActiveProfileSlot > 0 ? LoadProfileSlot(ActiveProfileSlot) : Profile;
    }

    public ProfileSaveDto LoadProfileSlot(int slotIndex)
    {
        slotIndex = NormalizeSlotIndex(slotIndex);
        var path = RuntimePathResolver.GlobalizeUserPath(GetProfileSlotPath(slotIndex));
        GD.Print($"SaveServiceLoadProfileSlot slot={slotIndex} path={path} exists={System.IO.File.Exists(path)}");
        if (!System.IO.File.Exists(path))
        {
            var newProfile = new ProfileSaveDto();
            ActiveProfileSlot = slotIndex;
            SaveProfile(newProfile);
            return newProfile;
        }

        var json = System.IO.File.ReadAllText(path);
        var profile = JsonSerializer.Deserialize<ProfileSaveDto>(json, JsonOptions) ?? new ProfileSaveDto();
        ActiveProfileSlot = slotIndex;
        if (ProfileDefaultsService.EnsureCurrent(profile))
        {
            SaveProfile(profile);
        }
        else
        {
            Profile = profile;
        }

        return profile;
    }

    public void SaveProfile(ProfileSaveDto profile)
    {
        if (ActiveProfileSlot <= 0)
        {
            ProfileDefaultsService.EnsureCurrent(profile);
            Profile = profile;
            GD.Print("SaveServiceSaveProfile active_slot=none persisted=false");
            return;
        }

        SaveProfileSlot(ActiveProfileSlot, profile);
    }

    public void SaveProfileSlot(int slotIndex, ProfileSaveDto profile)
    {
        ProfileDefaultsService.EnsureCurrent(profile);
        slotIndex = NormalizeSlotIndex(slotIndex);
        var path = RuntimePathResolver.GlobalizeUserPath(GetProfileSlotPath(slotIndex));
        var directory = System.IO.Path.GetDirectoryName(path);
        if (!string.IsNullOrEmpty(directory))
        {
            System.IO.Directory.CreateDirectory(directory);
        }

        var json = JsonSerializer.Serialize(profile, JsonOptions);
        System.IO.File.WriteAllText(path, json);
        GD.Print($"SaveServiceSaveProfileSlot slot={slotIndex} path={path}");
        if (slotIndex == ActiveProfileSlot)
        {
            Profile = profile;
        }
    }

    public IReadOnlyList<ProfileSlotSummaryDto> LoadSlotSummaries()
    {
        var summaries = new List<ProfileSlotSummaryDto>(ProfileSlotCount);
        for (var slotIndex = 1; slotIndex <= ProfileSlotCount; slotIndex++)
        {
            summaries.Add(LoadSlotSummary(slotIndex));
        }

        return summaries;
    }

    public ProfileSlotSummaryDto LoadSlotSummary(int slotIndex)
    {
        slotIndex = NormalizeSlotIndex(slotIndex);
        var path = RuntimePathResolver.GlobalizeUserPath(GetProfileSlotPath(slotIndex));
        if (!System.IO.File.Exists(path))
        {
            return new ProfileSlotSummaryDto { SlotIndex = slotIndex, Exists = false };
        }

        var json = System.IO.File.ReadAllText(path);
        var profile = JsonSerializer.Deserialize<ProfileSaveDto>(json, JsonOptions) ?? new ProfileSaveDto();
        ProfileDefaultsService.EnsureCurrent(profile);
        return ProfileSlotSummaryDto.FromProfile(slotIndex, true, profile);
    }

    public void DeleteProfileSlot(int slotIndex)
    {
        slotIndex = NormalizeSlotIndex(slotIndex);
        var path = RuntimePathResolver.GlobalizeUserPath(GetProfileSlotPath(slotIndex));
        var existed = System.IO.File.Exists(path);
        if (System.IO.File.Exists(path))
        {
            System.IO.File.Delete(path);
        }

        GD.Print($"SaveServiceDeleteProfileSlot slot={slotIndex} path={path} existed={existed} existsAfter={System.IO.File.Exists(path)}");
        if (slotIndex == ActiveProfileSlot)
        {
            ActiveProfileSlot = 0;
            Profile = new ProfileSaveDto();
            ProfileDefaultsService.EnsureCurrent(Profile);
        }
    }

    public SettingsSaveDto LoadSettings()
    {
        var path = RuntimePathResolver.GlobalizeUserPath(SettingsPath);
        if (!System.IO.File.Exists(path))
        {
            var newSettings = new SettingsSaveDto();
            SaveSettings(newSettings);
            return newSettings;
        }

        var json = System.IO.File.ReadAllText(path);
        var settings = JsonSerializer.Deserialize<SettingsSaveDto>(json, JsonOptions) ?? new SettingsSaveDto();
        SaveSettings(settings);
        return settings;
    }

    public void SaveSettings(SettingsSaveDto settings)
    {
        var path = RuntimePathResolver.GlobalizeUserPath(SettingsPath);
        var directory = System.IO.Path.GetDirectoryName(path);
        if (!string.IsNullOrEmpty(directory))
        {
            System.IO.Directory.CreateDirectory(directory);
        }

        var json = JsonSerializer.Serialize(settings, JsonOptions);
        System.IO.File.WriteAllText(path, json);
        Settings = settings;
        InputBindingService.ApplySavedBindings(Settings);
        LocalizationService.Current.SetLocale(Settings.Locale);
        SettingsApplicationService.Apply(Settings, GetViewport());
    }

    private static string GetProfileSlotPath(int slotIndex)
    {
        return $"{ProfileSlotDirectory}/slot_{slotIndex:00}.json";
    }

    private static int NormalizeSlotIndex(int slotIndex)
    {
        return Math.Clamp(slotIndex, 1, ProfileSlotCount);
    }
}

public sealed class ProfileSlotSummaryDto
{
    public int SlotIndex { get; init; }

    public bool Exists { get; init; }

    public string PreferredMageId { get; init; } = ProfileDefaultsService.DefaultMageId;

    public int RunsPlayed { get; init; }

    public int Wins { get; init; }

    public int Deaths { get; init; }

    public int HighestDayReached { get; init; }

    public int HumanityKilled { get; init; }

    public int EnemiesKilled { get; init; }

    public int ItemsDiscovered { get; init; }

    public int AchievementsCompleted { get; init; }

    public static ProfileSlotSummaryDto FromProfile(int slotIndex, bool exists, ProfileSaveDto profile)
    {
        return new ProfileSlotSummaryDto
        {
            SlotIndex = slotIndex,
            Exists = exists,
            PreferredMageId = profile.PreferredMageId,
            RunsPlayed = profile.RunStatistics.TotalNormalRuns,
            Wins = profile.RunStatistics.Victories,
            Deaths = profile.RunStatistics.Defeats,
            HighestDayReached = profile.RunStatistics.HighestDayReached,
            HumanityKilled = profile.RunStatistics.TotalHumanityKilled,
            EnemiesKilled = profile.RunStatistics.TotalEnemiesKilled,
            ItemsDiscovered = profile.DiscoveredItemIds.Count,
            AchievementsCompleted = profile.CompletedAchievementIds.Count
        };
    }
}
