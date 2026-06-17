#pragma once

#include "CoreMinimal.h"
#include "ProximityVoiceTypes.generated.h"

USTRUCT(BlueprintType)
struct PROXIMITYVOICERUNTIME_API FProximityVoiceInputDevice
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Voice")
    FString DeviceId;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Voice")
    FText DisplayName;
};

USTRUCT(BlueprintType)
struct PROXIMITYVOICERUNTIME_API FProximityVoiceRemotePreference
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "EMR|Voice", meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0"))
    float Volume = 1.0f;

    UPROPERTY(BlueprintReadWrite, Category = "EMR|Voice")
    bool bMuted = false;
};

USTRUCT(BlueprintType)
struct PROXIMITYVOICERUNTIME_API FProximityVoiceRemotePlayerView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Voice")
    FString PlayerId;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Voice")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Voice")
    bool bIsTalking = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Voice")
    float VoiceLevel = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Voice")
    int32 PingMs = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Voice")
    bool bIsLocalPlayer = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Voice")
    FProximityVoiceRemotePreference Preference;
};

DECLARE_MULTICAST_DELEGATE(FOnProximityVoiceRosterChanged);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnProximityVoiceRemoteTalkingChanged, const FString& /*RemotePlayerId*/, bool /*bIsTalking*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnProximityVoiceLocalMicActivityChanged, bool /*bIsActive*/);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProximityVoiceRosterChangedDynamic);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProximityVoiceRemoteTalkingChangedDynamic, const FString&, RemotePlayerId, bool, bIsTalking);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProximityVoiceLocalMicActivityChangedDynamic, bool, bIsActive);
