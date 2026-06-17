#include "Framework/EMRLobbyGameMode.h"

#include "Framework/EMRLobbyGameState.h"
#include "Framework/EMRLobbyPlayerSlot.h"
#include "Subsystems/EMRRunProgressionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbyPlayerController.h"
#include "Utils/EMREndSessionTrace.h"

AEMRLobbyGameMode::AEMRLobbyGameMode()
{
    GameStateClass = AEMRLobbyGameState::StaticClass();
    bUseSeamlessTravel = true;
}

void AEMRLobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.BeginPlay auth=%d netmode=%d map=%s"),
        HasAuthority() ? 1 : 0,
        static_cast<int32>(GetNetMode()),
        GetWorld() ? *GetWorld()->GetMapName() : TEXT("<null>"));

    if (!HasAuthority())
    {
        return;
    }

    ResetRunProgressionForFreshSession();

    if (AEMRLobbyGameState* LobbyGameState = GetLobbyGameState())
    {
        LobbyGameState->OnLobbySlotRegistered.AddUObject(this, &AEMRLobbyGameMode::HandleLobbySlotRegistered);
        AssignSlotsForExistingPlayers();
        TryAssignPendingPlayers();
    }

    // Invite code generation now happens in the lobby session subsystem after session creation.
}

void AEMRLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!HasAuthority() || !NewPlayer)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.PostLogin skip auth=%d controller=%s"),
            HasAuthority() ? 1 : 0,
            *GetNameSafe(NewPlayer));
        return;
    }

    APlayerState* PlayerState = NewPlayer->PlayerState;
    if (!PlayerState)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LobbyGameMode] PostLogin - PlayerState missing for %s"), *GetNameSafe(NewPlayer));
        return;
    }

    if (AEMRLobbyGameState* LobbyGameState = GetLobbyGameState())
    {
        const bool bAssigned = LobbyGameState->AssignSlotToPlayer(PlayerState);
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.PostLogin player=%s assigned=%d registeredSlots=%d pendingBefore=%d"),
            *GetNameSafe(PlayerState),
            bAssigned ? 1 : 0,
            LobbyGameState->GetRegisteredSlotCount(),
            PendingPlayerStates.Num());
        if (!bAssigned)
        {
            PendingPlayerStates.AddUnique(PlayerState);
            EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.PostLogin queued pending player=%s pendingAfter=%d"),
                *GetNameSafe(PlayerState),
                PendingPlayerStates.Num());
        }
    }
}

void AEMRLobbyGameMode::Logout(AController* Exiting)
{
    APlayerState* PlayerState = Exiting ? Exiting->PlayerState : nullptr;
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.Logout auth=%d controller=%s player=%s"),
        HasAuthority() ? 1 : 0,
        *GetNameSafe(Exiting),
        *GetNameSafe(PlayerState));

    if (HasAuthority() && PlayerState)
    {
        RemovePendingPlayerState(PlayerState);
        if (AEMRLobbyGameState* LobbyGameState = GetLobbyGameState())
        {
            LobbyGameState->ClearSlotForPlayer(PlayerState);
        }
    }

    Super::Logout(Exiting);
}

bool AEMRLobbyGameMode::StartLobbyGame()
{
    if (!HasAuthority())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.StartLobbyGame rejected: no authority"));
        return false;
    }

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.StartLobbyGame begin players=%d"),
        GameState && GameState->PlayerArray.IsValidIndex(0) ? GameState->PlayerArray.Num() : 0);

    ResetRunProgressionForFreshSession();

    return TravelToHubLevel();
}

bool AEMRLobbyGameMode::KickLobbyPlayer(APlayerState* TargetPlayerState)
{
    if (!HasAuthority() || !TargetPlayerState)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    APlayerController* TargetController = nullptr;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* Controller = It->Get();
        if (Controller && Controller->PlayerState == TargetPlayerState)
        {
            TargetController = Controller;
            break;
        }
    }

    if (!TargetController)
    {
        return false;
    }

    if (AEMRFrontendLobbyPlayerController* LobbyController = Cast<AEMRFrontendLobbyPlayerController>(TargetController))
    {
        if (AEMRLobbyGameState* LobbyGameState = GetLobbyGameState())
        {
            LobbyGameState->ClearSlotForPlayer(TargetPlayerState);
        }

        if (APawn* TargetPawn = TargetController->GetPawn())
        {
            TargetController->UnPossess();
            TargetPawn->Destroy();
        }

        LobbyController->Client_HandleKickedFromLobby();
        return true;
    }

    return false;
}

AEMRLobbyGameState* AEMRLobbyGameMode::GetLobbyGameState() const
{
    return GetGameState<AEMRLobbyGameState>();
}

void AEMRLobbyGameMode::HandleLobbySlotRegistered(AEMRLobbyPlayerSlot* SlotActor)
{
    if (!HasAuthority() || !SlotActor)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.HandleLobbySlotRegistered skip auth=%d slot=%s"),
            HasAuthority() ? 1 : 0,
            *GetNameSafe(SlotActor));
        return;
    }

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.HandleLobbySlotRegistered slot=%s index=%d"),
        *GetNameSafe(SlotActor),
        SlotActor->GetSlotIndex());
    TryAssignPendingPlayers();
}

