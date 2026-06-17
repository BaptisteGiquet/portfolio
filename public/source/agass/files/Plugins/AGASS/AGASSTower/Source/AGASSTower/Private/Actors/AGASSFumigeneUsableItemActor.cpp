#include "Actors/AGASSFumigeneUsableItemActor.h"

#include "Actors/AGASSMadPlaneSessionEventActor.h"
#include "Components/AGASSSessionEventManagerComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSFumigene, Log, All);

AAGASSFumigeneUsableItemActor::AAGASSFumigeneUsableItemActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);

	IgnitionEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("IgnitionEffect"));
	IgnitionEffectComponent->SetupAttachment(GetRootComponent());
	IgnitionEffectComponent->SetAutoActivate(false);

	ActiveBurnEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ActiveBurnEffect"));
	ActiveBurnEffectComponent->SetupAttachment(GetRootComponent());
	ActiveBurnEffectComponent->SetAutoActivate(false);

	IgnitionAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("IgnitionAudio"));
	IgnitionAudioComponent->SetupAttachment(GetRootComponent());
	IgnitionAudioComponent->bAutoActivate = false;

	ActiveLoopAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ActiveLoopAudio"));
	ActiveLoopAudioComponent->SetupAttachment(GetRootComponent());
	ActiveLoopAudioComponent->bAutoActivate = false;

	BurnLightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("BurnLight"));
	BurnLightComponent->SetupAttachment(GetRootComponent());
	BurnLightComponent->SetVisibility(false);
	BurnLightComponent->SetCastShadows(false);

	FirstPersonIgnitionEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FirstPersonIgnitionEffect"));
	FirstPersonIgnitionEffectComponent->SetupAttachment(GetFirstPersonPresentationMeshComponent());
	FirstPersonIgnitionEffectComponent->SetAutoActivate(false);
	FirstPersonIgnitionEffectComponent->SetOnlyOwnerSee(true);
	FirstPersonIgnitionEffectComponent->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;

	FirstPersonActiveBurnEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FirstPersonActiveBurnEffect"));
	FirstPersonActiveBurnEffectComponent->SetupAttachment(GetFirstPersonPresentationMeshComponent());
	FirstPersonActiveBurnEffectComponent->SetAutoActivate(false);
	FirstPersonActiveBurnEffectComponent->SetOnlyOwnerSee(true);
	FirstPersonActiveBurnEffectComponent->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;

	FirstPersonBurnLightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("FirstPersonBurnLight"));
	FirstPersonBurnLightComponent->SetupAttachment(GetFirstPersonPresentationMeshComponent());
	FirstPersonBurnLightComponent->SetVisibility(false);
	FirstPersonBurnLightComponent->SetCastShadows(false);
}

void AAGASSFumigeneUsableItemActor::BeginPlay()
{
	Super::BeginPlay();

	RefreshPresentationAssets();
	StopAllCosmeticPresentation();

	if (!HasAuthority() && ActivationState != EAGASSFumigeneActivationState::Inactive)
	{
		OnRep_ActivationState();
	}
}

void AAGASSFumigeneUsableItemActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllCosmeticPresentation();

	Super::EndPlay(EndPlayReason);
}

void AAGASSFumigeneUsableItemActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && IsCarried() && IsSignalActive() && !bMadPlaneSignalAccepted)
	{
		TryConsumeActiveMadPlaneSignal();
	}

	if (ActivationState == EAGASSFumigeneActivationState::Burning)
	{
		RefreshLightIntensityFromServerTime();
	}
}

void AAGASSFumigeneUsableItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ActivationState);
	DOREPLIFETIME(ThisClass, ActivationStateStartServerTimeSeconds);
	DOREPLIFETIME(ThisClass, ActiveWindowEndServerTimeSeconds);
	DOREPLIFETIME(ThisClass, bHasBeenActivated);
	DOREPLIFETIME(ThisClass, bMadPlaneSignalAccepted);
}

