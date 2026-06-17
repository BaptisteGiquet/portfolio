#include "Characters/Player/EMRPlayerController.h"

#include "AbilitySystemComponent.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "Abilities/GameplayAbility.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Characters/Player/EMRPlayerState.h"
#include "Data/EMREconomySystemGenerics.h"
#include "GAS/EMRTags.h"
#include "Characters/Patient/EMRPatient.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Framework/EMRAssetManager.h"
#include "Subsystems/EMRExamQueueSubsystem.h"
#include "Subsystems/EMRGameplayTelemetrySubsystem.h"
#include "Subsystems/EMRLobbyInviteCodeSubsystem.h"
#include "Subsystems/EMRLobbySessionSubsystem.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "Subsystems/EMRRunProgressionSubsystem.h"
#include "Subsystems/EMRNightShiftSpawnSubsystem.h"
#include "Subsystems/EMRTreatmentSubsystem.h"
#include "Framework/EMRHubGameMode.h"
#include "Framework/EMRNightShiftGameMode.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "UI/Hub/EMRCommonHubNightShiftSelectionWidget.h"
#include "UI/Dev/EMRDeveloperToolsWidget.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "UI/Frontend/EMRGameUserSettings.h"
#include "Interaction/EMRBaseMachine.h"
#include "Interaction/EMRReceptionMachine.h"
#include "UI/Frontend/Core/EMRCommonPrimaryLayoutWidget.h"
#include "UI/Frontend/Utils/DebugHelper.h"
#include "Subsystems/EMRLobbyCharacterSelectionSubsystem.h"
#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
    constexpr TCHAR PlayerControllerUpgradesFlowLogPrefix[] = TEXT("[UpgradesFlow]");

    FGameplayTag NormalizeReceptionExamTag(const FGameplayTag& ExamTag)
    {
        if (!ExamTag.IsValid())
        {
            return FGameplayTag::EmptyTag;
        }

        if (ExamTag.MatchesTag(EMRTags::Abilities::Exam::LabAnalyzer::Root))
        {
            return EMRTags::Abilities::Exam::LabAnalyzer::Root;
        }

        return ExamTag;
    }
}

AEMRPlayerController::AEMRPlayerController()
{
    GameMenuStackTag = EMRTags::UI::WidgetStack::GameMenu;
    GameHudStackTag = EMRTags::UI::WidgetStack::GameHud;
    DeveloperToolsStackTag = EMRTags::UI::WidgetStack::GameMenu;
    DeveloperToolsWidgetClass = UEMRDeveloperToolsWidget::StaticClass();
}


void AEMRPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	bIsTraveling = false;
	bIgnoreNightShiftWidgetPush = false;
    bPendingReturnToFrontendAfterTelemetry = false;

	AEMRPlayerState* PS = GetPlayerState<AEMRPlayerState>();
	PlayerCharacter = Cast<AEMRPlayerCharacter>(InPawn);
	if (PS && PlayerCharacter)
	{
		// Init ASC on server
		PS->InitializeAbilitySystemForPawn(PlayerCharacter);

		// Grant abilities
		PlayerCharacter->GrantStartupAbilities();
	}
}


void AEMRPlayerController::AcknowledgePossession(class APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);
	
	bIsTraveling = false;
	bIgnoreNightShiftWidgetPush = false;
    bPendingReturnToFrontendAfterTelemetry = false;

	PlayerCharacter = Cast<AEMRPlayerCharacter>(InPawn);
	AEMRPlayerState* PS = GetPlayerState<AEMRPlayerState>();
	if (PS)
	{
		// Client waits for bAbilityActorInfoReady to replicate
		// If already ready, TryInitAbilityActorInfo will run immediately

		PS->TryInitAbilityActorInfo();
	}

	BindToRunState();
}



void AEMRPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController())
    {
        bIsTraveling = false;
        bIgnoreNightShiftWidgetPush = false;
        bPendingReturnToFrontendAfterTelemetry = false;
		EnsureReticleWidget();

        if (UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(this))
        {
            if (TSubclassOf<UEMRCommonPrimaryLayoutWidget> PrimaryLayoutClass = ResolvePrimaryLayoutWidgetClass())
            {
                CachedPrimaryLayoutWidget = UISubsystem->EnsurePrimaryLayout(this, PrimaryLayoutClass);
            }
        }

        EnsureGameHudRootWidget();

        if (UEMRGameUserSettings* UserSettings = UEMRGameUserSettings::Get())
        {
            UserSettings->ApplyShowKeybindHelper();
        }

        if (UEMRLobbyCharacterSelectionSubsystem* SelectionSubsystem = GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UEMRLobbyCharacterSelectionSubsystem>() : nullptr)
        {
            SelectionSubsystem->ApplySavedSelectionToPlayerState();
        }

        if (AEMRNightShiftGameState* RunGS = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr)
        {
            PendingTravelDelegateHandle = RunGS->OnPendingTravelChanged().AddUObject(this, &ThisClass::HandlePendingTravelUpdated);
            NightShiftTelemetryPublishedHandle = RunGS->OnNightShiftTelemetryPublished().AddUObject(this, &ThisClass::HandleNightShiftTelemetryPublished);
            HandlePendingTravelUpdated();
        }
    }

    BindToRunState();
}

void AEMRPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent)
    {
        // Code-only fallback so dev tools work without any Blueprint/Enhanced Input assignment.
        InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &ThisClass::ToggleDeveloperTools);
    }
}


void AEMRPlayerController::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    if (IsLocalController())
    {
		EnsureReticleWidget();
        if (AEMRNightShiftGameState* RunGS = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr)
        {
            if (!PendingTravelDelegateHandle.IsValid())
            {
                PendingTravelDelegateHandle = RunGS->OnPendingTravelChanged().AddUObject(this, &ThisClass::HandlePendingTravelUpdated);
            }
            if (!NightShiftTelemetryPublishedHandle.IsValid())
            {
                NightShiftTelemetryPublishedHandle = RunGS->OnNightShiftTelemetryPublished().AddUObject(this, &ThisClass::HandleNightShiftTelemetryPublished);
            }
            HandlePendingTravelUpdated();
        }
    }

    BindToRunState();
}


void AEMRPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReturnToFrontendTelemetryTimeoutHandle);
    }

    bIsTraveling = true;
    bIgnoreNightShiftWidgetPush = true;
    bPendingReturnToFrontendAfterTelemetry = false;
    HideDeveloperTools();
    CleanupHubUI();

    Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);
}


void AEMRPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReturnToFrontendTelemetryTimeoutHandle);
    }

    if (PendingTravelDelegateHandle.IsValid())
    {
        if (AEMRNightShiftGameState* RunGS = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr)
        {
            RunGS->OnPendingTravelChanged().Remove(PendingTravelDelegateHandle);
        }
        PendingTravelDelegateHandle.Reset();
    }

    if (NightShiftTelemetryPublishedHandle.IsValid())
    {
        if (AEMRNightShiftGameState* RunGS = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr)
        {
            RunGS->OnNightShiftTelemetryPublished().Remove(NightShiftTelemetryPublishedHandle);
        }
        NightShiftTelemetryPublishedHandle.Reset();
    }

    CleanupHubUI();
    HideDeveloperTools();
    UnbindFromRunState();

	if (ReticleWidget)
	{
		ReticleWidget->RemoveFromParent();
		ReticleWidget = nullptr;
	}

    Super::EndPlay(EndPlayReason);
}



bool AEMRPlayerController::SendGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (!EventTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] SendGameplayEvent - Invalid EventTag"));
		return false;
	}
	
	AEMRPlayerState* OwnerPlayerState = GetPlayerState<AEMRPlayerState>();
	if (!OwnerPlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] SendGameplayEvent - PlayerState not ready"));
		return false;
	}

	UAbilitySystemComponent* ASC = nullptr;
	if (IAbilitySystemInterface* PlayerStateASCI = Cast<IAbilitySystemInterface>(OwnerPlayerState))
	{
		ASC = PlayerStateASCI->GetAbilitySystemComponent();
	}
	
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] SendGameplayEvent - ASC not ready"));
		return false;
	}

	const TArray<FGameplayAbilitySpec>& ActivatableAbilities = ASC->GetActivatableAbilities();
	UE_LOG(LogTemp, Log, TEXT("[PlayerController] ASC contains %d abilities:"), ActivatableAbilities.Num());

	for (int32 i = 0; i < ActivatableAbilities.Num(); ++i)
	{
		const FGameplayAbilitySpec& Spec = ActivatableAbilities[i];
		if (Spec.Ability)
		{
			UE_LOG(LogTemp, Log, TEXT("  [%d] %s (Handle: %ls, Active: %s)"),
				i,
				*Spec.Ability->GetName(),
				*Spec.Handle.ToString(),
				Spec.IsActive() ? TEXT("YES") : TEXT("NO")
			);
		}
	}

	FGameplayTagContainer ActiveTags;
	ASC->GetOwnedGameplayTags(ActiveTags);
	UE_LOG(LogTemp, Log, TEXT("[PlayerController] ASC Active Tags: %s"), *ActiveTags.ToStringSimple());

	
	FGameplayEventData EventData = Payload;
	EventData.EventTag = EventTag;

	if (!IsValid(EventData.Instigator))
	{
		EventData.Instigator = GetPawn();
	}

	UE_LOG(LogTemp, Log, TEXT("[PlayerController] SendGameplayEvent - Tag: %s, Magnitude: %.0f"),
	*EventTag.ToString(), EventData.EventMagnitude);

	const int32 TriggeredAbilities = ASC->HandleGameplayEvent(EventTag, &EventData);

	UE_LOG(LogTemp, Log, TEXT("[PlayerController] SendGameplayEvent - Event %s triggered (%d abilities)"),
		*EventTag.ToString(), TriggeredAbilities);

	return TriggeredAbilities > 0;
}


bool AEMRPlayerController::SendGameplayEventToActor(AActor* TargetActor, FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (!EventTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] SendGameplayEventToActor - Invalid EventTag"));
		return false;
	}

	AActor* Target = TargetActor ? TargetActor : GetPawn();
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] SendGameplayEventToActor - No valid target"));
		return false;
	}

	UAbilitySystemComponent* TargetASC = nullptr;
	if (IAbilitySystemInterface* ASCI = Cast<IAbilitySystemInterface>(Target))
	{
		TargetASC = ASCI->GetAbilitySystemComponent();
	}

	if (!TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] SendGameplayEventToActor - Target %s has no ASC"), *Target->GetName());
		return false;
	}

	FGameplayEventData EventData = Payload;
	EventData.EventTag = EventTag;

	if (!IsValid(EventData.Instigator))
	{
		EventData.Instigator = GetPawn();
	}

	const int32 TriggeredAbilities = TargetASC->HandleGameplayEvent(EventTag, &EventData);
	UE_LOG(LogTemp, Warning, TEXT("[PlayerController] SendGameplayEventToActor - Event %s sent to %s (%d abilities triggered)"),
		*EventTag.ToString(), *Target->GetName(), TriggeredAbilities);

	return TriggeredAbilities > 0;
}


void AEMRPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}



