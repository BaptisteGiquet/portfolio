#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRLobbyCharacterDisplayActor.generated.h"

class USkeletalMeshComponent;

UCLASS()
class LOD_API AEMRLobbyCharacterDisplayActor : public AActor
{
    GENERATED_BODY()

public:
    AEMRLobbyCharacterDisplayActor();

    void SetCharacterIndex(int32 NewIndex);

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby|Characters")
    TObjectPtr<USkeletalMeshComponent> MeshComponent;

    UPROPERTY(ReplicatedUsing = OnRep_CharacterIndex)
    int32 CharacterIndex = INDEX_NONE;

    UFUNCTION()
    void OnRep_CharacterIndex();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void ApplyCharacterDefinition();

    void DestroyCharacterActor();
    void RefreshSpawnedActorVisibility();

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> SpawnedCharacterActor;

    FTimerHandle VisibilityRefreshHandle;
    int32 VisibilityRefreshRemaining = 0;
};