bool AAGASSFumigeneUsableItemActor::TryUse(AController* UsingController)
{
	if (!HasAuthority()
		|| UsingController == nullptr
		|| bHasBeenActivated
		|| !IsCarried()
		|| HoldingPlayerState == nullptr
		|| UsingController->PlayerState != HoldingPlayerState)
	{
		return false;
	}

	const double ActivationStartServerTimeSeconds = GetReplicatedServerWorldTimeSeconds();
	const double TotalActiveWindowSeconds =
		FMath::Max(static_cast<double>(IgnitionLeadInSeconds), 0.0)
		+ FMath::Max(static_cast<double>(PreBurnDurationSeconds), 0.0)
		+ FMath::Max(static_cast<double>(BurnDurationSeconds), 0.0);

	bHasBeenActivated = true;
	bMadPlaneSignalAccepted = false;
	ActivationState = EAGASSFumigeneActivationState::Igniting;
	ActivationStateStartServerTimeSeconds = ActivationStartServerTimeSeconds;
	ActiveWindowEndServerTimeSeconds = ActivationStartServerTimeSeconds + TotalActiveWindowSeconds;
	StartIgnitionPhase(false);
	TryConsumeActiveMadPlaneSignal();
	ForceNetUpdate();
	return true;
}

bool AAGASSFumigeneUsableItemActor::CanBeClaimedByController(const AController* Controller) const
{
	return !bHasBeenActivated
		&& ActivationState == EAGASSFumigeneActivationState::Inactive
		&& Super::CanBeClaimedByController(Controller);
}

void AAGASSFumigeneUsableItemActor::OnRep_ActivationState()
{
	switch (ActivationState)
	{
	case EAGASSFumigeneActivationState::Igniting:
		StartIgnitionPhase(true);
		break;

	case EAGASSFumigeneActivationState::Burning:
		StartBurningPhase(true);
		break;

	case EAGASSFumigeneActivationState::Spent:
	case EAGASSFumigeneActivationState::Inactive:
	default:
		StopAllCosmeticPresentation();
		break;
	}
}

void AAGASSFumigeneUsableItemActor::RefreshPresentationAssets()
{
	if (IgnitionEffectComponent != nullptr)
	{
		IgnitionEffectComponent->SetAsset(IgnitionEffectTemplate.LoadSynchronous(), true);
		ApplyNiagaraColorParameters(IgnitionEffectComponent);
	}

	if (FirstPersonIgnitionEffectComponent != nullptr)
	{
		FirstPersonIgnitionEffectComponent->SetAsset(IgnitionEffectTemplate.LoadSynchronous(), true);
		ApplyNiagaraColorParameters(FirstPersonIgnitionEffectComponent);
	}

	if (ActiveBurnEffectComponent != nullptr)
	{
		ActiveBurnEffectComponent->SetAsset(ActiveBurnEffectTemplate.LoadSynchronous(), true);
		ApplyNiagaraColorParameters(ActiveBurnEffectComponent);
	}

	if (FirstPersonActiveBurnEffectComponent != nullptr)
	{
		FirstPersonActiveBurnEffectComponent->SetAsset(ActiveBurnEffectTemplate.LoadSynchronous(), true);
		ApplyNiagaraColorParameters(FirstPersonActiveBurnEffectComponent);
	}

	if (IgnitionAudioComponent != nullptr)
	{
		IgnitionAudioComponent->SetSound(IgnitionSound.LoadSynchronous());
		IgnitionAudioComponent->SetAttenuationSettings(IgnitionSoundAttenuationOverride.LoadSynchronous());
	}

	if (ActiveLoopAudioComponent != nullptr)
	{
		ActiveLoopAudioComponent->SetSound(ActiveLoopSound.LoadSynchronous());
		ActiveLoopAudioComponent->SetAttenuationSettings(ActiveLoopSoundAttenuationOverride.LoadSynchronous());
	}

	if (BurnLightComponent != nullptr)
	{
		BurnLightComponent->SetLightColor(BurnLightColor);
		BurnLightComponent->SetIntensity(BurnLightIntensity);
		BurnLightComponent->SetAttenuationRadius(BurnLightAttenuationRadius);
	}

	if (FirstPersonBurnLightComponent != nullptr)
	{
		FirstPersonBurnLightComponent->SetLightColor(BurnLightColor);
		FirstPersonBurnLightComponent->SetIntensity(BurnLightIntensity);
		FirstPersonBurnLightComponent->SetAttenuationRadius(BurnLightAttenuationRadius);
	}
}

void AAGASSFumigeneUsableItemActor::ApplyNiagaraColorParameters(UNiagaraComponent* NiagaraComponent) const
{
	if (NiagaraComponent == nullptr)
	{
		return;
	}

	if (!SparkColorParameterName.IsNone())
	{
		NiagaraComponent->SetVariableLinearColor(
			SparkColorParameterName,
			SparkColor * SparkColorIntensityMultiplier);
	}

	if (bApplySmokeColorParameter && !SmokeColorParameterName.IsNone())
	{
		NiagaraComponent->SetVariableLinearColor(SmokeColorParameterName, SmokeColor);
	}
}

