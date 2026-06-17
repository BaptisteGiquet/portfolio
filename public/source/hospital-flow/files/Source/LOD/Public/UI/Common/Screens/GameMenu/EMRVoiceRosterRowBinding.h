#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EMRVoiceRosterRowBinding.generated.h"

class UProximityVoiceLocalPlayerSubsystem;
class UTextBlock;

UCLASS()
class LOD_API UEMRVoiceRosterRowBinding : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UProximityVoiceLocalPlayerSubsystem* InVoiceSubsystem, const FString& InRemotePlayerId, bool bInMuted, UTextBlock* InMuteLabel);

    UFUNCTION()
    void HandleVolumeChanged(float InValue);

    UFUNCTION()
    void HandleMuteClicked();

private:
    void RefreshMuteLabel() const;

    UPROPERTY()
    TObjectPtr<UProximityVoiceLocalPlayerSubsystem> VoiceSubsystem = nullptr;

    UPROPERTY()
    FString RemotePlayerId;

    UPROPERTY()
    bool bMuted = false;

    UPROPERTY()
    TObjectPtr<UTextBlock> MuteLabel = nullptr;
};

