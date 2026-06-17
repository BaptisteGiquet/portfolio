#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRLobbyPlayerSlot.generated.h"

class APlayerState;
class AEMRLobbyGameState;
class APawn;
struct FLobbyPlayerInfo;
class AEMRPlayerState;
class UWidgetComponent;
class UEMRFrontendPlayerSlotInfosWidget;
class AEMRLobbyCharacterDisplayActor;

UCLASS()
class LOD_API AEMRLobbyPlayerSlot : public AActor
{
    GENERATED_BODY()

public:
    AEMRLobbyPlayerSlot();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    bool AssignPlayerState(APlayerState* NewPlayerState);

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    bool ClearPlayerState();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    bool IsOccupied() const;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    APlayerState* GetAssignedPlayerState() const;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    int32 GetSlotIndex() const;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    bool GetLobbyPlayerInfo(FLobbyPlayerInfo& OutLobbyPlayerInfo) const;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    AEMRPlayerState* GetAssignedEMRPlayerState() const;
	
    UFUNCTION(BlueprintNativeEvent, Category = "Lobby")
    void HandleSlotAssigned(APlayerState* NewPlayerState);
	
    virtual void HandleSlotAssigned_Implementation(APlayerState* NewPlayerState);

    UFUNCTION(BlueprintNativeEvent, Category = "Lobby")
    void HandleSlotCleared(APlayerState* OldPlayerState);
	
    virtual void HandleSlotCleared_Implementation(APlayerState* OldPlayerState);

protected:
    UFUNCTION()
    void HandleLobbyPlayerInfoChanged(const FLobbyPlayerInfo& Info);

    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
    void OnLobbyPlayerInfoChanged(const FLobbyPlayerInfo& Info);

protected:
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lobby")
    int32 SlotIndex = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters")
    TSubclassOf<AEMRLobbyCharacterDisplayActor> CharacterDisplayActorClass;

    UPROPERTY(ReplicatedUsing = OnRep_AssignedPlayerState)
    APlayerState* AssignedPlayerState = nullptr;


    UFUNCTION()
    void OnRep_AssignedPlayerState(APlayerState* PreviousPlayerState);

    void SetAssignedPlayerState(APlayerState* NewPlayerState);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void NotifyAssignmentChanged(APlayerState* PreviousPlayerState, APlayerState* NewPlayerState);

    void CachePlayerInfoWidget();

    UPROPERTY(Transient)
    TWeakObjectPtr<UWidgetComponent> CachedPlayerInfoWidgetComponent;

    UPROPERTY(Transient)
    TWeakObjectPtr<UEMRFrontendPlayerSlotInfosWidget> CachedPlayerInfoWidgetInstance;

    UPROPERTY()
    TWeakObjectPtr<AEMRPlayerState> BoundPlayerState;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRLobbyCharacterDisplayActor> SpawnedCharacterDisplayActor;

    void SpawnCharacterDisplayActor();
    void DestroyCharacterDisplayActor();
    void UpdateCharacterDisplayActor(const FLobbyPlayerInfo& Info);
};
