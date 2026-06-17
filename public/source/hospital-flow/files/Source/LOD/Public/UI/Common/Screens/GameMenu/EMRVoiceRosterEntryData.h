#pragma once

#include "CoreMinimal.h"
#include "ProximityVoiceTypes.h"
#include "UI/Frontend/Data/EMRListDataObjectBase.h"
#include "EMRVoiceRosterEntryData.generated.h"

class UProximityVoiceLocalPlayerSubsystem;

UCLASS()
class LOD_API UEMRVoiceRosterEntryData : public UEMRListDataObjectBase
{
    GENERATED_BODY()

public:
    void Initialize(UProximityVoiceLocalPlayerSubsystem* InVoiceSubsystem, const FProximityVoiceRemotePlayerView& InPlayerView);
    void UpdateFromPlayerView(const FProximityVoiceRemotePlayerView& InPlayerView);

    UProximityVoiceLocalPlayerSubsystem* GetVoiceSubsystem() const { return VoiceSubsystem; }
    const FString& GetPlayerId() const { return PlayerId; }
    const FText& GetDisplayName() const { return DisplayName; }
    bool IsTalking() const { return bIsTalking; }
    float GetVolume() const { return Volume; }
    bool IsMuted() const { return bMuted; }
    int32 GetPingMs() const { return PingMs; }
    bool IsLocalPlayer() const { return bIsLocalPlayer; }

    void SetVolume(float InVolume) { Volume = FMath::Clamp(InVolume, 0.0f, 2.0f); }
    void SetMuted(bool bInMuted) { bMuted = bInMuted; }

private:
    UPROPERTY(Transient)
    TObjectPtr<UProximityVoiceLocalPlayerSubsystem> VoiceSubsystem = nullptr;

    UPROPERTY(Transient)
    FString PlayerId;

    UPROPERTY(Transient)
    FText DisplayName;

    UPROPERTY(Transient)
    bool bIsTalking = false;

    UPROPERTY(Transient)
    float Volume = 1.0f;

    UPROPERTY(Transient)
    bool bMuted = false;

    UPROPERTY(Transient)
    int32 PingMs = INDEX_NONE;

    UPROPERTY(Transient)
    bool bIsLocalPlayer = false;
};