void AEMRLobbyGameMode::RemovePendingPlayerState(APlayerState* PlayerState)
{
    if (!PlayerState)
    {
        return;
    }

    PendingPlayerStates.Remove(PlayerState);
}

void AEMRLobbyGameMode::TryAssignPendingPlayers()
{
    if (!HasAuthority())
    {
        return;
    }

    AEMRLobbyGameState* LobbyGameState = GetLobbyGameState();
    if (!LobbyGameState)
    {
        return;
    }

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.TryAssignPendingPlayers pending=%d slots=%d players=%d"),
        PendingPlayerStates.Num(),
        LobbyGameState->GetRegisteredSlotCount(),
        LobbyGameState->PlayerArray.Num());

    for (int32 Index = PendingPlayerStates.Num() - 1; Index >= 0; --Index)
    {
        APlayerState* PlayerState = PendingPlayerStates[Index].Get();
        if (!PlayerState)
        {
            PendingPlayerStates.RemoveAt(Index);
            continue;
        }

        if (LobbyGameState->AssignSlotToPlayer(PlayerState))
        {
            EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.TryAssignPendingPlayers assigned %s"), *GetNameSafe(PlayerState));
            PendingPlayerStates.RemoveAt(Index);
        }
    }
}

void AEMRLobbyGameMode::AssignSlotsForExistingPlayers()
{
    if (!HasAuthority())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.AssignSlotsForExistingPlayers skip no authority"));
        return;
    }

    AEMRLobbyGameState* LobbyGameState = GetLobbyGameState();
    if (!LobbyGameState)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.AssignSlotsForExistingPlayers skip missing LobbyGameState"));
        return;
    }

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.AssignSlotsForExistingPlayers begin players=%d slots=%d"),
        LobbyGameState->PlayerArray.Num(),
        LobbyGameState->GetRegisteredSlotCount());

    for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
    {
        if (!PlayerState)
        {
            continue;
        }

        if (LobbyGameState->FindSlotForPlayer(PlayerState))
        {
            RemovePendingPlayerState(PlayerState);
            continue;
        }

        const bool bAssigned = LobbyGameState->AssignSlotToPlayer(PlayerState);
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.AssignSlotsForExistingPlayers player=%s assigned=%d"),
            *GetNameSafe(PlayerState),
            bAssigned ? 1 : 0);
        if (!bAssigned)
        {
            PendingPlayerStates.AddUnique(PlayerState);
            continue;
        }

        RemovePendingPlayerState(PlayerState);
    }
}

bool AEMRLobbyGameMode::TravelToHubLevel()
{
    if (HubLevel.IsNull())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.TravelToHubLevel aborted HubLevel is null"));
        return false;
    }

    const FSoftObjectPath LevelPath = HubLevel.ToSoftObjectPath();
    const FString LevelLongName = LevelPath.GetLongPackageName();
    if (LevelLongName.IsEmpty())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.TravelToHubLevel aborted invalid HubLevel"));
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.TravelToHubLevel aborted missing world"));
        return false;
    }

    const FString SanitizedLevelLongName = UWorld::RemovePIEPrefix(LevelLongName);
    const FString ClientTravelURL = SanitizedLevelLongName;
    const FString ServerTravelURL = SanitizedLevelLongName.Contains(TEXT("?"))
    ? SanitizedLevelLongName + TEXT("&listen")
    : SanitizedLevelLongName + TEXT("?listen");

    // NOTE: Do not call ClientTravel on connected clients here. ServerTravel will migrate them automatically.
    // Calling ClientTravel in parallel can cause net driver/world teardown issues (Fatal World Leaks).

    const bool bWasSeamless = bUseSeamlessTravel;
    if (World->WorldType == EWorldType::PIE)
    {
        bUseSeamlessTravel = false;
    }

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.TravelToHubLevel ServerTravelURL=%s ClientTravelURL=%s"),
        *ServerTravelURL,
        *ClientTravelURL);
    World->ServerTravel(ServerTravelURL, true);
    bUseSeamlessTravel = bWasSeamless;
    return true;
}

void AEMRLobbyGameMode::NotifyControllersOfPendingTravel(const FString& TravelURL)
{
    // Deprecated: Travel is driven by ServerTravel. Kept as a hook for future UI-only notifications.
    // Intentionally does not call ClientTravel.
    (void)TravelURL;
}

void AEMRLobbyGameMode::ResetRunProgressionForFreshSession() const
{
    if (UEMRRunProgressionSubsystem* RunProgressionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRRunProgressionSubsystem>() : nullptr)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.ResetRunProgressionForFreshSession reset cached progression"));
        RunProgressionSubsystem->ResetCachedState();
    }
    else
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyGameMode.ResetRunProgressionForFreshSession missing RunProgressionSubsystem"));
    }
}

