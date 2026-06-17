#include "UI/Common/Screens/GameMenu/EMRVoiceRosterEntryData.h"

#include "ProximityVoiceLocalPlayerSubsystem.h"

void UEMRVoiceRosterEntryData::Initialize(UProximityVoiceLocalPlayerSubsystem* InVoiceSubsystem, const FProximityVoiceRemotePlayerView& InPlayerView)
{
    VoiceSubsystem = InVoiceSubsystem;
    PlayerId = InPlayerView.PlayerId;
    const FText ResolvedDisplayName = InPlayerView.DisplayName.IsEmpty() ? FText::FromString(InPlayerView.PlayerId) : InPlayerView.DisplayName;
    DisplayName = ResolvedDisplayName;
    SetDataID(FName(*PlayerId));
    SetDataDisplayName(DisplayName);
    bIsTalking = InPlayerView.bIsTalking;
    Volume = FMath::Clamp(InPlayerView.Preference.Volume, 0.0f, 2.0f);
    bMuted = InPlayerView.Preference.bMuted;
    PingMs = InPlayerView.PingMs;
    bIsLocalPlayer = InPlayerView.bIsLocalPlayer;
    InitDataObject();
}

void UEMRVoiceRosterEntryData::UpdateFromPlayerView(const FProximityVoiceRemotePlayerView& InPlayerView)
{
    bool bHasChanges = false;

    const FText ResolvedDisplayName = InPlayerView.DisplayName.IsEmpty() ? FText::FromString(InPlayerView.PlayerId) : InPlayerView.DisplayName;
    if (!DisplayName.EqualTo(ResolvedDisplayName))
    {
        DisplayName = ResolvedDisplayName;
        SetDataDisplayName(ResolvedDisplayName);
        bHasChanges = true;
    }

    const bool bNewTalking = InPlayerView.bIsTalking;
    if (bIsTalking != bNewTalking)
    {
        bIsTalking = bNewTalking;
        bHasChanges = true;
    }

    const float NewVolume = FMath::Clamp(InPlayerView.Preference.Volume, 0.0f, 2.0f);
    if (!FMath::IsNearlyEqual(Volume, NewVolume))
    {
        Volume = NewVolume;
        bHasChanges = true;
    }

    const bool bNewMuted = InPlayerView.Preference.bMuted;
    if (bMuted != bNewMuted)
    {
        bMuted = bNewMuted;
        bHasChanges = true;
    }

    if (PingMs != InPlayerView.PingMs)
    {
        PingMs = InPlayerView.PingMs;
        bHasChanges = true;
    }

    if (bIsLocalPlayer != InPlayerView.bIsLocalPlayer)
    {
        bIsLocalPlayer = InPlayerView.bIsLocalPlayer;
        bHasChanges = true;
    }

    if (bHasChanges)
    {
        NotifyListDataModified(this);
    }
}
