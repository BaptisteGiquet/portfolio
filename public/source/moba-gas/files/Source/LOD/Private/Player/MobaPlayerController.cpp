
#include "Player/MobaPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GenericTeamAgentInterface.h"
#include "MobaPlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/MobaGameplayWidget.h"



void AMobaPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	PlayerCharacter = Cast<AMobaPlayerCharacter>(InPawn);
	if (PlayerCharacter)
	{
		PlayerCharacter->InitializeAbilitySystemComponent_Server();
		PlayerCharacter->SetGenericTeamId(TeamID);
	}
}




void AMobaPlayerController::AcknowledgePossession(class APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);
	
	PlayerCharacter = Cast<AMobaPlayerCharacter>(InPawn);
	if (PlayerCharacter)
	{
		PlayerCharacter->InitializeAbilitySystemComponent_Client();
		SpawnGameplayWidget();
	}
}




void AMobaPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	IGenericTeamAgentInterface::SetGenericTeamId(NewTeamID);
	TeamID = NewTeamID;
}




FGenericTeamId AMobaPlayerController::GetGenericTeamId() const
{
	return TeamID;
}


void AMobaPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, TeamID)
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, bHasWonMatch, COND_OwnerOnly, REPNOTIFY_Always)
}


void AMobaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UIInputMappingContext)
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		if (InputSubsystem)
		{
			constexpr int32 MappingPriority = 1;
			InputSubsystem->RemoveMappingContext(UIInputMappingContext);
			InputSubsystem->AddMappingContext(UIInputMappingContext, MappingPriority);	
		}
	}
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent) { return; }

	EnhancedInputComponent->BindAction(ToggleShopInputAction, ETriggerEvent::Started, this, &AMobaPlayerController::ToggleShopVisibility);
	EnhancedInputComponent->BindAction(ToggleGameplayMenuInputAction, ETriggerEvent::Started, this, &AMobaPlayerController::ToggleGameplayMenu);
}


void AMobaPlayerController::MatchFinished(AActor* ViewTarget, EMobaTeam WinningTeam)
{
	if (!HasAuthority()) { return; }
	
	if (PlayerCharacter)
	{
		PlayerCharacter->DisableInput(this);	
	}

	SetViewTargetWithBlend(ViewTarget, MatchFinishedBlendTimeDuration);

	const EMobaTeam PlayerTeam = static_cast<EMobaTeam>(GetGenericTeamId().GetId());
	if (PlayerTeam == WinningTeam)
	{
		bHasWonMatch = true;
	}
	else
	{
		bHasWonMatch = false;
	}

	// To make it work on the ListenServer;
	FTimerHandle DelayHandle;
	GetWorldTimerManager().SetTimer(DelayHandle, this, &ThisClass::OnRep_bHasWonMatch, 0.1f, false);
}


void AMobaPlayerController::SpawnGameplayWidget()
{
	if (!GameplayWidgetClass || !IsLocalPlayerController())
	{
		return;
	}
	
	GameplayWidget = CreateWidget<UMobaGameplayWidget>(this, GameplayWidgetClass);
	if (GameplayWidget)
	{
		GameplayWidget->AddToViewport();	
	}
}


void AMobaPlayerController::ShowWinLoseState()
{
	if (GameplayWidget)
	{
		GameplayWidget->ShowGameplayMenu();
	}
}


void AMobaPlayerController::ToggleShopVisibility()
{
	if (GameplayWidget)
	{
		GameplayWidget->ToggleShopVisibility();
	}
}


void AMobaPlayerController::ToggleGameplayMenu()
{
	if (GameplayWidget)
	{
		GameplayWidget->ToggleGameplayMenu();
	}
}


void AMobaPlayerController::OnRep_bHasWonMatch()
{
	UE_LOG(LogTemp, Warning, TEXT("OnRep_bHasWonMatch"))
	FString WinLoseMessage = TEXT("");
	WinLoseMessage = bHasWonMatch ? TEXT("You Win!") : TEXT("You Lose!");

	if (GameplayWidget)
	{
		GameplayWidget->SetGameplayMenuTitle(WinLoseMessage);	
	}

	FTimerHandle ShowWinLoseStateTimeHandle;
	GetWorldTimerManager().SetTimer(ShowWinLoseStateTimeHandle, this, &ThisClass::ShowWinLoseState, MatchFinishedBlendTimeDuration);
}