bool AAGASSFumigeneUsableItemActor::IsSignalActive() const
{
	return ActivationState == EAGASSFumigeneActivationState::Igniting
		|| ActivationState == EAGASSFumigeneActivationState::Burning;
}

bool AAGASSFumigeneUsableItemActor::TryConsumeActiveMadPlaneSignal()
{
	if (!HasAuthority()
		|| bMadPlaneSignalAccepted
		|| !IsSignalActive()
		|| !IsCarried()
		|| HoldingPlayerState == nullptr)
	{
		return false;
	}

	AController* const HoldingController = HoldingPlayerState->GetOwningController();
	if (HoldingController == nullptr)
	{
		return false;
	}

	AGameStateBase* const GameState = GetWorld() != nullptr ? GetWorld()->GetGameState() : nullptr;
	UAGASSSessionEventManagerComponent* const EventManager =
		GameState != nullptr ? GameState->FindComponentByClass<UAGASSSessionEventManagerComponent>() : nullptr;
	if (EventManager == nullptr)
	{
		return false;
	}

	for (const FAGASSActiveSessionEventInfo& ActiveEventInfo : EventManager->GetActiveEvents())
	{
		AAGASSMadPlaneSessionEventActor* const MadPlaneEvent = Cast<AAGASSMadPlaneSessionEventActor>(ActiveEventInfo.EventActor);
		if (MadPlaneEvent != nullptr && MadPlaneEvent->TryAcceptFumigeneSignal(HoldingController))
		{
			bMadPlaneSignalAccepted = true;
			ForceNetUpdate();
			return true;
		}
	}

	return false;
}

void AAGASSFumigeneUsableItemActor::ClearCosmeticTimers()
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.ClearTimer(IgnitionLeadInTimerHandle);
	TimerManager.ClearTimer(IgnitionPhaseFinishTimerHandle);
	TimerManager.ClearTimer(BurnPhaseFinishTimerHandle);
}

void AAGASSFumigeneUsableItemActor::ActivateIgnitionEffect()
{
	if (ActivationState != EAGASSFumigeneActivationState::Igniting || IgnitionEffectComponent == nullptr)
	{
		if (ActivationState != EAGASSFumigeneActivationState::Igniting || FirstPersonIgnitionEffectComponent == nullptr)
		{
			return;
		}
	}

	if (IgnitionEffectComponent != nullptr && IgnitionEffectComponent->GetAsset() != nullptr)
	{
		IgnitionEffectComponent->Activate(true);
	}

	if (FirstPersonIgnitionEffectComponent != nullptr && FirstPersonIgnitionEffectComponent->GetAsset() != nullptr)
	{
		FirstPersonIgnitionEffectComponent->Activate(true);
	}
}

void AAGASSFumigeneUsableItemActor::StartIgnitionPhase(const bool bFromReplication)
{
	RefreshPresentationAssets();
	StopAllCosmeticPresentation();
	SetActorTickEnabled(HasAuthority() || BurnLightIntensityCurve.LoadSynchronous() != nullptr);

	const double CurrentServerTimeSeconds = GetReplicatedServerWorldTimeSeconds();
	const double LeadInSeconds = FMath::Max(static_cast<double>(IgnitionLeadInSeconds), 0.0);
	const double IgnitionWindowSeconds = LeadInSeconds + FMath::Max(static_cast<double>(PreBurnDurationSeconds), 0.0);
	const double ElapsedIgnitionSeconds = FMath::Max(CurrentServerTimeSeconds - ActivationStateStartServerTimeSeconds, 0.0);

	if (IgnitionAudioComponent != nullptr && IgnitionAudioComponent->Sound != nullptr)
	{
		IgnitionAudioComponent->FadeIn(
			FMath::Max(IgnitionFadeInSeconds, 0.0f),
			1.0f,
			static_cast<float>(ElapsedIgnitionSeconds));
	}

	if (ElapsedIgnitionSeconds >= LeadInSeconds)
	{
		ActivateIgnitionEffect();
	}
	else if (UWorld* const World = GetWorld(); World != nullptr)
	{
		World->GetTimerManager().SetTimer(
			IgnitionLeadInTimerHandle,
			this,
			&ThisClass::HandleIgnitionLeadInElapsed,
			static_cast<float>(LeadInSeconds - ElapsedIgnitionSeconds),
			false);
	}

	if (!HasAuthority())
	{
		return;
	}

	if (IgnitionWindowSeconds <= ElapsedIgnitionSeconds + UE_DOUBLE_SMALL_NUMBER)
	{
		HandleIgnitionPhaseFinished();
		return;
	}

	if (!bFromReplication)
	{
		GetWorld()->GetTimerManager().SetTimer(
			IgnitionPhaseFinishTimerHandle,
			this,
			&ThisClass::HandleIgnitionPhaseFinished,
			static_cast<float>(IgnitionWindowSeconds - ElapsedIgnitionSeconds),
			false);
	}
}

