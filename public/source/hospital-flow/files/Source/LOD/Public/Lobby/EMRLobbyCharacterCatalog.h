#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EMRLobbyCharacterCatalog.generated.h"

class USkeletalMesh;
class UAnimInstance;
class AEMRPlayerCharacter;

USTRUCT(BlueprintType)
struct FLobbyCharacterDefinition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters")
    TSubclassOf<AEMRPlayerCharacter> PlayerCharacterClass = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters")
    TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters")
    TSubclassOf<UAnimInstance> AnimClass = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters")
    FTransform RelativeTransform = FTransform::Identity;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters")
    FRotator PreviewRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters")
    FRotator SlotRotationOffset = FRotator::ZeroRotator;
};

UCLASS(BlueprintType)
class LOD_API UEMRLobbyCharacterCatalog : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters")
    TArray<FLobbyCharacterDefinition> Characters;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters")
    int32 DefaultCharacterIndex = 0;

    const FLobbyCharacterDefinition* GetCharacterDefinition(int32 Index) const;
};
