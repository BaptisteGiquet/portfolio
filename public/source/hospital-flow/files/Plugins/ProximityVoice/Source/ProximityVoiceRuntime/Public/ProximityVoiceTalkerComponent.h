#pragma once

#include "CoreMinimal.h"
#include "Net/VoiceConfig.h"
#include "ProximityVoiceTypes.h"
#include "ProximityVoiceTalkerComponent.generated.h"

class UAudioComponent;

UCLASS(ClassGroup = VOIP, meta = (BlueprintSpawnableComponent))
class PROXIMITYVOICERUNTIME_API UProximityVoiceTalkerComponent : public UVOIPTalker
{
    GENERATED_BODY()

public:
    void SetRemotePreference(const FProximityVoiceRemotePreference& InPreference);
    void SetDistanceAttenuation(float InDistanceAttenuation);
    void SetGlobalVoiceVolume(float InGlobalVoiceVolume);

    virtual void OnTalkingBegin(UAudioComponent* AudioComponent) override;
    virtual void OnTalkingEnd() override;

    float GetLastEnvelope();

private:
    void RefreshActiveAudioVolume() const;

    TWeakObjectPtr<UAudioComponent> ActiveAudioComponent;
    FProximityVoiceRemotePreference CachedPreference;
    float CachedDistanceAttenuation = 1.0f;
    float CachedGlobalVoiceVolume = 1.0f;
};
