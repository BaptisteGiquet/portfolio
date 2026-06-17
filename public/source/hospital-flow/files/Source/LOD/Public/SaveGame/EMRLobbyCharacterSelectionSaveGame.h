#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "EMRLobbyCharacterSelectionSaveGame.generated.h"

UCLASS()
class LOD_API UEMRLobbyCharacterSelectionSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    int32 SelectedCharacterIndex = INDEX_NONE;
};
