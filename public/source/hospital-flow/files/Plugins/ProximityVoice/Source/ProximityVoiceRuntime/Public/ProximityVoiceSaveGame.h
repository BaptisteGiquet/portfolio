#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ProximityVoiceTypes.h"
#include "ProximityVoiceSaveGame.generated.h"

UCLASS()
class PROXIMITYVOICERUNTIME_API UProximityVoiceSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    TMap<FString, FProximityVoiceRemotePreference> RemotePreferences;
};
