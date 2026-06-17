#include "UI/Common/Screens/GameMenu/EMRVoiceRosterRowBinding.h"

#include "Components/TextBlock.h"
#include "ProximityVoiceLocalPlayerSubsystem.h"

void UEMRVoiceRosterRowBinding::Initialize(UProximityVoiceLocalPlayerSubsystem* InVoiceSubsystem, const FString& InRemotePlayerId, bool bInMuted, UTextBlock* InMuteLabel)
{
    VoiceSubsystem = InVoiceSubsystem;
    RemotePlayerId = InRemotePlayerId;
    bMuted = bInMuted;
    MuteLabel = InMuteLabel;
    RefreshMuteLabel();
}

void UEMRVoiceRosterRowBinding::HandleVolumeChanged(float InValue)
{
    if (!VoiceSubsystem || RemotePlayerId.IsEmpty())
    {
        return;
    }

    VoiceSubsystem->SetRemotePlayerVolume(RemotePlayerId, InValue);
}

void UEMRVoiceRosterRowBinding::HandleMuteClicked()
{
    if (!VoiceSubsystem || RemotePlayerId.IsEmpty())
    {
        return;
    }

    bMuted = !bMuted;
    VoiceSubsystem->SetRemotePlayerMuted(RemotePlayerId, bMuted);
    RefreshMuteLabel();
}

void UEMRVoiceRosterRowBinding::RefreshMuteLabel() const
{
    if (!MuteLabel)
    {
        return;
    }

    MuteLabel->SetText(bMuted ? FText::FromString(TEXT("Unmute")) : FText::FromString(TEXT("Mute")));
}