void AEMRPlayerController::Server_CompleteReceptionTask_Implementation(AEMRPatient* Patient, const TArray<FGameplayTag>& Exams, bool bNoExamNeeded, bool bRegisterPatientFolder)
{
    if (!Patient)
    {
            UE_LOG(LogTemp, Error, TEXT("[PlayerController::Server_CompleteReceptionTask] Patient is NULL"));
            return;
    }

    if (!bNoExamNeeded && Exams.Num() == 0)
    {
            UE_LOG(LogTemp, Error, TEXT("[PlayerController::Server_CompleteReceptionTask] No exams provided"));
            return;
    }

    UEMRExamQueueSubsystem* ExamQueue = GetGameInstance()->GetSubsystem<UEMRExamQueueSubsystem>();
    if (!ExamQueue)
    {
            UE_LOG(LogTemp, Error, TEXT("[PlayerController::Server_CompleteReceptionTask] ExamQueueSubsystem is NULL"));
            return;
    }

    AEMRReceptionMachine* ReceptionMachine = nullptr;
    if (const UEMRNavigationIntentSubsystem* NavigationSubsystem = GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
    {
        AActor* ReferenceActor = GetPawn();
        if (!ReferenceActor)
        {
            ReferenceActor = this;
        }

        ReceptionMachine = Cast<AEMRReceptionMachine>(NavigationSubsystem->FindClosestMachine(ReferenceActor, EMRTags::Machine::Reception));
    }

    if (!DoesReceptionSelectionMatchRequiredExams(ExamQueue, Patient, Exams, bNoExamNeeded))
    {
        if (ReceptionMachine)
        {
            ReceptionMachine->PlayValidationCueForNearbyPlayers(false, GetPawn());
        }
        Client_NotifyReceptionValidationResult(Patient, EEMRReceptionValidationResult::WrongExamSelection);
        return;
    }

    if (!bNoExamNeeded && ExamQueue->AreAllExamWaitingSeatsFull())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[PlayerController::Server_CompleteReceptionTask] Rejecting exam assignment because all exam waiting seats are full. patient=%s"),
            *GetNameSafe(Patient));

        if (ReceptionMachine)
        {
            ReceptionMachine->PlayValidationCueForNearbyPlayers(false, GetPawn());
        }
        Client_NotifyReceptionValidationResult(Patient, EEMRReceptionValidationResult::NoExamWaitingSeatAvailable);
        return;
    }

    if (bNoExamNeeded)
    {
        if (UEMRTreatmentSubsystem* TreatmentSubsystem = GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>())
        {
            if (!TreatmentSubsystem->HasAvailableBed() && TreatmentSubsystem->AreAllTreatmentWaitingSeatsFull())
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("[PlayerController::Server_CompleteReceptionTask] Rejecting no-exam assignment because treatment area is full. patient=%s"),
                    *GetNameSafe(Patient));

                if (ReceptionMachine)
                {
                    ReceptionMachine->PlayValidationCueForNearbyPlayers(false, GetPawn());
                }
                Client_NotifyReceptionValidationResult(Patient, EEMRReceptionValidationResult::NoExamWaitingSeatAvailable);
                return;
            }
        }
    }

    Patient->SetFolderDisplayRegistered(bRegisterPatientFolder);

    if (bNoExamNeeded)
    {
            ExamQueue->BypassExamQueueToTreatment(Patient);
    }
    else
    {
            ExamQueue->AddPatientToExamQueue(Patient, Exams);
    }

    if (ReceptionMachine)
    {
        ReceptionMachine->ClearOccupiedPatient(Patient);
        ReceptionMachine->PlayValidationCueForNearbyPlayers(true, GetPawn());
    }

    Client_NotifyReceptionValidationResult(Patient, EEMRReceptionValidationResult::Success);
}

void AEMRPlayerController::Client_NotifyReceptionValidationResult_Implementation(AEMRPatient* Patient, EEMRReceptionValidationResult Result)
{
    OnReceptionValidationResult.Broadcast(Patient, Result);
}

void AEMRPlayerController::Client_ReceiveNightShiftTelemetry_Implementation(const FEMRNightShiftTelemetryRecord& Record)
{
    HandleNightShiftTelemetryPublished(Record);
}

void AEMRPlayerController::Server_RequestNightShiftTelemetryOnLeave_Implementation()
{
    bool bTelemetrySent = false;
    if (UWorld* World = GetWorld())
    {
        if (AEMRNightShiftGameMode* NightShiftGameMode = World->GetAuthGameMode<AEMRNightShiftGameMode>())
        {
            bTelemetrySent = NightShiftGameMode->SendNightShiftTelemetrySnapshotToController(this);
        }
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[Telemetry][PlayerController] Leave request telemetry snapshot sent=%s player=%s"),
        bTelemetrySent ? TEXT("true") : TEXT("false"),
        *GetNameSafe(this));

    Client_ConfirmReturnToFrontendAfterTelemetry();
}

void AEMRPlayerController::Client_ConfirmReturnToFrontendAfterTelemetry_Implementation()
{
    if (!IsLocalController())
    {
        return;
    }

    if (!bPendingReturnToFrontendAfterTelemetry)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReturnToFrontendTelemetryTimeoutHandle);
    }

    bPendingReturnToFrontendAfterTelemetry = false;
    ContinueReturnToFrontendAfterTelemetry();
}

bool AEMRPlayerController::DoesReceptionSelectionMatchRequiredExams(UEMRExamQueueSubsystem* ExamQueue, AEMRPatient* Patient, const TArray<FGameplayTag>& Exams, bool bNoExamNeeded) const
{
    if (!ExamQueue || !Patient)
    {
        return false;
    }

    const FGameplayTagContainer RequiredExamTags = ExamQueue->GetMissingRequiredExamsForPatient(Patient);
    TSet<FGameplayTag> RequiredSelectionSet;
    for (const FGameplayTag& RequiredExamTag : RequiredExamTags)
    {
        const FGameplayTag NormalizedExamTag = NormalizeReceptionExamTag(RequiredExamTag);
        if (NormalizedExamTag.IsValid() && !NormalizedExamTag.MatchesTagExact(EMRTags::Abilities::Exam::None))
        {
            RequiredSelectionSet.Add(NormalizedExamTag);
        }
    }

    if (bNoExamNeeded)
    {
        return RequiredSelectionSet.IsEmpty();
    }

    TSet<FGameplayTag> SelectedSelectionSet;
    for (const FGameplayTag& SelectedExamTag : Exams)
    {
        const FGameplayTag NormalizedExamTag = NormalizeReceptionExamTag(SelectedExamTag);
        if (!NormalizedExamTag.IsValid() || NormalizedExamTag.MatchesTagExact(EMRTags::Abilities::Exam::None))
        {
            return false;
        }

        SelectedSelectionSet.Add(NormalizedExamTag);
    }

    if (RequiredSelectionSet.Num() != SelectedSelectionSet.Num())
    {
        return false;
    }

    for (const FGameplayTag& RequiredExamTag : RequiredSelectionSet)
    {
        if (!SelectedSelectionSet.Contains(RequiredExamTag))
        {
            return false;
        }
    }

    return true;
}

void AEMRPlayerController::Server_CallWaitingPatient_Implementation(AEMRPatient* Patient)
{
    if (!Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] CallWaitingPatient aborted: null patient."));
        return;
    }

    FGameplayEventData Payload;
    Payload.EventTag = EMRTags::Abilities::CallWaitingPatient;
    Payload.Instigator = GetPawn();
    Payload.Target = Patient;
    Payload.OptionalObject = Patient;

    const bool bTriggered = SendGameplayEvent(EMRTags::Abilities::CallWaitingPatient, Payload);
    if (!bTriggered)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Failed to trigger CallWaitingPatient gameplay event."));
    }
}


