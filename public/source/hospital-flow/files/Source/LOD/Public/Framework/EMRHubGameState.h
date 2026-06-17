#pragma once

#include "CoreMinimal.h"
#include "Framework/EMRNightShiftGameState.h"
#include "EMRHubGameState.generated.h"

class AEMRHubPlayerSlot;
class APlayerState;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHubSlotRegistered, AEMRHubPlayerSlot*);

/**
 * Hub-specific GameState that reuses the team ASC + cycle tracking from AEMRNightShiftGameState.
 * Keeps the same replicated data so the UI can remain consistent between Hub and NightShift.
 */
UCLASS()
class LOD_API AEMRHubGameState : public AEMRNightShiftGameState
{
    GENERATED_BODY()

public:
    AEMRHubGameState();

    virtual void BeginPlay() override;

    FOnHubSlotRegistered OnHubSlotRegistered;

    void RegisterHubSlot(AEMRHubPlayerSlot* SlotActor);
    void UnregisterHubSlot(AEMRHubPlayerSlot* SlotActor);

    UFUNCTION(BlueprintCallable, Category = "Hub|Seating")
    bool AssignSlotToPlayer(APlayerState* PlayerState);

    UFUNCTION(BlueprintCallable, Category = "Hub|Seating")
    bool ClearSlotForPlayer(APlayerState* PlayerState);

    UFUNCTION(BlueprintCallable, Category = "Hub|Seating")
    AEMRHubPlayerSlot* FindSlotForPlayer(APlayerState* PlayerState) const;

    UFUNCTION(BlueprintCallable, Category = "Hub|Seating")
    int32 GetRegisteredSlotCount() const;

private:
    bool IsRunPhaseEligibleForHubSlotAssignment(EER_RunPhase InRunPhase) const;
    void RebuildSlotCache();
    void SortSlotCache();

    UPROPERTY()
    TArray<TWeakObjectPtr<AEMRHubPlayerSlot>> HubSlots;
};