void AAGASSFumigeneUsableItemActor::StartBurningPhase(const bool bFromReplication)
{
	RefreshPresentationAssets();
	ClearCosmeticTimers();
	SetActorTickEnabled(HasAuthority() || BurnLightIntensityCurve.LoadSynchronous() != nullptr);

	if (IgnitionEffectComponent != nullptr)
	{
		IgnitionEffectComponent->Deactivate();
	}

	if (FirstPersonIgnitionEffectComponent != nullptr)
	{
		FirstPersonIgnitionEffectComponent->Deactivate();
	}

	if (IgnitionAudioComponent != nullptr)
	{
		if (IgnitionAudioComponent->IsPlaying())
		{
			IgnitionAudioComponent->FadeOut(FMath::Max(IgnitionFadeOutSeconds, 0.0f), 0.0f);
		}
		else
		{
			IgnitionAudioComponent->Stop();
		}
	}

	if (ActiveBurnEffectComponent != nullptr && ActiveBurnEffectComponent->GetAsset() != nullptr)
	{
		ActiveBurnEffectComponent->Activate(true);
	}

	if (FirstPersonActiveBurnEffectComponent != nullptr && FirstPersonActiveBurnEffectComponent->GetAsset() != nullptr)
	{
		FirstPersonActiveBurnEffectComponent->Activate(true);
	}

	const double CurrentServerTimeSeconds = GetReplicatedServerWorldTimeSeconds();
	if (ActiveLoopAudioComponent != nullptr && ActiveLoopAudioComponent->Sound != nullptr)
	{
		ActiveLoopAudioComponent->FadeIn(
			FMath::Max(ActiveLoopFadeInSeconds, 0.0f),
			1.0f,
			0.0f);
	}

	if (BurnLightComponent != nullptr)
	{
		BurnLightComponent->SetVisibility(true);
		RefreshLightIntensityFromServerTime();
	}

	if (FirstPersonBurnLightComponent != nullptr && IsUsingFirstPersonCarryPresentation())
	{
		FirstPersonBurnLightComponent->SetVisibility(true);
		RefreshLightIntensityFromServerTime();
	}

	if (!HasAuthority())
	{
		return;
	}

	const double RemainingBurnSeconds = FMath::Max(ActiveWindowEndServerTimeSeconds - CurrentServerTimeSeconds, 0.0);
	if (RemainingBurnSeconds <= UE_DOUBLE_SMALL_NUMBER)
	{
		HandleBurningPhaseFinished();
		return;
	}

	if (!bFromReplication)
	{
		GetWorld()->GetTimerManager().SetTimer(
			BurnPhaseFinishTimerHandle,
			this,
			&ThisClass::HandleBurningPhaseFinished,
			static_cast<float>(RemainingBurnSeconds),
			false);
	}
}

void AAGASSFumigeneUsableItemActor::FinishActivation()
{
	StopAllCosmeticPresentation();

	if (!HasAuthority())
	{
		return;
	}

	ActivationState = EAGASSFumigeneActivationState::Spent;
	ActivationStateStartServerTimeSeconds = GetReplicatedServerWorldTimeSeconds();
	ActiveWindowEndServerTimeSeconds = ActivationStateStartServerTimeSeconds;
	ForceNetUpdate();
}

void AAGASSFumigeneUsableItemActor::StopAllCosmeticPresentation()
{
	ClearCosmeticTimers();
	SetActorTickEnabled(false);

	if (IgnitionEffectComponent != nullptr)
	{
		IgnitionEffectComponent->Deactivate();
	}

	if (FirstPersonIgnitionEffectComponent != nullptr)
	{
		FirstPersonIgnitionEffectComponent->Deactivate();
	}

	if (ActiveBurnEffectComponent != nullptr)
	{
		ActiveBurnEffectComponent->Deactivate();
	}

	if (FirstPersonActiveBurnEffectComponent != nullptr)
	{
		FirstPersonActiveBurnEffectComponent->Deactivate();
	}

	if (IgnitionAudioComponent != nullptr)
	{
		IgnitionAudioComponent->Stop();
	}

	if (ActiveLoopAudioComponent != nullptr)
	{
		ActiveLoopAudioComponent->Stop();
	}

	if (BurnLightComponent != nullptr)
	{
		BurnLightComponent->SetVisibility(false);
		BurnLightComponent->SetIntensity(0.0f);
	}

	if (FirstPersonBurnLightComponent != nullptr)
	{
		FirstPersonBurnLightComponent->SetVisibility(false);
		FirstPersonBurnLightComponent->SetIntensity(0.0f);
	}
}