void AEMRPlayerController::Server_SelectNightShiftByIndex_Implementation(int32 OfferIndex)
{
    if (OfferIndex < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] NightShift selection rejected: invalid index."));
        return;
    }

    FGameplayEventData Payload;
    Payload.Instigator = GetPawn();
    Payload.EventMagnitude = OfferIndex;
    Payload.EventTag = EMRTags::Abilities::SelectNightShift;

    const bool bTriggered = SendGameplayEvent(EMRTags::Abilities::SelectNightShift, Payload);
    if (!bTriggered)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] NightShift selection event not handled by GAS."));
    }
}

void AEMRPlayerController::Server_VoteForHubUpgradeByIndex_Implementation(int32 OfferIndex)
{
    if (OfferIndex < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Server_VoteForHubUpgradeByIndex rejected invalid offer index=%d."), PlayerControllerUpgradesFlowLogPrefix, OfferIndex);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Server_VoteForHubUpgradeByIndex aborted: world null."), PlayerControllerUpgradesFlowLogPrefix);
        return;
    }

    AEMRHubGameMode* HubGameMode = World->GetAuthGameMode<AEMRHubGameMode>();
    if (!HubGameMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Server_VoteForHubUpgradeByIndex aborted: HubGameMode not found."), PlayerControllerUpgradesFlowLogPrefix);
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s Server_VoteForHubUpgradeByIndex forwarding vote playerState=%s offerIndex=%d"),
        PlayerControllerUpgradesFlowLogPrefix,
        *GetNameSafe(PlayerState),
        OfferIndex);
    HubGameMode->CastOrUpdateUpgradeVote(PlayerState, OfferIndex);
}

void AEMRPlayerController::Server_RequestStartHubUpgradeVoteNow_Implementation()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Server_RequestStartHubUpgradeVoteNow aborted: world null."), PlayerControllerUpgradesFlowLogPrefix);
        return;
    }

    AEMRHubGameMode* HubGameMode = World->GetAuthGameMode<AEMRHubGameMode>();
    if (!HubGameMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Server_RequestStartHubUpgradeVoteNow aborted: HubGameMode not found."), PlayerControllerUpgradesFlowLogPrefix);
        return;
    }

    HubGameMode->RequestStartHubUpgradeVoteNow(PlayerState);
}


void AEMRPlayerController::SpawnHUDWidgets()
{
    
    if (!IsLocalController() || bIsTraveling || bIgnoreNightShiftWidgetPush)
    {
        return;
    }

    const AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get();
    if (!RunGS && GetWorld())
    {
        RunGS = GetWorld()->GetGameState<AEMRNightShiftGameState>();
    }

    if (!(RunGS && RunGS->GetRunPhase() == EER_RunPhase::Hub))
    {
        return;
    }

    const TSubclassOf<UEMRCommonPrimaryLayoutWidget> PrimaryLayoutClass = ResolvePrimaryLayoutWidgetClass();
    if (!PrimaryLayoutClass)
    {
        return;
    }

    if (UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        CachedPrimaryLayoutWidget = UISubsystem->EnsurePrimaryLayout(this, PrimaryLayoutClass);
    }
}


bool AEMRPlayerController::IsInHubPhase() const
{
    const UWorld* World = GetWorld();
	AEMRNightShiftGameState* RunGS = World ? World->GetGameState<AEMRNightShiftGameState>() : nullptr;
    if (RunGS && CachedNightShiftGameState.Get() != RunGS)
    {
        CachedNightShiftGameState = RunGS;
    }
    return RunGS && RunGS->GetRunPhase() == EER_RunPhase::Hub;
}


void AEMRPlayerController::EnsureReticleWidget()
{
	if (!IsLocalController() || !ReticleWidgetClass)
	{
		return;
	}

	if (!IsValid(ReticleWidget))
	{
		ReticleWidget = nullptr;
	}

	if (ReticleWidget)
	{
		return;
	}

	ReticleWidget = CreateWidget<UUserWidget>(this, ReticleWidgetClass);
	if (!ReticleWidget)
	{
		return;
	}

	ReticleWidget->AddToViewport(ReticleZOrder);
}

void AEMRPlayerController::RequestReturnToFrontendWithNightShiftTelemetry()
{
    if (!IsLocalController())
    {
        return;
    }

    if (bPendingReturnToFrontendAfterTelemetry)
    {
        return;
    }

    const AEMRNightShiftGameState* RunGS = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
    const bool bShouldRequestTelemetry = RunGS && RunGS->GetRunPhase() == EER_RunPhase::InNightShift;
    if (bShouldRequestTelemetry)
    {
        bPendingReturnToFrontendAfterTelemetry = true;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                ReturnToFrontendTelemetryTimeoutHandle,
                this,
                &ThisClass::HandleReturnToFrontendTelemetryTimeout,
                1.0f,
                false);
        }
        Server_RequestNightShiftTelemetryOnLeave();
        return;
    }

    ContinueReturnToFrontendAfterTelemetry();
}

void AEMRPlayerController::RequestRegenerateSessionInviteCode()
{
    if (!IsLocalController())
    {
        return;
    }

    Server_RegenerateSessionInviteCode();
}

void AEMRPlayerController::Server_RegenerateSessionInviteCode_Implementation()
{
    if (!HasAuthority() || !GetLocalPlayer())
    {
        return;
    }

    UEMRLobbyInviteCodeSubsystem* InviteCodeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbyInviteCodeSubsystem>() : nullptr;
    if (!InviteCodeSubsystem)
    {
        return;
    }

    const FString NewCode = InviteCodeSubsystem->GenerateInviteCode();
    if (NewCode.IsEmpty() || !InviteCodeSubsystem->UpdateSessionInviteCode(NewCode))
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Failed to regenerate session invite code."));
        return;
    }

    if (AEMRNightShiftGameState* RunGameState = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr)
    {
        RunGameState->SetSessionInviteCode(NewCode);

        if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRRunProgressionSubsystem>() : nullptr)
        {
            ProgressionSubsystem->CacheFromGameState(RunGameState);
        }
    }
}

