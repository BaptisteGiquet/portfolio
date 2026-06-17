#include "Framework/EMRHubGameState.h"

#include "Framework/EMRHubPlayerSlot.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"

AEMRHubGameState::AEMRHubGameState()
{
    // Hub shares the same replicated data set as the run GameState but can remain lightweight.
}

void AEMRHubGameState::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        RebuildSlotCache();
    }
}

void AEMRHubGameState::RegisterHubSlot(AEMRHubPlayerSlot* SlotActor)
{
    if (!SlotActor)
    {
        return;
    }

    if (!HubSlots.Contains(SlotActor))
    {
        HubSlots.Add(SlotActor);
        SortSlotCache();
        OnHubSlotRegistered.Broadcast(SlotActor);
    }
}

void AEMRHubGameState::UnregisterHubSlot(AEMRHubPlayerSlot* SlotActor)
{
    if (!SlotActor)
    {
        return;
    }

    HubSlots.Remove(SlotActor);
}

bool AEMRHubGameState::AssignSlotToPlayer(APlayerState* PlayerState)
{
    if (!HasAuthority() || !PlayerState)
    {
        return false;
    }

    if (!IsRunPhaseEligibleForHubSlotAssignment(GetRunPhase()))
    {
        return false;
    }

    if (AEMRHubPlayerSlot* ExistingSlot = FindSlotForPlayer(PlayerState))
    {
        return true;
    }

    for (const TWeakObjectPtr<AEMRHubPlayerSlot>& SlotPtr : HubSlots)
    {
        AEMRHubPlayerSlot* SlotActor = SlotPtr.Get();
        if (!SlotActor || SlotActor->IsOccupied())
        {
            continue;
        }

        return SlotActor->AssignPlayerState(PlayerState);
    }

    UE_LOG(LogTemp, Warning, TEXT("[HubGameState] No available hub slots for %s"), *GetNameSafe(PlayerState));
    return false;
}

bool AEMRHubGameState::ClearSlotForPlayer(APlayerState* PlayerState)
{
    if (!HasAuthority() || !PlayerState)
    {
        return false;
    }

    if (AEMRHubPlayerSlot* SlotActor = FindSlotForPlayer(PlayerState))
    {
        return SlotActor->ClearPlayerState();
    }

    return false;
}

AEMRHubPlayerSlot* AEMRHubGameState::FindSlotForPlayer(APlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return nullptr;
    }

    for (const TWeakObjectPtr<AEMRHubPlayerSlot>& SlotPtr : HubSlots)
    {
        AEMRHubPlayerSlot* SlotActor = SlotPtr.Get();
        if (SlotActor && SlotActor->GetAssignedPlayerState() == PlayerState)
        {
            return SlotActor;
        }
    }

    return nullptr;
}

int32 AEMRHubGameState::GetRegisteredSlotCount() const
{
    return HubSlots.Num();
}

bool AEMRHubGameState::IsRunPhaseEligibleForHubSlotAssignment(const EER_RunPhase InRunPhase) const
{
    return InRunPhase == EER_RunPhase::Hub
    || InRunPhase == EER_RunPhase::MissionFinal
    || InRunPhase == EER_RunPhase::RunFinished;
}

void AEMRHubGameState::RebuildSlotCache()
{
    HubSlots.Reset();

    if (!GetWorld())
    {
        return;
    }

    for (TActorIterator<AEMRHubPlayerSlot> It(GetWorld()); It; ++It)
    {
        HubSlots.Add(*It);
    }

    SortSlotCache();
}

void AEMRHubGameState::SortSlotCache()
{
    HubSlots.Sort([](const TWeakObjectPtr<AEMRHubPlayerSlot>& A, const TWeakObjectPtr<AEMRHubPlayerSlot>& B)
    {
        const AEMRHubPlayerSlot* SlotA = A.Get();
        const AEMRHubPlayerSlot* SlotB = B.Get();

        if (!SlotA || !SlotB)
        {
            return SlotA != nullptr;
        }

        return SlotA->GetSlotIndex() < SlotB->GetSlotIndex();
    });
}