void AAGASSFumigeneUsableItemActor::RefreshLightIntensityFromServerTime()
{
	if (ActivationState != EAGASSFumigeneActivationState::Burning)
	{
		return;
	}

	float LightIntensityScale = 1.0f;
	if (UCurveFloat* const IntensityCurve = BurnLightIntensityCurve.LoadSynchronous())
	{
		const double ElapsedBurnSeconds = FMath::Max(GetReplicatedServerWorldTimeSeconds() - ActivationStateStartServerTimeSeconds, 0.0);
		LightIntensityScale = IntensityCurve->GetFloatValue(static_cast<float>(ElapsedBurnSeconds));
	}

	if (BurnLightComponent != nullptr)
	{
		BurnLightComponent->SetIntensity(BurnLightIntensity * FMath::Max(LightIntensityScale, 0.0f));
	}

	if (FirstPersonBurnLightComponent != nullptr && IsUsingFirstPersonCarryPresentation())
	{
		FirstPersonBurnLightComponent->SetIntensity(BurnLightIntensity * FMath::Max(LightIntensityScale, 0.0f));
	}
}

double AAGASSFumigeneUsableItemActor::GetReplicatedServerWorldTimeSeconds() const
{
	const UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return 0.0;
	}

	const AGameStateBase* const GameState = World->GetGameState();
	return GameState != nullptr ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

void AAGASSFumigeneUsableItemActor::HandleIgnitionLeadInElapsed()
{
	ActivateIgnitionEffect();
}

void AAGASSFumigeneUsableItemActor::HandleIgnitionPhaseFinished()
{
	if (!HasAuthority() || ActivationState != EAGASSFumigeneActivationState::Igniting)
	{
		return;
	}

	if (BurnDurationSeconds <= 0.0f)
	{
		FinishActivation();
		return;
	}

	ActivationState = EAGASSFumigeneActivationState::Burning;
	ActivationStateStartServerTimeSeconds = GetReplicatedServerWorldTimeSeconds();
	StartBurningPhase(false);
	ForceNetUpdate();
}

void AAGASSFumigeneUsableItemActor::HandleBurningPhaseFinished()
{
	if (!HasAuthority() || ActivationState != EAGASSFumigeneActivationState::Burning)
	{
		return;
	}

	FinishActivation();
}

void AAGASSFumigeneUsableItemActor::HandleFirstPersonCarryPresentationUpdated()
{
	Super::HandleFirstPersonCarryPresentationUpdated();

	const bool bUseFirstPersonCarryPresentation = IsUsingFirstPersonCarryPresentation();

	if (IgnitionEffectComponent != nullptr)
	{
		IgnitionEffectComponent->SetOwnerNoSee(bUseFirstPersonCarryPresentation);
	}

	if (ActiveBurnEffectComponent != nullptr)
	{
		ActiveBurnEffectComponent->SetOwnerNoSee(bUseFirstPersonCarryPresentation);
	}

	if (FirstPersonIgnitionEffectComponent != nullptr)
	{
		FirstPersonIgnitionEffectComponent->SetHiddenInGame(!bUseFirstPersonCarryPresentation, true);
	}

	if (FirstPersonActiveBurnEffectComponent != nullptr)
	{
		FirstPersonActiveBurnEffectComponent->SetHiddenInGame(!bUseFirstPersonCarryPresentation, true);
	}

	if (FirstPersonBurnLightComponent != nullptr)
	{
		FirstPersonBurnLightComponent->SetHiddenInGame(!bUseFirstPersonCarryPresentation, true);
		if (!bUseFirstPersonCarryPresentation)
		{
			FirstPersonBurnLightComponent->SetVisibility(false);
			FirstPersonBurnLightComponent->SetIntensity(0.0f);
		}
	}

	switch (ActivationState)
	{
	case EAGASSFumigeneActivationState::Igniting:
		StartIgnitionPhase(true);
		break;

	case EAGASSFumigeneActivationState::Burning:
		StartBurningPhase(true);
		break;

	case EAGASSFumigeneActivationState::Spent:
	case EAGASSFumigeneActivationState::Inactive:
	default:
		StopAllCosmeticPresentation();
		break;
	}
}
