#include "Subsystems/EMRLobbyCharacterSelectionSubsystem.h"

#include "Online.h"
#include "Characters/Player/EMRPlayerState.h"
#include "Framework/EMRLobbyGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Lobby/EMRLobbyCharacterCatalog.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "SaveGame/EMRLobbyCharacterSelectionSaveGame.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

void UEMRLobbyCharacterSelectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    LoadSelection();
}

void UEMRLobbyCharacterSelectionSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ApplySelectionRetryHandle);
    }

    Super::Deinitialize();
}

void UEMRLobbyCharacterSelectionSubsystem::SetSelectionIndex(int32 NewIndex)
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    APlayerController* PlayerController = LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
    const AEMRPlayerState* EMRPlayerState = PlayerController ? PlayerController->GetPlayerState<AEMRPlayerState>() : nullptr;
    if (EMRPlayerState && EMRPlayerState->IsLobbyReady())
    {
        return;
    }

    const int32 ResolvedIndex = ResolveSelectionIndex(NewIndex);
    if (ResolvedIndex == CurrentSelectionIndex)
    {
        return;
    }

    CurrentSelectionIndex = ResolvedIndex;
    SaveSelection();
    OnSelectionChanged.Broadcast(CurrentSelectionIndex);
    ApplySelectionToPlayerState();
}

void UEMRLobbyCharacterSelectionSubsystem::ApplySavedSelectionToPlayerState()
{
    if (CurrentSelectionIndex == INDEX_NONE)
    {
        const int32 DefaultIndex = GetFirstCatalogCharacterIndex();
        if (DefaultIndex != INDEX_NONE)
        {
            SetSelectionIndex(DefaultIndex);
        }
        return;
    }

    ApplySelectionToPlayerState();
}

FString UEMRLobbyCharacterSelectionSubsystem::GetSaveSlotName() const
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    const int32 LocalUserNum = LocalPlayer ? LocalPlayer->GetControllerId() : 0;

    if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
    {
        IOnlineIdentityPtr IdentityInterface = Subsystem->GetIdentityInterface();
        if (IdentityInterface.IsValid())
        {
            const TSharedPtr<const FUniqueNetId> UniqueNetId = IdentityInterface->GetUniquePlayerId(LocalUserNum);
            if (UniqueNetId.IsValid())
            {
                return FString::Printf(TEXT("LobbyCharSelection_%s"), *UniqueNetId->ToString());
            }
        }
    }

    static bool bWarnedMissingNetId = false;
    if (!bWarnedMissingNetId)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LobbyCharacterSelection] Missing UniqueNetId. Using local save slot fallback."));
        bWarnedMissingNetId = true;
    }

    return FString::Printf(TEXT("LobbyCharSelection_LocalUser_%d"), LocalUserNum);
}

void UEMRLobbyCharacterSelectionSubsystem::LoadSelection()
{
    const FString SlotName = GetSaveSlotName();
    if (USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SlotName, 0))
    {
        if (const UEMRLobbyCharacterSelectionSaveGame* SelectionSave = Cast<UEMRLobbyCharacterSelectionSaveGame>(LoadedGame))
        {
            CurrentSelectionIndex = SelectionSave->SelectedCharacterIndex;
            return;
        }
    }

    CurrentSelectionIndex = INDEX_NONE;
}

void UEMRLobbyCharacterSelectionSubsystem::SaveSelection() const
{
    UEMRLobbyCharacterSelectionSaveGame* SaveGame = Cast<UEMRLobbyCharacterSelectionSaveGame>(UGameplayStatics::CreateSaveGameObject(UEMRLobbyCharacterSelectionSaveGame::StaticClass()));
    if (!SaveGame)
    {
        return;
    }

    SaveGame->SelectedCharacterIndex = CurrentSelectionIndex;
    UGameplayStatics::SaveGameToSlot(SaveGame, GetSaveSlotName(), 0);
}

void UEMRLobbyCharacterSelectionSubsystem::ApplySelectionToPlayerState()
{
    const int32 ResolvedIndex = ResolveSelectionIndex(CurrentSelectionIndex);
    if (ResolvedIndex != CurrentSelectionIndex)
    {
        CurrentSelectionIndex = ResolvedIndex;
        SaveSelection();
        OnSelectionChanged.Broadcast(CurrentSelectionIndex);
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    APlayerController* PlayerController = LocalPlayer ? LocalPlayer->GetPlayerController(World) : nullptr;
    AEMRPlayerState* EMRPlayerState = PlayerController ? PlayerController->GetPlayerState<AEMRPlayerState>() : nullptr;
    if (!EMRPlayerState)
    {
        if (!World->GetTimerManager().IsTimerActive(ApplySelectionRetryHandle))
        {
            World->GetTimerManager().SetTimer(ApplySelectionRetryHandle, this, &ThisClass::ApplySelectionToPlayerState, 0.2f, true);
        }
        return;
    }

    World->GetTimerManager().ClearTimer(ApplySelectionRetryHandle);
    EMRPlayerState->SetLobbyCharacterIndex(CurrentSelectionIndex);
}

int32 UEMRLobbyCharacterSelectionSubsystem::GetFirstCatalogCharacterIndex() const
{
    const AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr;
    const UEMRLobbyCharacterCatalog* Catalog = LobbyGameState ? LobbyGameState->GetLobbyCharacterCatalog() : nullptr;
    if (!Catalog || Catalog->Characters.IsEmpty())
    {
        return INDEX_NONE;
    }

    return 0;
}

int32 UEMRLobbyCharacterSelectionSubsystem::ResolveSelectionIndex(int32 InIndex) const
{
    const AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr;
    const UEMRLobbyCharacterCatalog* Catalog = LobbyGameState ? LobbyGameState->GetLobbyCharacterCatalog() : nullptr;
    const int32 FirstIndex = GetFirstCatalogCharacterIndex();
    if (FirstIndex == INDEX_NONE || !Catalog)
    {
        return InIndex;
    }

    if (Catalog->Characters.IsValidIndex(InIndex))
    {
        return InIndex;
    }

    return FirstIndex;
}