void AEMRPlayerController::Server_RequestDeveloperToolAction_Implementation(const FEMRDeveloperToolActionRequest& Request)
{
    FEMRDeveloperToolActionResult Result;
    Result.Action = Request.Action;

    ExecuteDeveloperToolAction_Server(Request, Result);
    Client_ReceiveDeveloperToolActionResult(Result);
}

void AEMRPlayerController::Server_RequestDeveloperToolUpgradeOptions_Implementation()
{
    TArray<FEMRDeveloperToolUpgradeOption> Options;
    FString ErrorMessage;

    if (!CollectDeveloperUpgradeOptions_Server(Options, ErrorMessage))
    {
        FEMRDeveloperToolActionResult Result;
        Result.Action = EEMRDeveloperToolAction::GiveSpecificUpgrade;
        Result.bSuccess = false;
        Result.Message = ErrorMessage;
        Client_ReceiveDeveloperToolUpgradeOptions(Options);
        Client_ReceiveDeveloperToolActionResult(Result);
        return;
    }

    Client_ReceiveDeveloperToolUpgradeOptions(Options);

    FEMRDeveloperToolActionResult Result;
    Result.Action = EEMRDeveloperToolAction::GiveSpecificUpgrade;
    Result.bSuccess = true;
    Result.Message = FString::Printf(TEXT("Upgrade list refreshed (%d option(s))."), Options.Num());
    Client_ReceiveDeveloperToolActionResult(Result);
}

void AEMRPlayerController::Client_ReceiveDeveloperToolActionResult_Implementation(const FEMRDeveloperToolActionResult& Result)
{
    NotifyDeveloperToolResult_Local(Result);
}

void AEMRPlayerController::Client_ReceiveDeveloperToolUpgradeOptions_Implementation(const TArray<FEMRDeveloperToolUpgradeOption>& Options)
{
    if (UEMRDeveloperToolsWidget* DevToolsWidget = GetActiveDeveloperToolsWidget())
    {
        DevToolsWidget->ApplyUpgradeOptions(Options);
    }
}

bool AEMRPlayerController::ExecuteDeveloperToolAction_Server(const FEMRDeveloperToolActionRequest& Request, FEMRDeveloperToolActionResult& OutResult)
{
    OutResult.Action = Request.Action;
    OutResult.bSuccess = false;
    OutResult.Message = TEXT("Unknown developer-tool action.");

    if (!HasAuthority())
    {
        OutResult.Message = TEXT("Action rejected: not authority.");
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        OutResult.Message = TEXT("Action failed: world is null.");
        return false;
    }

    AEMRNightShiftGameState* RunGS = World->GetGameState<AEMRNightShiftGameState>();
    if (RunGS && !RunGS->GetPendingTravelURL().IsEmpty())
    {
        OutResult.Message = TEXT("Action unavailable: travel is pending.");
        return false;
    }

    switch (Request.Action)
    {
    case EEMRDeveloperToolAction::SpawnPatient:
        {
            if (!RunGS || RunGS->GetRunPhase() != EER_RunPhase::InNightShift)
            {
                OutResult.Message = TEXT("Spawn patient is only available during Nightshift.");
                return false;
            }

            UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>();
            if (!SpawnSubsystem)
            {
                OutResult.Message = TEXT("Spawn patient failed: Nightshift spawn subsystem unavailable.");
                return false;
            }

            SpawnSubsystem->RequestImmediateSpawn();
            OutResult.bSuccess = true;
            OutResult.Message = TEXT("Requested immediate patient spawn.");
            return true;
        }

    case EEMRDeveloperToolAction::WinNightShift:
        {
            if (!RunGS || RunGS->GetRunPhase() != EER_RunPhase::InNightShift)
            {
                OutResult.Message = TEXT("Win Nightshift is only available during Nightshift.");
                return false;
            }

            AEMRNightShiftGameMode* NightShiftGameMode = World->GetAuthGameMode<AEMRNightShiftGameMode>();
            if (!NightShiftGameMode)
            {
                OutResult.Message = TEXT("Win Nightshift failed: Nightshift GameMode unavailable.");
                return false;
            }

            NightShiftGameMode->ForceNightShiftSuccessForTests();
            OutResult.bSuccess = true;
            OutResult.Message = TEXT("Forced Nightshift success and requested normal finish flow.");
            return true;
        }

    case EEMRDeveloperToolAction::LoseNightShift:
        {
            if (!RunGS || RunGS->GetRunPhase() != EER_RunPhase::InNightShift)
            {
                OutResult.Message = TEXT("Lose Nightshift is only available during Nightshift.");
                return false;
            }

            AEMRNightShiftGameMode* NightShiftGameMode = World->GetAuthGameMode<AEMRNightShiftGameMode>();
            if (!NightShiftGameMode)
            {
                OutResult.Message = TEXT("Lose Nightshift failed: Nightshift GameMode unavailable.");
                return false;
            }

            NightShiftGameMode->ForceNightShiftFailureByReputationForTests();
            OutResult.bSuccess = true;
            OutResult.Message = TEXT("Forced Nightshift failure by reputation.");
            return true;
        }

    case EEMRDeveloperToolAction::EarnRevenue1000:
        {
            FString Error;
            const bool bApplied = ApplyDeveloperEconomyDelta_Server(1000.0f, 0.0f, Error);
            OutResult.bSuccess = bApplied;
            OutResult.Message = bApplied ? TEXT("Granted 1000 TotalRevenue.") : Error;
            return bApplied;
        }

    case EEMRDeveloperToolAction::EarnReputation10:
        {
            FString Error;
            const bool bApplied = ApplyDeveloperEconomyDelta_Server(0.0f, 10.0f, Error);
            OutResult.bSuccess = bApplied;
            OutResult.Message = bApplied ? TEXT("Granted 10 Reputation.") : Error;
            return bApplied;
        }

    case EEMRDeveloperToolAction::LoseReputation10:
        {
            FString Error;
            const bool bApplied = ApplyDeveloperEconomyDelta_Server(0.0f, -10.0f, Error);
            OutResult.bSuccess = bApplied;
            OutResult.Message = bApplied ? TEXT("Removed 10 Reputation.") : Error;
            return bApplied;
        }

    case EEMRDeveloperToolAction::SpeedUpGameX2:
        {
            FString Message;
            const bool bApplied = ApplyDeveloperTimeDilationScale_Server(2.0f, Message);
            OutResult.bSuccess = bApplied;
            OutResult.Message = Message;
            return bApplied;
        }

    case EEMRDeveloperToolAction::SlowDownGameX2:
        {
            FString Message;
            const bool bApplied = ApplyDeveloperTimeDilationScale_Server(0.5f, Message);
            OutResult.bSuccess = bApplied;
            OutResult.Message = Message;
            return bApplied;
        }

    case EEMRDeveloperToolAction::RerollHubUpgrades:
        {
            AEMRHubGameMode* HubGameMode = World->GetAuthGameMode<AEMRHubGameMode>();
            if (!HubGameMode)
            {
                OutResult.Message = TEXT("Reroll upgrades is only available in Hub.");
                return false;
            }

            FString Message;
            const bool bSuccess = HubGameMode->RerollActiveHubUpgradeVoteOffersForTests(Message);
            OutResult.bSuccess = bSuccess;
            OutResult.Message = Message;
            return bSuccess;
        }

    case EEMRDeveloperToolAction::GiveSpecificUpgrade:
        {
            if (!Request.UpgradeTag.IsValid())
            {
                OutResult.Message = TEXT("Give specific upgrade failed: no upgrade selected.");
                return false;
            }

            AEMRHubGameMode* HubGameMode = World->GetAuthGameMode<AEMRHubGameMode>();
            if (!HubGameMode)
            {
                OutResult.Message = TEXT("Give specific upgrade is only available in Hub (v1).");
                return false;
            }

            FString Message;
            const bool bSuccess = HubGameMode->ApplySpecificRunUpgradeForTests(Request.UpgradeTag, Message);
            OutResult.bSuccess = bSuccess;
            OutResult.Message = Message;
            return bSuccess;
        }

    default:
        break;
    }

    return false;
}

