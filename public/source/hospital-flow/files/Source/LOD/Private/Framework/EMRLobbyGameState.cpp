#include "Framework/EMRLobbyGameState.h"

#include "Framework/EMRLobbyPlayerSlot.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Utils/EMREndSessionTrace.h"

AEMRLobbyGameState::AEMRLobbyGameState()
{
}

void AEMRLobbyGameState::BeginPlay()
{
    Super::BeginPlay();

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.BeginPlay auth=%d netmode=%d map=%s"),
        HasAuthority() ? 1 : 0,
        static_cast<int32>(GetNetMode()),
        GetWorld() ? *GetWorld()->GetMapName() : TEXT("<null>"));

    if (HasAuthority())
    {
        RebuildSlotCache();
    }
}

void AEMRLobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
    Super::AddPlayerState(PlayerState);

    if (PlayerState)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.AddPlayerState player=%s totalPlayers=%d"),
            *GetNameSafe(PlayerState),
            PlayerArray.Num());
        OnLobbyPlayerStateAdded.Broadcast(PlayerState);
    }
}

void AEMRLobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
    Super::RemovePlayerState(PlayerState);

    if (PlayerState)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.RemovePlayerState player=%s totalPlayers=%d"),
            *GetNameSafe(PlayerState),
            PlayerArray.Num());
        OnLobbyPlayerStateRemoved.Broadcast(PlayerState);
    }
}

void AEMRLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRLobbyGameState, LobbyInviteCode);
}

void AEMRLobbyGameState::SetLobbyInviteCode(const FString& InCode)
{
    if (!HasAuthority())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.SetLobbyInviteCode skip no authority"));
        return;
    }

    if (LobbyInviteCode == InCode)
    {
        return;
    }

    LobbyInviteCode = InCode;
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.SetLobbyInviteCode updated code=%s"), *LobbyInviteCode);
    OnRep_LobbyInviteCode();
}

void AEMRLobbyGameState::OnRep_LobbyInviteCode()
{
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.OnRep_LobbyInviteCode code=%s"), *LobbyInviteCode);
    OnLobbyInviteCodeChanged.Broadcast(LobbyInviteCode);
}

void AEMRLobbyGameState::RegisterLobbySlot(AEMRLobbyPlayerSlot* SlotActor)
{
    if (!SlotActor)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.RegisterLobbySlot skip null slot"));
        return;
    }

    if (!LobbySlots.Contains(SlotActor))
    {
        LobbySlots.Add(SlotActor);
        SortSlotCache();
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.RegisterLobbySlot slot=%s index=%d totalSlots=%d"),
            *GetNameSafe(SlotActor),
            SlotActor->GetSlotIndex(),
            LobbySlots.Num());
        OnLobbySlotRegistered.Broadcast(SlotActor);
    }
}

void AEMRLobbyGameState::UnregisterLobbySlot(AEMRLobbyPlayerSlot* SlotActor)
{
    if (!SlotActor)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.UnregisterLobbySlot skip null slot"));
        return;
    }

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.UnregisterLobbySlot slot=%s index=%d before=%d"),
        *GetNameSafe(SlotActor),
        SlotActor->GetSlotIndex(),
        LobbySlots.Num());
    LobbySlots.Remove(SlotActor);
}


bool AEMRLobbyGameState::AssignSlotToPlayer(APlayerState* PlayerState)
{
    if (!HasAuthority() || !PlayerState)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.AssignSlotToPlayer skip auth=%d player=%s"),
            HasAuthority() ? 1 : 0,
            *GetNameSafe(PlayerState));
        return false;
    }

    if (AEMRLobbyPlayerSlot* ExistingSlot = FindSlotForPlayer(PlayerState))
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.AssignSlotToPlayer already assigned player=%s slot=%s index=%d"),
            *GetNameSafe(PlayerState),
            *GetNameSafe(ExistingSlot),
            ExistingSlot->GetSlotIndex());
        return true;
    }

    for (const TWeakObjectPtr<AEMRLobbyPlayerSlot>& SlotPtr : LobbySlots)
    {
        AEMRLobbyPlayerSlot* SlotActor = SlotPtr.Get();
        if (!SlotActor || SlotActor->IsOccupied())
        {
            continue;
        }

        const bool bAssigned = SlotActor->AssignPlayerState(PlayerState);
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.AssignSlotToPlayer try player=%s slot=%s index=%d assigned=%d"),
            *GetNameSafe(PlayerState),
            *GetNameSafe(SlotActor),
            SlotActor->GetSlotIndex(),
            bAssigned ? 1 : 0);
        return bAssigned;
    }

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.AssignSlotToPlayer failed no slot for player=%s totalSlots=%d"),
        *GetNameSafe(PlayerState),
        LobbySlots.Num());
    return false;
}

bool AEMRLobbyGameState::ClearSlotForPlayer(APlayerState* PlayerState)
{
    if (!HasAuthority() || !PlayerState)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.ClearSlotForPlayer skip auth=%d player=%s"),
            HasAuthority() ? 1 : 0,
            *GetNameSafe(PlayerState));
        return false;
    }

    if (AEMRLobbyPlayerSlot* SlotActor = FindSlotForPlayer(PlayerState))
    {
        const bool bCleared = SlotActor->ClearPlayerState();
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.ClearSlotForPlayer player=%s slot=%s index=%d cleared=%d"),
            *GetNameSafe(PlayerState),
            *GetNameSafe(SlotActor),
            SlotActor->GetSlotIndex(),
            bCleared ? 1 : 0);
        return bCleared;
    }

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.ClearSlotForPlayer no slot found player=%s"), *GetNameSafe(PlayerState));
    return false;
}

AEMRLobbyPlayerSlot* AEMRLobbyGameState::FindSlotForPlayer(APlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return nullptr;
    }

    for (const TWeakObjectPtr<AEMRLobbyPlayerSlot>& SlotPtr : LobbySlots)
    {
        AEMRLobbyPlayerSlot* SlotActor = SlotPtr.Get();
        if (SlotActor && SlotActor->GetAssignedPlayerState() == PlayerState)
        {
            return SlotActor;
        }
    }

    return nullptr;
}

int32 AEMRLobbyGameState::GetRegisteredSlotCount() const
{
    return LobbySlots.Num();
}

void AEMRLobbyGameState::RebuildSlotCache()
{
    LobbySlots.Reset();

    if (!GetWorld())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.RebuildSlotCache aborted missing world"));
        return;
    }

    for (TActorIterator<AEMRLobbyPlayerSlot> It(GetWorld()); It; ++It)
    {
        LobbySlots.Add(*It);
    }

    SortSlotCache();
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameState.RebuildSlotCache completed totalSlots=%d"),
        LobbySlots.Num());
}

void AEMRLobbyGameState::SortSlotCache()
{
    LobbySlots.Sort([](const TWeakObjectPtr<AEMRLobbyPlayerSlot>& A, const TWeakObjectPtr<AEMRLobbyPlayerSlot>& B)
    {
        const AEMRLobbyPlayerSlot* SlotA = A.Get();
        const AEMRLobbyPlayerSlot* SlotB = B.Get();

        if (!SlotA || !SlotB)
        {
            return SlotA != nullptr;
        }

        return SlotA->GetSlotIndex() < SlotB->GetSlotIndex();
    });
}

