#include "Framework/EMRLobbyPlayerSlot.h"

#include "Framework/EMRLobbyGameState.h"
#include "Characters/Player/EMRPlayerState.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "Lobby/EMRLobbyCharacterDisplayActor.h"
#include "Net/UnrealNetwork.h"
#include "UI/Frontend/Utils/DebugHelper.h"
#include "UI/Frontend/Screens/Lobby/EMRFrontendPlayerSlotInfosWidget.h"
#include "Utils/EMREndSessionTrace.h"

AEMRLobbyPlayerSlot::AEMRLobbyPlayerSlot()
{
    bReplicates = true;
    bAlwaysRelevant = true;

    CharacterDisplayActorClass = AEMRLobbyCharacterDisplayActor::StaticClass();
}


void AEMRLobbyPlayerSlot::BeginPlay()
{
    Super::BeginPlay();

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.BeginPlay slot=%s index=%d auth=%d netmode=%d"),
        *GetNameSafe(this),
        SlotIndex,
        HasAuthority() ? 1 : 0,
        static_cast<int32>(GetNetMode()));

    CachePlayerInfoWidget();
    if (CachedPlayerInfoWidgetComponent.IsValid() && !IsOccupied())
    {
        CachedPlayerInfoWidgetComponent->SetHiddenInGame(true);
        CachedPlayerInfoWidgetComponent->SetVisibility(false);
    }

    if (HasAuthority())
    {
        if (AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr)
        {
            LobbyGameState->RegisterLobbySlot(this);
        }
    }
}


void AEMRLobbyPlayerSlot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.EndPlay slot=%s index=%d reason=%d"),
        *GetNameSafe(this),
        SlotIndex,
        static_cast<int32>(EndPlayReason));

    if (HasAuthority())
    {
        if (AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr)
        {
            LobbyGameState->UnregisterLobbySlot(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}


bool AEMRLobbyPlayerSlot::AssignPlayerState(APlayerState* NewPlayerState)
{
    if (!HasAuthority() || !NewPlayerState)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.AssignPlayerState rejected slot=%s index=%d auth=%d player=%s"),
            *GetNameSafe(this),
            SlotIndex,
            HasAuthority() ? 1 : 0,
            *GetNameSafe(NewPlayerState));
        return false;
    }

    if (AssignedPlayerState == NewPlayerState)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.AssignPlayerState noop slot=%s index=%d same player=%s"),
            *GetNameSafe(this),
            SlotIndex,
            *GetNameSafe(NewPlayerState));
        return true;
    }

    if (AssignedPlayerState)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.AssignPlayerState rejected occupied slot=%s index=%d current=%s incoming=%s"),
            *GetNameSafe(this),
            SlotIndex,
            *GetNameSafe(AssignedPlayerState),
            *GetNameSafe(NewPlayerState));
        return false;
    }

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.AssignPlayerState assigning slot=%s index=%d player=%s"),
        *GetNameSafe(this),
        SlotIndex,
        *GetNameSafe(NewPlayerState));
    SetAssignedPlayerState(NewPlayerState);
    return true;
}

bool AEMRLobbyPlayerSlot::ClearPlayerState()
{
    if (!HasAuthority())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.ClearPlayerState rejected no authority slot=%s index=%d"),
            *GetNameSafe(this),
            SlotIndex);
        return false;
    }

    if (!AssignedPlayerState)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.ClearPlayerState noop empty slot=%s index=%d"),
            *GetNameSafe(this),
            SlotIndex);
        return true;
    }

    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.ClearPlayerState clearing slot=%s index=%d oldPlayer=%s"),
        *GetNameSafe(this),
        SlotIndex,
        *GetNameSafe(AssignedPlayerState));
    SetAssignedPlayerState(nullptr);
    return true;
}


bool AEMRLobbyPlayerSlot::IsOccupied() const
{
    return AssignedPlayerState != nullptr;
}


APlayerState* AEMRLobbyPlayerSlot::GetAssignedPlayerState() const
{
    return AssignedPlayerState;
}


int32 AEMRLobbyPlayerSlot::GetSlotIndex() const
{
    return SlotIndex;
}


bool AEMRLobbyPlayerSlot::GetLobbyPlayerInfo(FLobbyPlayerInfo& OutLobbyPlayerInfo) const
{
    const AEMRPlayerState* EMRPlayerState = GetAssignedEMRPlayerState();
    if (!EMRPlayerState)
    {
        return false;
    }

    OutLobbyPlayerInfo = EMRPlayerState->GetLobbyPlayerInfo();
    return true;
}


AEMRPlayerState* AEMRLobbyPlayerSlot::GetAssignedEMRPlayerState() const
{
    return Cast<AEMRPlayerState>(AssignedPlayerState);
}