bool AEMRPlayerController::ApplyDeveloperEconomyDelta_Server(const float RevenueDelta, const float ReputationDelta, FString& OutError) const
{
    UWorld* World = GetWorld();
    AEMRNightShiftGameState* RunGS = World ? World->GetGameState<AEMRNightShiftGameState>() : nullptr;
    UAbilitySystemComponent* TeamASC = RunGS ? RunGS->GetAbilitySystemComponent() : nullptr;
    if (!TeamASC)
    {
        OutError = TEXT("Economy action failed: Team ASC unavailable.");
        return false;
    }

    TArray<const UEMREconomySystemGenerics*> LoadedEconomy;
    if (!UEMRAssetManager::Get().CollectLoadedEconomySystemGenerics(LoadedEconomy) || LoadedEconomy.IsEmpty())
    {
        OutError = TEXT("Economy action failed: EconomySystemGenerics not loaded.");
        return false;
    }

    const UEMREconomySystemGenerics* EconomyConfig = LoadedEconomy[0];
    if (!EconomyConfig)
    {
        OutError = TEXT("Economy action failed: Economy config is null.");
        return false;
    }

    auto ApplyEffectWithMagnitude = [TeamASC](const TSubclassOf<UGameplayEffect> EffectClass, const FGameplayTag& SetByCallerTag, const float Magnitude) -> bool
    {
        if (!EffectClass)
        {
            return false;
        }

        FGameplayEffectContextHandle EffectContext = TeamASC->MakeEffectContext();
        EffectContext.AddSourceObject(TeamASC->GetOwner());

        FGameplayEffectSpecHandle SpecHandle = TeamASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
        if (!SpecHandle.IsValid())
        {
            return false;
        }

        SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, Magnitude);
        TeamASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        return true;
    };

    if (RevenueDelta > 0.0f)
    {
        if (!ApplyEffectWithMagnitude(EconomyConfig->GetGrantMoneyEffect(), EMRTags::SetByCaller::GrantMoney, RevenueDelta))
        {
            OutError = TEXT("Economy action failed: GrantMoney effect unavailable or failed.");
            return false;
        }
    }

    if (ReputationDelta > 0.0f)
    {
        if (!ApplyEffectWithMagnitude(EconomyConfig->GetGrantReputationEffect(), EMRTags::SetByCaller::GrantReputation, ReputationDelta))
        {
            OutError = TEXT("Economy action failed: GrantReputation effect unavailable or failed.");
            return false;
        }
    }
    else if (ReputationDelta < 0.0f)
    {
        if (!ApplyEffectWithMagnitude(EconomyConfig->GetRemoveReputationEffect(), EMRTags::SetByCaller::RemoveReputation, ReputationDelta))
        {
            OutError = TEXT("Economy action failed: RemoveReputation effect unavailable or failed.");
            return false;
        }
    }

    return true;
}

bool AEMRPlayerController::ApplyDeveloperTimeDilationScale_Server(const float Multiplier, FString& OutMessage) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        OutMessage = TEXT("Time-dilation action failed: world unavailable.");
        return false;
    }

    if (!FMath::IsFinite(Multiplier) || Multiplier <= 0.0f)
    {
        OutMessage = TEXT("Time-dilation action failed: invalid multiplier.");
        return false;
    }

    const float Current = UGameplayStatics::GetGlobalTimeDilation(World);
    const float NewTimeDilation = Current * Multiplier;
    UGameplayStatics::SetGlobalTimeDilation(World, NewTimeDilation);

    OutMessage = FString::Printf(TEXT("Global time dilation: %.3f -> %.3f"), Current, NewTimeDilation);
    return true;
}

bool AEMRPlayerController::CollectDeveloperUpgradeOptions_Server(TArray<FEMRDeveloperToolUpgradeOption>& OutOptions, FString& OutError) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        OutError = TEXT("Upgrade list unavailable: world is null.");
        return false;
    }

    AEMRHubGameMode* HubGameMode = World->GetAuthGameMode<AEMRHubGameMode>();
    if (!HubGameMode)
    {
        OutError = TEXT("Upgrade list is only available in Hub (v1).");
        return false;
    }

    return HubGameMode->CollectRunUpgradeOptionsForTests(OutOptions, OutError);
}

void AEMRPlayerController::ContinueReturnToFrontendAfterTelemetry()
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    if (UEMRLobbySessionSubsystem* LobbySessionSubsystem = GameInstance->GetSubsystem<UEMRLobbySessionSubsystem>())
    {
        LobbySessionSubsystem->ReturnToFrontend(this);
    }
}

