#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "EMRLobbyGameState.generated.h"

class AEMRLobbyPlayerSlot;
class APlayerState;
class UEMRLobbyCharacterCatalog;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbySlotRegistered, AEMRLobbyPlayerSlot*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyInviteCodeChanged, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyPlayerStateChanged, APlayerState*);

UCLASS()
class LOD_API AEMRLobbyGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AEMRLobbyGameState();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void AddPlayerState(APlayerState* PlayerState) override;
    virtual void RemovePlayerState(APlayerState* PlayerState) override;

    FOnLobbySlotRegistered OnLobbySlotRegistered;
    FOnLobbyPlayerStateChanged OnLobbyPlayerStateAdded;
    FOnLobbyPlayerStateChanged OnLobbyPlayerStateRemoved;

    void RegisterLobbySlot(AEMRLobbyPlayerSlot* SlotActor);
    void UnregisterLobbySlot(AEMRLobbyPlayerSlot* SlotActor);

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    bool AssignSlotToPlayer(APlayerState* PlayerState);

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    bool ClearSlotForPlayer(APlayerState* PlayerState);

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    AEMRLobbyPlayerSlot* FindSlotForPlayer(APlayerState* PlayerState) const;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    int32 GetRegisteredSlotCount() const;

    UFUNCTION(BlueprintCallable, Category = "Lobby|InviteCode")
    const FString& GetLobbyInviteCode() const { return LobbyInviteCode; }

    UFUNCTION(BlueprintCallable, Category = "Lobby|InviteCode")
    void SetLobbyInviteCode(const FString& InCode);

    FOnLobbyInviteCodeChanged OnLobbyInviteCodeChanged;

    UFUNCTION(BlueprintCallable, Category = "Lobby|Characters")
    const UEMRLobbyCharacterCatalog* GetLobbyCharacterCatalog() const { return LobbyCharacterCatalog; }

private:
    void RebuildSlotCache();
    void SortSlotCache();

    UFUNCTION()
    void OnRep_LobbyInviteCode();

    UPROPERTY()
    TArray<TWeakObjectPtr<AEMRLobbyPlayerSlot>> LobbySlots;

    UPROPERTY(ReplicatedUsing = OnRep_LobbyInviteCode, BlueprintReadOnly, Category = "Lobby|InviteCode", meta = (AllowPrivateAccess = "true"))
    FString LobbyInviteCode;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Characters", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UEMRLobbyCharacterCatalog> LobbyCharacterCatalog = nullptr;

};