void AEMRLobbyPlayerSlot::HandleSlotAssigned_Implementation(APlayerState* NewPlayerState)
{
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.HandleSlotAssigned slot=%s index=%d auth=%d player=%s"),
        *GetNameSafe(this),
        SlotIndex,
        HasAuthority() ? 1 : 0,
        *GetNameSafe(NewPlayerState));

    AEMRPlayerState* EMRPlayerState = Cast<AEMRPlayerState>(NewPlayerState);
    if (!EMRPlayerState)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.HandleSlotAssigned invalid EMRPlayerState slot=%s player=%s"),
            *GetNameSafe(this),
            *GetNameSafe(NewPlayerState));
        return;
    }

    BoundPlayerState = EMRPlayerState;
    EMRPlayerState->OnLobbyPlayerInfoChanged.AddDynamic(this, &ThisClass::HandleLobbyPlayerInfoChanged);
    HandleLobbyPlayerInfoChanged(EMRPlayerState->GetLobbyPlayerInfo());

    CachePlayerInfoWidget();
    if (CachedPlayerInfoWidgetInstance.IsValid())
    {
        CachedPlayerInfoWidgetInstance->SetAssignedPlayerState(EMRPlayerState);
    }

    if (HasAuthority())
    {
        SpawnCharacterDisplayActor();
        UpdateCharacterDisplayActor(EMRPlayerState->GetLobbyPlayerInfo());
    }
}

void AEMRLobbyPlayerSlot::HandleSlotCleared_Implementation(APlayerState* OldPlayerState)
{
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.HandleSlotCleared slot=%s index=%d auth=%d oldPlayer=%s"),
        *GetNameSafe(this),
        SlotIndex,
        HasAuthority() ? 1 : 0,
        *GetNameSafe(OldPlayerState));

    AEMRPlayerState* EMRPlayerState = Cast<AEMRPlayerState>(OldPlayerState);
    if (EMRPlayerState)
    {
        EMRPlayerState->OnLobbyPlayerInfoChanged.RemoveDynamic(this, &ThisClass::HandleLobbyPlayerInfoChanged);
    }

    BoundPlayerState.Reset();

    CachePlayerInfoWidget();
    if (CachedPlayerInfoWidgetInstance.IsValid())
    {
        CachedPlayerInfoWidgetInstance->ClearPlayerInfo();
        CachedPlayerInfoWidgetInstance->SetAssignedPlayerState(nullptr);
    }

    if (CachedPlayerInfoWidgetComponent.IsValid())
    {
        CachedPlayerInfoWidgetComponent->SetHiddenInGame(true);
        CachedPlayerInfoWidgetComponent->SetVisibility(false);
    }

    if (HasAuthority())
    {
        DestroyCharacterDisplayActor();
    }
}


void AEMRLobbyPlayerSlot::OnRep_AssignedPlayerState(APlayerState* PreviousPlayerState)
{
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.OnRep_AssignedPlayerState slot=%s index=%d prev=%s new=%s"),
        *GetNameSafe(this),
        SlotIndex,
        *GetNameSafe(PreviousPlayerState),
        *GetNameSafe(AssignedPlayerState));
    NotifyAssignmentChanged(PreviousPlayerState, AssignedPlayerState);
}


void AEMRLobbyPlayerSlot::HandleLobbyPlayerInfoChanged(const FLobbyPlayerInfo& Info)
{
    const FString PlayerNameString = Info.PlayerName.ToString();
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.HandleLobbyPlayerInfoChanged slot=%s index=%d playerName=%s ready=%d characterIndex=%d auth=%d"),
        *GetNameSafe(this),
        SlotIndex,
        *PlayerNameString,
        Info.bIsPlayerReady ? 1 : 0,
        Info.SelectedCharacterIndex,
        HasAuthority() ? 1 : 0);

    OnLobbyPlayerInfoChanged(Info);

    CachePlayerInfoWidget();
    if (CachedPlayerInfoWidgetInstance.IsValid())
    {
        CachedPlayerInfoWidgetInstance->SetPlayerInfo(Info.PlayerName, Info.bIsPlayerReady);
    }

    if (CachedPlayerInfoWidgetComponent.IsValid())
    {
        CachedPlayerInfoWidgetComponent->SetHiddenInGame(false);
        CachedPlayerInfoWidgetComponent->SetVisibility(true);
    }

    if (HasAuthority())
    {
        UpdateCharacterDisplayActor(Info);
    }
}


