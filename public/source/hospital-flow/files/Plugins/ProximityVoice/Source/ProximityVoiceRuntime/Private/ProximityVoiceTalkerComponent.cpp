#include "ProximityVoiceTalkerComponent.h"

#include "Components/AudioComponent.h"

namespace
{
    constexpr float MaxPerPlayerVolume = 2.0f;
    constexpr float MaxGlobalVoiceVolume = 1.0f;
    constexpr float MaxEffectiveVoiceVolume = 1.5f;
    constexpr float HighVolumeTaper = 0.5f;
}

void UProximityVoiceTalkerComponent::SetRemotePreference(const FProximityVoiceRemotePreference& InPreference)
{
    CachedPreference.Volume = FMath::Clamp(InPreference.Volume, 0.0f, MaxPerPlayerVolume);
    CachedPreference.bMuted = InPreference.bMuted;
    RefreshActiveAudioVolume();
}

void UProximityVoiceTalkerComponent::SetDistanceAttenuation(float InDistanceAttenuation)
{
    CachedDistanceAttenuation = FMath::Clamp(InDistanceAttenuation, 0.0f, 1.0f);
    RefreshActiveAudioVolume();
}

void UProximityVoiceTalkerComponent::SetGlobalVoiceVolume(float InGlobalVoiceVolume)
{
    CachedGlobalVoiceVolume = FMath::Clamp(InGlobalVoiceVolume, 0.0f, MaxGlobalVoiceVolume);
    RefreshActiveAudioVolume();
}

void UProximityVoiceTalkerComponent::OnTalkingBegin(UAudioComponent* AudioComponent)
{
    ActiveAudioComponent = AudioComponent;
    RefreshActiveAudioVolume();

    Super::OnTalkingBegin(AudioComponent);
}

void UProximityVoiceTalkerComponent::OnTalkingEnd()
{
    ActiveAudioComponent.Reset();
    Super::OnTalkingEnd();
}

float UProximityVoiceTalkerComponent::GetLastEnvelope()
{
    return GetVoiceLevel();
}

void UProximityVoiceTalkerComponent::RefreshActiveAudioVolume() const
{
    UAudioComponent* AudioComponent = ActiveAudioComponent.Get();
    if (!AudioComponent)
    {
        return;
    }

    // Keep >100% per-player boost available, but taper it to reduce clipping risk.
    const float TaperedPlayerVolume = CachedPreference.Volume <= 1.0f
        ? CachedPreference.Volume
        : 1.0f + ((CachedPreference.Volume - 1.0f) * HighVolumeTaper);

    const float EffectiveVolume = CachedPreference.bMuted
        ? 0.0f
        : FMath::Clamp(TaperedPlayerVolume * CachedDistanceAttenuation * CachedGlobalVoiceVolume, 0.0f, MaxEffectiveVoiceVolume);

    AudioComponent->SetVolumeMultiplier(EffectiveVolume);
}
