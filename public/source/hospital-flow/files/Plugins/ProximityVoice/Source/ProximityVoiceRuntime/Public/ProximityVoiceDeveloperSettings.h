#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ProximityVoiceDeveloperSettings.generated.h"

class USoundAttenuation;
class USoundEffectSourcePresetChain;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Proximity Voice Settings"))
class PROXIMITYVOICERUNTIME_API UProximityVoiceDeveloperSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Config, Category = "EMR|Voice|Spatialization")
    TSoftObjectPtr<USoundAttenuation> VoiceAttenuation;

    UPROPERTY(EditDefaultsOnly, Config, Category = "EMR|Voice|Spatialization")
    TSoftObjectPtr<USoundEffectSourcePresetChain> VoiceSourceEffects;

    UPROPERTY(EditDefaultsOnly, Config, Category = "EMR|Voice|Spatialization", meta = (ClampMin = "100.0", UIMin = "100.0"))
    float MaxVoiceDistance = 2500.0f;

    UPROPERTY(EditDefaultsOnly, Config, Category = "EMR|Voice|Spatialization", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float FullVolumeDistance = 250.0f;

    UPROPERTY(EditDefaultsOnly, Config, Category = "EMR|Voice|Mic", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float MicActivityThreshold = 0.05f;

    UPROPERTY(EditDefaultsOnly, Config, Category = "EMR|Voice|Runtime", meta = (ClampMin = "0.05", UIMin = "0.05", UIMax = "1.0"))
    float PollIntervalSeconds = 0.2f;
};
