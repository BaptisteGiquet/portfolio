#include "Characters/Player/EMRPlayerState.h"

#include "GAS/EMRAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Framework/EMRLobbyGameState.h"
#include "Lobby/EMRLobbyCharacterCatalog.h"
#include "Net/UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
    void UpdateAbilityActorInfoMesh(UAbilitySystemComponent* ASC, APawn* InPawn)
    {
        if (!ASC || !InPawn)
        {
            return;
        }

        AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(InPawn);
        USkeletalMeshComponent* MeshComponent = PlayerCharacter ? PlayerCharacter->GetPlayerMeshComponentForSeating() : nullptr;
        if (!MeshComponent && InPawn)
        {
            MeshComponent = InPawn->FindComponentByClass<USkeletalMeshComponent>();
        }

        if (MeshComponent && ASC->AbilityActorInfo.IsValid())
        {
            ASC->AbilityActorInfo->SkeletalMeshComponent = MeshComponent;
        }
    }
}


AEMRPlayerState::AEMRPlayerState()
{
    // Replicate to all clients
    SetReplicates(true);
	SetNetUpdateFrequency(10.f);
	
    // Create ASC (authority only, but exists on clients for replication)
    AbilitySystemComponent = CreateDefaultSubobject<UEMRAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

namespace
{
	const TCHAR* GetNetModeLabel(ENetMode NetMode)
	{
		switch (NetMode)
		{
		case NM_Client:
			return TEXT("Client");
		case NM_ListenServer:
			return TEXT("ListenServer");
		case NM_DedicatedServer:
			return TEXT("DedicatedServer");
		default:
			return TEXT("Standalone");
		}
	}
}


void AEMRPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(AEMRPlayerState, bAbilityActorInfoReady, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(AEMRPlayerState, LobbyPlayerInfo, COND_None, REPNOTIFY_OnChanged);
}


UAbilitySystemComponent* AEMRPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}


void AEMRPlayerState::InitializeAbilitySystemForPawn(APawn* InPawn)
{
    if (!HasAuthority() || !InPawn || !AbilitySystemComponent)
    {
        return;
    }

    // Init ASC with PlayerState as Owner and Pawn as Avatar
    AbilitySystemComponent->InitAbilityActorInfo(this, InPawn);
    UpdateAbilityActorInfoMesh(AbilitySystemComponent, InPawn);

    // Mark as ready and replicate
    bAbilityActorInfoReady = true;
}


void AEMRPlayerState::BeginPlay()
{
    Super::BeginPlay();

	if (HasAuthority())
	{
		UpdateLobbyPlayerInfo();
	}

    // Client might receive PlayerState before Pawn, so we try init here too
    if (!HasAuthority())
    {
        TryInitAbilityActorInfo();
    }
}


void AEMRPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ClearActorInfo();
	}
}


void AEMRPlayerState::TryInitAbilityActorInfo()
{
    if (!AbilitySystemComponent ||  !bAbilityActorInfoReady)
    {
        return;
    }

    // Try to get the owning Pawn
    APawn* OwnerPawn = GetPawn();
    if (OwnerPawn && AbilitySystemComponent->GetAvatarActor() == nullptr)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, OwnerPawn);
        UpdateAbilityActorInfoMesh(AbilitySystemComponent, OwnerPawn);
    }
}


void AEMRPlayerState::OnRep_AbilityActorInfoReady()
{
    // When server marks ASC as ready, client can init its side
    if (bAbilityActorInfoReady)
    {
        TryInitAbilityActorInfo();
    }

	if (AbilitySystemComponent)
	{
		const TArray<FGameplayAbilitySpec>& GrantedAbilities = AbilitySystemComponent->GetActivatableAbilities();
		for (int32 i = 0; i < GrantedAbilities.Num(); ++i)
		{
			const FGameplayAbilitySpec& Spec = GrantedAbilities[i];
			if (Spec.Ability)
			{
				UE_LOG(LogTemp, Log, TEXT("[XRayFlow] AbilityList ClientReady [%d] %s (Handle=%ls Active=%s)"),
					i,
					*Spec.Ability->GetName(),
					*Spec.Handle.ToString(),
					Spec.IsActive() ? TEXT("YES") : TEXT("NO"));
			}
		}
	}
}


void AEMRPlayerState::SetLobbyReady(bool bReady)
{
	if (!HasAuthority())
	{
		Server_SetLobbyReady(bReady);
		return;
	}

	if (LobbyPlayerInfo.bIsPlayerReady == bReady)
	{
		return;
	}

	LobbyPlayerInfo.bIsPlayerReady = bReady;
	UpdateLobbyPlayerInfo();
	NotifyLobbyPlayerInfoChanged();
}

void AEMRPlayerState::SetLobbyCharacterIndex(int32 NewIndex)
{
	if (!HasAuthority())
	{
		Server_SetLobbyCharacterIndex(NewIndex);
		return;
	}

	if (LobbyPlayerInfo.bIsPlayerReady)
	{
		return;
	}

	int32 ResolvedIndex = NewIndex;
	if (const AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr)
	{
		const UEMRLobbyCharacterCatalog* Catalog = LobbyGameState->GetLobbyCharacterCatalog();
		if (Catalog && Catalog->Characters.Num() > 0)
		{
			if (!Catalog->Characters.IsValidIndex(NewIndex))
			{
				ResolvedIndex = 0;
			}
		}
	}

	if (LobbyPlayerInfo.SelectedCharacterIndex == ResolvedIndex)
	{
		return;
	}

	LobbyPlayerInfo.SelectedCharacterIndex = ResolvedIndex;
	UpdateLobbyPlayerInfo();
	NotifyLobbyPlayerInfoChanged();
}


void AEMRPlayerState::Server_SetLobbyReady_Implementation(bool bReady)
{
	SetLobbyReady(bReady);
}

void AEMRPlayerState::Server_SetLobbyCharacterIndex_Implementation(int32 NewIndex)
{
	SetLobbyCharacterIndex(NewIndex);
}


void AEMRPlayerState::UpdateLobbyPlayerInfo()
{
	if (!HasAuthority())
	{
		Server_UpdateLobbyPlayerInfo();
		return;
	}
	
	LobbyPlayerInfo.PlayerName = FText::FromString(GetPlayerName());
	
	OnRep_LobbyPlayerInfo();
}


void AEMRPlayerState::OnRep_LobbyPlayerInfo()
{
	NotifyLobbyPlayerInfoChanged();
}


void AEMRPlayerState::Server_UpdateLobbyPlayerInfo_Implementation()
{
	UpdateLobbyPlayerInfo();
}


void AEMRPlayerState::NotifyLobbyPlayerInfoChanged()
{
	OnLobbyPlayerInfoChanged.Broadcast(LobbyPlayerInfo);
}