void AEMRPlayerController::HandleReturnToFrontendTelemetryTimeout()
{
    if (!IsLocalController() || !bPendingReturnToFrontendAfterTelemetry)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Telemetry][PlayerController] Leave telemetry request timed out, proceeding with ReturnToFrontend."));
    bPendingReturnToFrontendAfterTelemetry = false;
    ContinueReturnToFrontendAfterTelemetry();
}

void AEMRPlayerController::ToggleDeveloperTools()
{
    if (!IsLocalController())
    {
        return;
    }

    if (GetActiveDeveloperToolsWidget())
    {
        HideDeveloperTools();
        return;
    }

    ShowDeveloperTools();
}

void AEMRPlayerController::ShowDeveloperTools()
{
    if (!IsLocalController())
    {
        return;
    }

    if (GetActiveDeveloperToolsWidget())
    {
        return;
    }

    if (!DeveloperToolsStackTag.IsValid() || DeveloperToolsWidgetClass.IsNull())
    {
        Debug::Print(TEXT("[DevTools] Assign a DevTools widget class and stack tag on the player controller defaults."), -1, FColor::Red);
        return;
    }

    if (UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        if (TSubclassOf<UEMRCommonPrimaryLayoutWidget> PrimaryLayoutClass = ResolvePrimaryLayoutWidgetClass())
        {
            CachedPrimaryLayoutWidget = UISubsystem->EnsurePrimaryLayout(this, PrimaryLayoutClass);
        }

        const int32 LocalPushId = ++DeveloperToolsPushId;
        UISubsystem->PushSoftWidgetToStackAsync(
            DeveloperToolsStackTag,
            DeveloperToolsWidgetClass,
            [this, LocalPushId](EAsyncPushWidgetState InPushState, UEMRFrontendCommonActivatableWidgetBase* PushedWidget)
            {
                if (InPushState != EAsyncPushWidgetState::AfterPush)
                {
                    return;
                }

                if (DeveloperToolsPushId != LocalPushId)
                {
                    return;
                }

                UEMRDeveloperToolsWidget* TypedWidget = Cast<UEMRDeveloperToolsWidget>(PushedWidget);
                ActiveDeveloperToolsWidget = TypedWidget;
                if (TypedWidget)
                {
                    TypedWidget->InitializeForController(this);
                }
                else
                {
                    Debug::Print(TEXT("[DevTools] Pushed widget must derive from UEMRDeveloperToolsWidget."), -1, FColor::Red);
                }
            });
    }
}

void AEMRPlayerController::HideDeveloperTools()
{
    ++DeveloperToolsPushId;

    UEMRDeveloperToolsWidget* ActiveWidget = GetActiveDeveloperToolsWidget();
    if (!ActiveWidget)
    {
        ActiveDeveloperToolsWidget = nullptr;
        return;
    }

    if (UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        UISubsystem->RemoveWidgetFromStack(DeveloperToolsStackTag, ActiveWidget);
    }

    if (ActiveDeveloperToolsWidget == ActiveWidget)
    {
        ActiveDeveloperToolsWidget = nullptr;
    }
}

UEMRDeveloperToolsWidget* AEMRPlayerController::GetActiveDeveloperToolsWidget() const
{
    return IsValid(ActiveDeveloperToolsWidget) ? ActiveDeveloperToolsWidget.Get() : nullptr;
}

void AEMRPlayerController::NotifyDeveloperToolResult_Local(const FEMRDeveloperToolActionResult& Result)
{
    if (UEMRDeveloperToolsWidget* DevToolsWidget = GetActiveDeveloperToolsWidget())
    {
        DevToolsWidget->ApplyActionResult(Result);
    }

    Debug::Print(
        FString::Printf(TEXT("[DevTools] %s"), *Result.Message),
        -1,
        Result.bSuccess ? FColor::Green : FColor::Red);
}



void AEMRPlayerController::ToggleGameplayMenu()
{
    if (!IsLocalController())
    {
        return;
    }

    if (!GameMenuStackTag.IsValid() || GameMenuWidgetClass.IsNull())
    {
        return;
    }

    if (UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        if (TSubclassOf<UEMRCommonPrimaryLayoutWidget> PrimaryLayoutClass = ResolvePrimaryLayoutWidgetClass())
        {
            CachedPrimaryLayoutWidget = UISubsystem->EnsurePrimaryLayout(this, PrimaryLayoutClass);
        }
    }

    EnsureGameHudRootWidget();

    if (UEMRFrontendCommonActivatableWidgetBase* ActiveWidget = GetActiveGameMenuWidget())
    {
        HideGameplayMenu();
        return;
    }

    ShowGameplayMenu();
}

void AEMRPlayerController::ShowGameplayMenu()
{
    if (!IsLocalController())
    {
        return;
    }

    if (!GameMenuStackTag.IsValid() || GameMenuWidgetClass.IsNull())
    {
        return;
    }

    if (UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        const int32 LocalPushId = ++GameMenuPushId;
        UISubsystem->PushSoftWidgetToStackAsync(
            GameMenuStackTag,
            GameMenuWidgetClass,
            [this, LocalPushId](EAsyncPushWidgetState InPushState, UEMRFrontendCommonActivatableWidgetBase* PushedWidget)
            {
                if (InPushState != EAsyncPushWidgetState::AfterPush)
                {
                    return;
                }

                if (GameMenuPushId != LocalPushId)
                {
                    return;
                }

                ActiveGameMenuWidget = PushedWidget;
            });
    }
}

void AEMRPlayerController::HideGameplayMenu()
{
    ++GameMenuPushId;

    UEMRFrontendCommonActivatableWidgetBase* ActiveWidget = GetActiveGameMenuWidget();
    if (!ActiveWidget)
    {
        ActiveGameMenuWidget = nullptr;
        return;
    }

    if (UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        UISubsystem->RemoveWidgetFromStack(GameMenuStackTag, ActiveWidget);
    }

    if (ActiveGameMenuWidget == ActiveWidget)
    {
        ActiveGameMenuWidget = nullptr;
    }
}