void AEMRLobbyPlayerSlot::CachePlayerInfoWidget()
{
    if (CachedPlayerInfoWidgetComponent.IsValid())
    {
        if (!CachedPlayerInfoWidgetInstance.IsValid())
        {
            CachedPlayerInfoWidgetInstance = Cast<UEMRFrontendPlayerSlotInfosWidget>(CachedPlayerInfoWidgetComponent->GetUserWidgetObject());
        }
        return;
    }

    TInlineComponentArray<UWidgetComponent*> WidgetComponents(this);
    for (UWidgetComponent* WidgetComponent : WidgetComponents)
    {
        if (!WidgetComponent)
        {
            continue;
        }

        UClass* WidgetClass = WidgetComponent->GetWidgetClass();
        if (WidgetClass && WidgetClass->IsChildOf(UEMRFrontendPlayerSlotInfosWidget::StaticClass()))
        {
            CachedPlayerInfoWidgetComponent = WidgetComponent;
            CachedPlayerInfoWidgetInstance = Cast<UEMRFrontendPlayerSlotInfosWidget>(WidgetComponent->GetUserWidgetObject());
            CachedPlayerInfoWidgetComponent->SetDrawAtDesiredSize(true);
            CachedPlayerInfoWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            break;
        }
    }
}


void AEMRLobbyPlayerSlot::SetAssignedPlayerState(APlayerState* NewPlayerState)
{
    APlayerState* PreviousPlayerState = AssignedPlayerState;
    AssignedPlayerState = NewPlayerState;

    if (HasAuthority())
    {
    	NotifyAssignmentChanged(PreviousPlayerState, AssignedPlayerState);
    }
}


void AEMRLobbyPlayerSlot::NotifyAssignmentChanged(APlayerState* PreviousPlayerState, APlayerState* NewPlayerState)
{
    if (PreviousPlayerState && PreviousPlayerState != NewPlayerState)
    {
        HandleSlotCleared(PreviousPlayerState);
    }

    if (NewPlayerState && PreviousPlayerState != NewPlayerState)
    {
        HandleSlotAssigned(NewPlayerState);
    }
}


void AEMRLobbyPlayerSlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRLobbyPlayerSlot, AssignedPlayerState);
}

void AEMRLobbyPlayerSlot::SpawnCharacterDisplayActor()
{
    if (!HasAuthority() || SpawnedCharacterDisplayActor.IsValid())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.SpawnCharacterDisplayActor skip slot=%s index=%d auth=%d existing=%d"),
            *GetNameSafe(this),
            SlotIndex,
            HasAuthority() ? 1 : 0,
            SpawnedCharacterDisplayActor.IsValid() ? 1 : 0);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.SpawnCharacterDisplayActor aborted missing world slot=%s index=%d"),
            *GetNameSafe(this),
            SlotIndex);
        return;
    }

    TSubclassOf<AEMRLobbyCharacterDisplayActor> SpawnClass = CharacterDisplayActorClass;
    if (!SpawnClass)
    {
        SpawnClass = AEMRLobbyCharacterDisplayActor::StaticClass();
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AEMRLobbyCharacterDisplayActor* SpawnedActor = World->SpawnActor<AEMRLobbyCharacterDisplayActor>(SpawnClass, GetActorTransform(), SpawnParams);
    if (!SpawnedActor)
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.SpawnCharacterDisplayActor failed spawn slot=%s index=%d class=%s"),
            *GetNameSafe(this),
            SlotIndex,
            *GetNameSafe(SpawnClass));
        return;
    }

    SpawnedActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    SpawnedCharacterDisplayActor = SpawnedActor;
    EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.SpawnCharacterDisplayActor spawned slot=%s index=%d actor=%s"),
        *GetNameSafe(this),
        SlotIndex,
        *GetNameSafe(SpawnedActor));
}

void AEMRLobbyPlayerSlot::DestroyCharacterDisplayActor()
{
    if (!HasAuthority())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.DestroyCharacterDisplayActor skip no authority slot=%s index=%d"),
            *GetNameSafe(this),
            SlotIndex);
        return;
    }

    if (AEMRLobbyCharacterDisplayActor* SpawnedActor = SpawnedCharacterDisplayActor.Get())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.DestroyCharacterDisplayActor destroying slot=%s index=%d actor=%s"),
            *GetNameSafe(this),
            SlotIndex,
            *GetNameSafe(SpawnedActor));
        SpawnedActor->Destroy();
    }

    SpawnedCharacterDisplayActor.Reset();
}

void AEMRLobbyPlayerSlot::UpdateCharacterDisplayActor(const FLobbyPlayerInfo& Info)
{
    if (!HasAuthority())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.UpdateCharacterDisplayActor skip no authority slot=%s index=%d"),
            *GetNameSafe(this),
            SlotIndex);
        return;
    }

    if (AEMRLobbyCharacterDisplayActor* SpawnedActor = SpawnedCharacterDisplayActor.Get())
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.UpdateCharacterDisplayActor slot=%s index=%d actor=%s characterIndex=%d"),
            *GetNameSafe(this),
            SlotIndex,
            *GetNameSafe(SpawnedActor),
            Info.SelectedCharacterIndex);
        SpawnedActor->SetCharacterIndex(Info.SelectedCharacterIndex);
    }
    else
    {
        EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPlayerSlot.UpdateCharacterDisplayActor missing actor slot=%s index=%d characterIndex=%d"),
            *GetNameSafe(this),
            SlotIndex,
            Info.SelectedCharacterIndex);
    }
}