UEMRFrontendCommonActivatableWidgetBase* AEMRPlayerController::GetActiveGameMenuWidget() const
{
    if (!IsLocalController())
    {
        return nullptr;
    }

    if (!GameMenuStackTag.IsValid())
    {
        return nullptr;
    }

    UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(this);
    if (!UISubsystem)
    {
        return nullptr;
    }

    UEMRCommonPrimaryLayoutWidget* PrimaryLayout = UISubsystem->GetPrimaryLayoutWidget();
    if (!PrimaryLayout)
    {
        return nullptr;
    }

    UCommonActivatableWidgetContainerBase* WidgetStack = PrimaryLayout->FindWidgetStackByTag(GameMenuStackTag);
    if (!WidgetStack)
    {
        return nullptr;
    }

    UCommonActivatableWidget* ActiveWidget = WidgetStack->GetActiveWidget();
    return Cast<UEMRFrontendCommonActivatableWidgetBase>(ActiveWidget);
}

TSubclassOf<UEMRCommonPrimaryLayoutWidget> AEMRPlayerController::ResolvePrimaryLayoutWidgetClass()
{
    if (PrimaryLayoutWidgetClass.IsNull())
    {
        return nullptr;
    }

    if (PrimaryLayoutWidgetClass.IsValid())
    {
        return PrimaryLayoutWidgetClass.Get();
    }

    return PrimaryLayoutWidgetClass.LoadSynchronous();
}

void AEMRPlayerController::EnsureGameHudRootWidget()
{
    if (!IsLocalController())
    {
        return;
    }

    if (!GameHudStackTag.IsValid() || GameHudRootWidgetClass.IsNull())
    {
        return;
    }

    if (IsValid(ActiveGameHudRootWidget))
    {
        return;
    }

    if (UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        if (!UISubsystem->GetPrimaryLayoutWidget())
        {
            if (TSubclassOf<UEMRCommonPrimaryLayoutWidget> PrimaryLayoutClass = ResolvePrimaryLayoutWidgetClass())
            {
                CachedPrimaryLayoutWidget = UISubsystem->EnsurePrimaryLayout(this, PrimaryLayoutClass);
            }

            if (!UISubsystem->GetPrimaryLayoutWidget())
            {
                return;
            }
        }

        UISubsystem->PushSoftWidgetToStackAsync(
            GameHudStackTag,
            GameHudRootWidgetClass,
            [this](EAsyncPushWidgetState InPushState, UEMRFrontendCommonActivatableWidgetBase* PushedWidget)
            {
                if (InPushState != EAsyncPushWidgetState::AfterPush)
                {
                    return;
                }

                ActiveGameHudRootWidget = PushedWidget;
            });
    }
}

void AEMRPlayerController::HandlePendingTravelUpdated()
{
    if (!IsLocalController())
    {
        return;
    }

    const UWorld* World = GetWorld();
    const AEMRNightShiftGameState* RunGS = World ? World->GetGameState<AEMRNightShiftGameState>() : nullptr;
    if (!RunGS)
    {
        return;
    }

    const FString TravelURL = RunGS->GetPendingTravelURL();
    if (TravelURL.IsEmpty())
    {
        return;
    }

    bIsTraveling = true;
    bIgnoreNightShiftWidgetPush = true;
    CleanupHubUI();
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][PlayerController] Pending travel signaled -> %s"), *TravelURL);
}

void AEMRPlayerController::HandleNightShiftTelemetryPublished(const FEMRNightShiftTelemetryRecord& Record)
{
    if (!IsLocalController())
    {
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    if (UEMRGameplayTelemetrySubsystem* TelemetrySubsystem = GameInstance->GetSubsystem<UEMRGameplayTelemetrySubsystem>())
    {
        TelemetrySubsystem->WriteNightShiftTelemetryLocal(Record);
    }
}


void AEMRPlayerController::BindToRunState()
{
    if (!IsLocalController())
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        CachedNightShiftGameState = World->GetGameState<AEMRNightShiftGameState>();
    }

    if (AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get())
    {
        if (!RunPhaseChangedHandle.IsValid())
        {
            RunPhaseChangedHandle = RunGS->OnRunPhaseChanged().AddUObject(this, &ThisClass::HandleRunPhaseChanged);
        }
        if (!NightShiftTelemetryPublishedHandle.IsValid())
        {
            NightShiftTelemetryPublishedHandle = RunGS->OnNightShiftTelemetryPublished().AddUObject(this, &ThisClass::HandleNightShiftTelemetryPublished);
        }

        if (RunGS->GetRunPhase() == EER_RunPhase::Hub)
        {
            HandleRunPhaseChanged(EER_RunPhase::Hub, EER_RunPhase::Hub);
        }
    }
}

void AEMRPlayerController::UnbindFromRunState()
{
    if (AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get())
    {
        if (RunPhaseChangedHandle.IsValid())
        {
            RunGS->OnRunPhaseChanged().Remove(RunPhaseChangedHandle);
            RunPhaseChangedHandle.Reset();
        }

        if (NightShiftTelemetryPublishedHandle.IsValid())
        {
            RunGS->OnNightShiftTelemetryPublished().Remove(NightShiftTelemetryPublishedHandle);
            NightShiftTelemetryPublishedHandle.Reset();
        }
    }

    CachedNightShiftGameState.Reset();
}


void AEMRPlayerController::HandleRunPhaseChanged(EER_RunPhase NewPhase, EER_RunPhase PreviousPhase)
{
    if (UEMRGameUserSettings* UserSettings = UEMRGameUserSettings::Get())
    {
        UserSettings->ApplyShowKeybindHelper();
    }
    
    if (NewPhase != EER_RunPhase::Hub)
    {
        CleanupHubUI();
        return;
    }

    SpawnHUDWidgets();
}

void AEMRPlayerController::CleanupHubUI()
{
    if (!IsLocalController())
    {
        return;
    }

    ++NightShiftWidgetPushId;

    if (NightShiftSelectionWidget)
    {
        if (NightShiftSelectionWidget->IsActivated())
        {
            NightShiftSelectionWidget->DeactivateWidget();
        }

        if (UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(this))
        {
            UISubsystem->RemoveWidgetFromStack(WidgetStackTag, NightShiftSelectionWidget);
        }

        NightShiftSelectionWidget = nullptr;
    }

    bNightShiftWidgetPushPending = false;
}
