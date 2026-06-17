#include "Components/AGASSInteractionComponent.h"

#include "Actors/AGASSCarryableItemActor.h"
#include "Actors/AGASSPlacementPreviewActor.h"
#include "Actors/AGASSPlaceableItemActor.h"
#include "Actors/AGASSScaffoldingPlaceableActor.h"
#include "Actors/AGASSUsableItemActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/AGASSInteractableInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/AGASSGameplayEventSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSInteractionFlow, Log, All);

UAGASSInteractionComponent::UAGASSInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	PreviewActorClass = AAGASSPlacementPreviewActor::StaticClass();
}

void UAGASSInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	NextPreviewSendTimeSeconds = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0;
	ResetPreviewDistanceState();
}

void UAGASSInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() != nullptr && GetOwner()->HasAuthority())
	{
		ReleaseServerHold(true);
	}

	if (PreviewActor != nullptr)
	{
		PreviewActor->ClearLocalPrediction();
	}

	ResetPreviewDistanceState();
	ResetPlacementRotationState();

	Super::EndPlay(EndPlayReason);
}

void UAGASSInteractionComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerController* const PlayerController = GetOwningPlayerController();
	if (PlayerController == nullptr || !PlayerController->IsLocalController() || HeldItem == nullptr || PreviewActor == nullptr)
	{
		return;
	}

	if (bPlacementRotationModeActive)
	{
		PlayerController->SetControlRotation(LockedPlacementViewRotation);
	}

	const FTransform DesiredPreviewTransform = CalculateDesiredPreviewTransform();
	const EAGASSPlacementValidationReason ValidationReason = EvaluatePreviewTransform(DesiredPreviewTransform);
	PreviewActor->ApplyLocalPredictedTransform(DesiredPreviewTransform, ValidationReason);

	UWorld* const World = GetWorld();
	if (World == nullptr || World->GetTimeSeconds() < NextPreviewSendTimeSeconds)
	{
		return;
	}

	NextPreviewSendTimeSeconds = World->GetTimeSeconds() + PreviewUpdateInterval;
	ServerUpdatePreview(DesiredPreviewTransform.GetLocation(), DesiredPreviewTransform.Rotator());
}

void UAGASSInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, HeldItem);
	DOREPLIFETIME(ThisClass, PreviewActor);
}

void UAGASSInteractionComponent::TryPickupFocusedItem()
{
	TryQuickInteract();
}

bool UAGASSInteractionComponent::TryQuickInteract()
{
	if (HeldItem != nullptr)
	{
		return TryHeldItemPrimaryAction();
	}

	if (!CanInteractInCurrentWorld())
	{
		return false;
	}

	if (AActor* const FocusedActor = FindFocusedInteractActor())
	{
		if (AAGASSCarryableItemActor* const TargetItem = Cast<AAGASSCarryableItemActor>(FocusedActor))
		{
			UE_LOG(
				LogAGASSInteractionFlow,
				Log,
				TEXT("TryQuickInteract found pickup target '%s' for owner '%s'."),
				*GetNameSafe(TargetItem),
				*GetNameSafe(GetOwner()));
			ServerTryPickup(TargetItem);
			return true;
		}

		if (FocusedActor->GetClass()->ImplementsInterface(UAGASSInteractableInterface::StaticClass())
			&& GetRequiredHoldDurationForActor(FocusedActor) <= 0.f)
		{
			UE_LOG(
				LogAGASSInteractionFlow,
				Log,
				TEXT("TryQuickInteract found instant interact target '%s' for owner '%s'."),
				*GetNameSafe(FocusedActor),
				*GetNameSafe(GetOwner()));
			ServerInteractActor(FocusedActor);
			return true;
		}

		UE_LOG(
			LogAGASSInteractionFlow,
			Log,
			TEXT("TryQuickInteract found actor '%s' for owner '%s' but it was not an instant interact target."),
			*GetNameSafe(FocusedActor),
			*GetNameSafe(GetOwner()));
	}
	else
	{
		UE_LOG(
			LogAGASSInteractionFlow,
			Log,
			TEXT("TryQuickInteract found no focused actor for owner '%s'."),
			*GetNameSafe(GetOwner()));
	}

	return false;
}

bool UAGASSInteractionComponent::TryHoldInteract(AActor* TargetActor)
{
	if (HeldItem != nullptr || !CanInteractInCurrentWorld())
	{
		return false;
	}

	if (TargetActor == nullptr)
	{
		TargetActor = FindFocusedInteractActor();
	}

	if (TargetActor == nullptr
		|| !TargetActor->GetClass()->ImplementsInterface(UAGASSInteractableInterface::StaticClass())
		|| GetRequiredHoldDurationForActor(TargetActor) <= 0.f
		|| !IsFocusedInteractActor(TargetActor))
	{
		return false;
	}

	ServerInteractActor(TargetActor);
	return true;
}

bool UAGASSInteractionComponent::ResolveFocusedHoldInteract(AActor*& OutTargetActor, float& OutHoldDurationSeconds) const
{
	OutTargetActor = nullptr;
	OutHoldDurationSeconds = 0.f;

	if (HeldItem != nullptr || !CanInteractInCurrentWorld())
	{
		return false;
	}

	AActor* const FocusedActor = FindFocusedInteractActor();
	APlayerController* const PlayerController = GetOwningPlayerController();
	if (FocusedActor == nullptr
		|| PlayerController == nullptr
		|| !FocusedActor->GetClass()->ImplementsInterface(UAGASSInteractableInterface::StaticClass()))
	{
		return false;
	}

	const float HoldDurationSeconds = GetRequiredHoldDurationForActor(FocusedActor);
	const bool bCanInteract = IAGASSInteractableInterface::Execute_CanAGASSInteract(FocusedActor, PlayerController);
	if (HoldDurationSeconds <= 0.f || !bCanInteract)
	{
		return false;
	}

	OutTargetActor = FocusedActor;
	OutHoldDurationSeconds = HoldDurationSeconds;
	return true;
}

bool UAGASSInteractionComponent::IsFocusedInteractActor(const AActor* TargetActor) const
{
	return TargetActor != nullptr && FindFocusedInteractActor() == TargetActor;
}

void UAGASSInteractionComponent::CancelHeldItem()
{
	if (HeldItem == nullptr)
	{
		return;
	}

	if (IsHeldItemUsingPlacementPrimaryAction())
	{
		TryPlaceHeldItem();
		return;
	}

	ServerCancelHeldItem();
}

void UAGASSInteractionComponent::TryPlaceHeldItem()
{
	if (HeldItem == nullptr || !CanInteractInCurrentWorld())
	{
		return;
	}

	if (!IsHeldItemUsingPlacementPrimaryAction())
	{
		ServerTryHeldPrimaryAction();
		return;
	}

	if (PreviewActor == nullptr)
	{
		return;
	}

	const FTransform DesiredPreviewTransform = CalculateDesiredPreviewTransform();
	const EAGASSPlacementValidationReason ValidationReason = EvaluatePreviewTransform(DesiredPreviewTransform);
	PreviewActor->ApplyLocalPredictedTransform(DesiredPreviewTransform, ValidationReason);
	ServerPlaceHeldItem(DesiredPreviewTransform.GetLocation(), DesiredPreviewTransform.Rotator());
}

void UAGASSInteractionComponent::TogglePlacementRotationMode()
{
	AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem();
	APlayerController* const PlayerController = GetOwningPlayerController();
	if (HeldPlaceableItem == nullptr
		|| PlayerController == nullptr
		|| !PlayerController->IsLocalController()
		|| !HeldPlaceableItem->CanToggleHeldRotationMode())
	{
		return;
	}

	if (bPlacementRotationModeActive)
	{
		DeactivatePlacementRotationMode();
		return;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	bPlacementRotationModeActive = true;
	LockedPlacementViewRotation = PlayerController->GetControlRotation();
	PlayerController->SetIgnoreLookInput(true);
	if (!bHasPlacementRotationOverride)
	{
		ManualPlacementRotation = FRotator(0.f, QuantizePlacementAngle(ViewRotation.Yaw), 0.f);
	}

	bHasLastLoggedQuantizedPlacementRotation = false;
}

bool UAGASSInteractionComponent::IsPlacementRotationModeActive() const
{
	return bPlacementRotationModeActive;
}

bool UAGASSInteractionComponent::TryBeginServerHoldFromSpawnedItem(AAGASSCarryableItemActor* SpawnedItem)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return false;
	}

	return BeginServerHold(SpawnedItem, true);
}

void UAGASSInteractionComponent::ForceReleaseHeldItem(const bool bRestoreSourceTransform)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}

	ReleaseServerHold(bRestoreSourceTransform);
}

void UAGASSInteractionComponent::ApplyPlacementRotationInput(const FVector2D& RotationInput)
{
	if (!bPlacementRotationModeActive || HeldItem == nullptr || RotationInput.IsNearlyZero())
	{
		return;
	}

	ManualPlacementRotation.Pitch = DoesHeldItemSupportPitchRotation()
		? FRotator::NormalizeAxis(ManualPlacementRotation.Pitch + RotationInput.Y)
		: 0.f;
	ManualPlacementRotation.Yaw = FRotator::NormalizeAxis(ManualPlacementRotation.Yaw - RotationInput.X);
	ManualPlacementRotation.Roll = 0.f;
	bHasPlacementRotationOverride = true;
	bHasLastLoggedQuantizedPlacementRotation = false;
}

void UAGASSInteractionComponent::AdjustPreviewDistance(const float DistanceInput)
{
	AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem();
	APlayerController* const PlayerController = GetOwningPlayerController();
	if (HeldPlaceableItem == nullptr
		|| PlayerController == nullptr
		|| !PlayerController->IsLocalController()
		|| !HeldPlaceableItem->CanAdjustHeldPreviewDistance()
		|| FMath::IsNearlyZero(DistanceInput))
	{
		return;
	}

	const float MinDistance = FMath::Max(MinPreviewDistanceFromPawn, 0.f);
	const float MaxDistance = FMath::Max(MaxPreviewDistanceFromPawn, MinDistance);
	CurrentPreviewDistanceFromPawn = FMath::Clamp(
		CurrentPreviewDistanceFromPawn + (DistanceInput * PreviewDistanceAdjustmentStep),
		MinDistance,
		MaxDistance);
}

void UAGASSInteractionComponent::ServerTryPickup_Implementation(AAGASSCarryableItemActor* TargetItem)
{
	if (BeginServerHold(TargetItem))
	{
		ClientPlayPickupItemSound();
	}
}

void UAGASSInteractionComponent::ServerInteractActor_Implementation(AActor* TargetActor)
{
	APlayerController* const PlayerController = GetOwningPlayerController();
	if (HeldItem != nullptr || TargetActor == nullptr || PlayerController == nullptr || !TargetActor->GetClass()->ImplementsInterface(UAGASSInteractableInterface::StaticClass()))
	{
		UE_LOG(
			LogAGASSInteractionFlow,
			Log,
			TEXT("ServerInteractActor rejected request. Owner='%s' Target='%s' HeldItem='%s' PlayerController='%s' ImplementsInterface=%d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetActor),
			*GetNameSafe(HeldItem),
			*GetNameSafe(PlayerController),
			TargetActor != nullptr && TargetActor->GetClass()->ImplementsInterface(UAGASSInteractableInterface::StaticClass()) ? 1 : 0);
		return;
	}

	UE_LOG(
		LogAGASSInteractionFlow,
		Log,
		TEXT("ServerInteractActor evaluating target '%s' for owner '%s'."),
		*GetNameSafe(TargetActor),
		*GetNameSafe(GetOwner()));

	const bool bCanInteract = IAGASSInteractableInterface::Execute_CanAGASSInteract(TargetActor, PlayerController);
	if (!bCanInteract)
	{
		UE_LOG(
			LogAGASSInteractionFlow,
			Log,
			TEXT("ServerInteractActor denied by target '%s' for owner '%s'."),
			*GetNameSafe(TargetActor),
			*GetNameSafe(GetOwner()));
		return;
	}

	UE_LOG(
		LogAGASSInteractionFlow,
		Log,
		TEXT("ServerInteractActor accepted by target '%s' for owner '%s'. Dispatching HandleAGASSInteract."),
		*GetNameSafe(TargetActor),
		*GetNameSafe(GetOwner()));
	IAGASSInteractableInterface::Execute_HandleAGASSInteract(TargetActor, PlayerController);
}

void UAGASSInteractionComponent::ServerCancelHeldItem_Implementation()
{
	if (HeldItem == nullptr)
	{
		return;
	}

	if (!IsHeldItemUsingPlacementPrimaryAction())
	{
		ReleaseServerHold(false);
		return;
	}

	if (PreviewActor == nullptr)
	{
		return;
	}

	ProcessServerPlacementRequest(PreviewActor->GetServerPreviewTransform());
}

void UAGASSInteractionComponent::ServerTryHeldPrimaryAction_Implementation()
{
	if (HeldItem == nullptr)
	{
		return;
	}

	if (IsHeldItemUsingPlacementPrimaryAction())
	{
		if (PreviewActor == nullptr)
		{
			return;
		}

		ProcessServerPlacementRequest(PreviewActor->GetServerPreviewTransform());
		return;
	}

	APlayerController* const PlayerController = GetOwningPlayerController();
	AAGASSUsableItemActor* const HeldUsableItem = GetHeldUsableItem();
	if (PlayerController == nullptr || HeldUsableItem == nullptr || !HeldUsableItem->TryUse(PlayerController))
	{
		return;
	}

	HeldUsableItem->PlayUseMontageForController(PlayerController);
	FinalizeSuccessfulHeldPrimaryAction(HeldUsableItem->ConsumeOnSuccessfulUse());
}

void UAGASSInteractionComponent::ServerUpdatePreview_Implementation(const FVector_NetQuantize100& PreviewLocation, const FRotator& PreviewRotation)
{
	AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem();
	if (HeldPlaceableItem == nullptr || PreviewActor == nullptr)
	{
		return;
	}

	const FTransform NewTransform(
		FRotator(
			DoesHeldItemSupportPitchRotation() ? QuantizePlacementAngle(PreviewRotation.Pitch) : 0.f,
			QuantizePlacementAngle(PreviewRotation.Yaw),
			0.f),
		PreviewLocation);
	const FTransform AdjustedTransform = HeldPlaceableItem->AdjustDesiredPreviewTransform(NewTransform, GetOwningPawn(), PreviewActor);
	if (!CanServerAcceptPreviewLocation(AdjustedTransform.GetLocation()))
	{
		return;
	}

	UpdateServerPreviewTransform(AdjustedTransform, EvaluatePreviewTransform(AdjustedTransform));
}

void UAGASSInteractionComponent::ServerPlaceHeldItem_Implementation(const FVector_NetQuantize100& PreviewLocation, const FRotator& PreviewRotation)
{
	if (GetHeldPlaceableItem() == nullptr || PreviewActor == nullptr)
	{
		return;
	}

	const FTransform CandidateTransform(
		FRotator(
			DoesHeldItemSupportPitchRotation() ? QuantizePlacementAngle(PreviewRotation.Pitch) : 0.f,
			QuantizePlacementAngle(PreviewRotation.Yaw),
			0.f),
		PreviewLocation);
	ProcessServerPlacementRequest(CandidateTransform);
}

void UAGASSInteractionComponent::OnRep_PreviewActor(AAGASSPlacementPreviewActor* PreviousPreviewActor)
{
	if (PreviousPreviewActor != nullptr)
	{
		PreviousPreviewActor->ClearLocalPrediction();
	}

	if (PreviewActor != nullptr)
	{
		if (AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem())
		{
			PreviewActor->InitializeFromPlaceableItem(HeldPlaceableItem);
		}
	}
}

void UAGASSInteractionComponent::OnRep_HeldItem()
{
	if (GetHeldPlaceableItem() != nullptr)
	{
		InitializePreviewStateFromHeldItem();
	}
	else
	{
		ResetPreviewDistanceState();
		ResetPlacementRotationState();
	}

	if (PreviewActor != nullptr)
	{
		if (AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem())
		{
			PreviewActor->InitializeFromPlaceableItem(HeldPlaceableItem);
		}
	}
}

bool UAGASSInteractionComponent::CanInteractInCurrentWorld() const
{
	const UWorld* const World = GetWorld();
	return World != nullptr && World->IsGameWorld() && GetOwningPlayerController() != nullptr;
}

bool UAGASSInteractionComponent::CanServerClaimTarget(const AAGASSCarryableItemActor* TargetItem) const
{
	APlayerController* const PlayerController = GetOwningPlayerController();
	if (TargetItem == nullptr || PlayerController == nullptr || PlayerController->PlayerState == nullptr)
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const float MaxDistanceSquared = FMath::Square(PickupTraceDistance + 75.f);
	return FVector::DistSquared(ViewLocation, TargetItem->GetActorLocation()) <= MaxDistanceSquared && TargetItem->CanBeClaimedBy(PlayerController);
}

bool UAGASSInteractionComponent::CanServerAcceptPreviewLocation(const FVector& PreviewLocation) const
{
	const APawn* const Pawn = GetOwningPawn();
	if (Pawn == nullptr)
	{
		return false;
	}

	return FVector::DistSquared(Pawn->GetActorLocation(), PreviewLocation) <= FMath::Square(MaxPreviewDistanceFromPawn);
}

EAGASSPlacementValidationReason UAGASSInteractionComponent::EvaluatePreviewTransform(const FTransform& CandidateTransform) const
{
	const AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem();
	if (HeldPlaceableItem == nullptr)
	{
		return EAGASSPlacementValidationReason::ItemRuleFailed;
	}

	if (!CanServerAcceptPreviewLocation(CandidateTransform.GetLocation()))
	{
		return EAGASSPlacementValidationReason::OutOfRange;
	}

	if (!IsHeldItemUsingPlacementPrimaryAction())
	{
		return EAGASSPlacementValidationReason::Valid;
	}

	return HeldPlaceableItem->EvaluatePlacementTransform(CandidateTransform, GetOwningPawn(), PreviewActor);
}

void UAGASSInteractionComponent::ResetPreviewDistanceState()
{
	const float MinDistance = FMath::Max(MinPreviewDistanceFromPawn, 0.f);
	const float MaxDistance = FMath::Max(MaxPreviewDistanceFromPawn, MinDistance);
	CurrentPreviewDistanceFromPawn = FMath::Clamp(PreviewTraceDistance, MinDistance, MaxDistance);
}

void UAGASSInteractionComponent::InitializePreviewStateFromHeldItem()
{
	const AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem();
	if (HeldPlaceableItem == nullptr)
	{
		ResetPreviewDistanceState();
		ResetPlacementRotationState();
		return;
	}

	DeactivatePlacementRotationMode();
	bHasLastLoggedQuantizedPlacementRotation = false;
	LastLoggedQuantizedPlacementRotation = FRotator::ZeroRotator;

	const FRotator SourceRotation = HeldPlaceableItem->GetActorRotation();
	ManualPlacementRotation = FRotator(
		DoesHeldItemSupportPitchRotation() ? FRotator::NormalizeAxis(SourceRotation.Pitch) : 0.f,
		FRotator::NormalizeAxis(SourceRotation.Yaw),
		0.f);
	bHasPlacementRotationOverride = true;

	const float MinDistance = FMath::Max(MinPreviewDistanceFromPawn, 0.f);
	const float MaxDistance = FMath::Max(MaxPreviewDistanceFromPawn, 0.f);
	float InitialPreviewDistance = PreviewTraceDistance;

	if (APlayerController* const PlayerController = GetOwningPlayerController())
	{
		FVector ViewLocation = FVector::ZeroVector;
		FRotator IgnoredViewRotation = FRotator::ZeroRotator;
		PlayerController->GetPlayerViewPoint(ViewLocation, IgnoredViewRotation);
		InitialPreviewDistance = FVector::Dist(ViewLocation, HeldPlaceableItem->GetActorLocation());
	}
	else if (const APawn* const Pawn = GetOwningPawn())
	{
		InitialPreviewDistance = FVector::Dist(Pawn->GetActorLocation(), HeldPlaceableItem->GetActorLocation());
	}

	const float PreferredPreviewDistance = HeldPlaceableItem->GetPreferredHeldPreviewDistance();
	if (PreferredPreviewDistance > 0.f)
	{
		InitialPreviewDistance = PreferredPreviewDistance;
	}

	CurrentPreviewDistanceFromPawn = FMath::Clamp(InitialPreviewDistance, MinDistance, MaxDistance);
}

float UAGASSInteractionComponent::GetPlacementVerticalOffset(const FRotator& PlacementRotation) const
{
	const AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem();
	if (HeldPlaceableItem == nullptr)
	{
		return 0.f;
	}

	const FVector HalfExtent = HeldPlaceableItem->GetPreviewHalfExtent();
	const FRotationMatrix RotationMatrix(PlacementRotation);
	const FVector AxisX = RotationMatrix.GetScaledAxis(EAxis::X).GetAbs();
	const FVector AxisY = RotationMatrix.GetScaledAxis(EAxis::Y).GetAbs();
	const FVector AxisZ = RotationMatrix.GetScaledAxis(EAxis::Z).GetAbs();

	return (AxisX.Z * HalfExtent.X) + (AxisY.Z * HalfExtent.Y) + (AxisZ.Z * HalfExtent.Z);
}

bool UAGASSInteractionComponent::DoesHeldItemSupportPitchRotation() const
{
	if (const AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem())
	{
		return HeldPlaceableItem->SupportsPlacementPitchRotation();
	}

	return false;
}

FRotator UAGASSInteractionComponent::ResolveDesiredPlacementRotation(const FRotator& ViewRotation) const
{
	if (bPlacementRotationModeActive || bHasPlacementRotationOverride)
	{
		return FRotator(
			DoesHeldItemSupportPitchRotation() ? QuantizePlacementAngle(ManualPlacementRotation.Pitch) : 0.f,
			QuantizePlacementAngle(ManualPlacementRotation.Yaw),
			0.f);
	}

	return FRotator(0.f, QuantizePlacementAngle(ViewRotation.Yaw), 0.f);
}

float UAGASSInteractionComponent::QuantizePlacementAngle(const float RawAngle) const
{
	const AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem();
	const float RotationStepDegrees = HeldPlaceableItem != nullptr ? HeldPlaceableItem->GetPlacementRotationStepDegrees() : 5.f;
	if (RotationStepDegrees <= KINDA_SMALL_NUMBER)
	{
		return FRotator::NormalizeAxis(RawAngle);
	}

	return FRotator::NormalizeAxis(FMath::RoundToFloat(RawAngle / RotationStepDegrees) * RotationStepDegrees);
}

void UAGASSInteractionComponent::DeactivatePlacementRotationMode()
{
	bPlacementRotationModeActive = false;
	LockedPlacementViewRotation = FRotator::ZeroRotator;

	if (APlayerController* const PlayerController = GetOwningPlayerController())
	{
		if (PlayerController->IsLocalController())
		{
			PlayerController->SetIgnoreLookInput(false);
		}
	}
}

void UAGASSInteractionComponent::ResetPlacementRotationState()
{
	DeactivatePlacementRotationMode();
	bHasPlacementRotationOverride = false;
	ManualPlacementRotation = FRotator::ZeroRotator;
	bHasLastLoggedQuantizedPlacementRotation = false;
	LastLoggedQuantizedPlacementRotation = FRotator::ZeroRotator;
}

APlayerController* UAGASSInteractionComponent::GetOwningPlayerController() const
{
	return Cast<APlayerController>(GetOwner());
}

APawn* UAGASSInteractionComponent::GetOwningPawn() const
{
	if (const APlayerController* const PlayerController = GetOwningPlayerController())
	{
		return PlayerController->GetPawn();
	}

	return nullptr;
}

AActor* UAGASSInteractionComponent::FindFocusedInteractActor() const
{
	APlayerController* const PlayerController = GetOwningPlayerController();
	UWorld* const World = GetWorld();
	if (PlayerController == nullptr || World == nullptr)
	{
		return nullptr;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * PickupTraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSInteractTrace), false);
	if (APawn* const Pawn = PlayerController->GetPawn())
	{
		QueryParams.AddIgnoredActor(Pawn);
	}

	if (PreviewActor != nullptr)
	{
		QueryParams.AddIgnoredActor(PreviewActor);
	}

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		return nullptr;
	}

	AActor* const HitActor = Hit.GetActor();
	UE_LOG(
		LogAGASSInteractionFlow,
		Log,
		TEXT("FindFocusedInteractActor for owner '%s' hit actor '%s' at distance %.1f."),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(HitActor),
		Hit.Distance);
	return HitActor;
}

AAGASSCarryableItemActor* UAGASSInteractionComponent::FindFocusedPickupTarget() const
{
	return Cast<AAGASSCarryableItemActor>(FindFocusedInteractActor());
}

float UAGASSInteractionComponent::GetRequiredHoldDurationForActor(const AActor* TargetActor) const
{
	if (TargetActor == nullptr || !TargetActor->GetClass()->ImplementsInterface(UAGASSInteractableInterface::StaticClass()))
	{
		return 0.f;
	}

	return FMath::Max(
		IAGASSInteractableInterface::Execute_GetAGASSInteractHoldDurationSeconds(
			const_cast<AActor*>(TargetActor),
			GetOwningPlayerController()),
		0.f);
}

FTransform UAGASSInteractionComponent::CalculateDesiredPreviewTransform() const
{
	APlayerController* const PlayerController = GetOwningPlayerController();
	UWorld* const World = GetWorld();
	if (PlayerController == nullptr || World == nullptr)
	{
		return FTransform::Identity;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	if (bPlacementRotationModeActive)
	{
		ViewRotation = LockedPlacementViewRotation;
	}

	const FRotator DesiredRotation = ResolveDesiredPlacementRotation(ViewRotation);
	const FVector DesiredForward = DoesHeldItemSupportPitchRotation()
		? ViewRotation.Vector()
		: FRotationMatrix(FRotator(0.f, ViewRotation.Yaw, 0.f)).GetUnitAxis(EAxis::X);
	FVector DesiredLocation = ViewLocation + (DesiredForward * CurrentPreviewDistanceFromPawn);
	DesiredLocation = ConstrainDesiredPreviewLocation(DesiredLocation);

	FTransform DesiredPreviewTransform(DesiredRotation, DesiredLocation);
	if (const AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem())
	{
		DesiredPreviewTransform = HeldPlaceableItem->AdjustDesiredPreviewTransform(DesiredPreviewTransform, GetOwningPawn(), PreviewActor);
	}

	return DesiredPreviewTransform;
}

FVector UAGASSInteractionComponent::ConstrainDesiredPreviewLocation(const FVector& DesiredLocation) const
{
	const APawn* const Pawn = GetOwningPawn();
	if (Pawn == nullptr)
	{
		return DesiredLocation;
	}

	const FVector PawnLocation = Pawn->GetActorLocation();
	const FVector PawnToDesired = DesiredLocation - PawnLocation;
	const float DesiredDistance = PawnToDesired.Size();
	if (DesiredDistance <= MaxPreviewDistanceFromPawn || DesiredDistance <= KINDA_SMALL_NUMBER)
	{
		return DesiredLocation;
	}

	return PawnLocation + (PawnToDesired / DesiredDistance) * MaxPreviewDistanceFromPawn;
}

bool UAGASSInteractionComponent::TryHeldItemPrimaryAction()
{
	if (HeldItem == nullptr || !CanInteractInCurrentWorld())
	{
		return false;
	}

	if (IsHeldItemUsingPlacementPrimaryAction())
	{
		TryPlaceHeldItem();
		return true;
	}

	ServerTryHeldPrimaryAction();
	return true;
}

bool UAGASSInteractionComponent::IsHeldItemUsingPlacementPrimaryAction() const
{
	return GetHeldPlaceableItem() != nullptr;
}

bool UAGASSInteractionComponent::BeginServerHold(AAGASSCarryableItemActor* TargetItem, const bool bSkipRangeValidation)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || HeldItem != nullptr || PreviewActor != nullptr || TargetItem == nullptr)
	{
		return false;
	}

	APlayerController* const PlayerController = GetOwningPlayerController();
	if (PlayerController == nullptr)
	{
		return false;
	}

	if (!bSkipRangeValidation && !CanServerClaimTarget(TargetItem))
	{
		return false;
	}

	const bool bClaimed = bSkipRangeValidation
		? TargetItem->ClaimForImmediateCarry(PlayerController)
		: TargetItem->Claim(PlayerController);
	if (!bClaimed)
	{
		return false;
	}

	HeldItem = TargetItem;
	if (AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem())
	{
		InitializePreviewStateFromHeldItem();
		const FTransform InitialPreviewTransform(
			ResolveDesiredPlacementRotation(HeldPlaceableItem->GetActorRotation()),
			HeldPlaceableItem->GetActorLocation());
		UClass* ResolvedPreviewActorClass = PreviewActorClass.LoadSynchronous();
		if (ResolvedPreviewActorClass == nullptr)
		{
			ResolvedPreviewActorClass = AAGASSPlacementPreviewActor::StaticClass();
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = PlayerController;
		SpawnParameters.Instigator = GetOwningPawn();
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		PreviewActor = GetWorld()->SpawnActor<AAGASSPlacementPreviewActor>(
			ResolvedPreviewActorClass,
			InitialPreviewTransform,
			SpawnParameters);

		if (PreviewActor == nullptr)
		{
			ResetPreviewDistanceState();
			ResetPlacementRotationState();
			HeldItem = nullptr;
			TargetItem->ReleaseClaim(true);
			return false;
		}

		PreviewActor->SetSourceItem(HeldPlaceableItem);
		if (HeldPlaceableItem->ShouldProvideCarryPreviewCollision())
		{
			HeldPlaceableItem->TransferCharacterMovementBases(HeldPlaceableItem->GetMovementBaseComponent(), PreviewActor->GetMovementBaseComponent());
		}

		UpdateServerPreviewTransform(InitialPreviewTransform, EvaluatePreviewTransform(InitialPreviewTransform));
	}
	GetOwner()->ForceNetUpdate();
	return true;
}

void UAGASSInteractionComponent::ProcessServerPlacementRequest(const FTransform& CandidateTransform)
{
	AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem();
	if (HeldPlaceableItem == nullptr || PreviewActor == nullptr)
	{
		return;
	}

	const FTransform AdjustedTransform = HeldPlaceableItem->AdjustDesiredPreviewTransform(CandidateTransform, GetOwningPawn(), PreviewActor);

	if (!CanServerAcceptPreviewLocation(AdjustedTransform.GetLocation()))
	{
		return;
	}

	const EAGASSPlacementValidationReason ValidationReason = EvaluatePreviewTransform(AdjustedTransform);
	if (ValidationReason != EAGASSPlacementValidationReason::Valid)
	{
		UpdateServerPreviewTransform(AdjustedTransform, ValidationReason);
		return;
	}

	FTransform ApprovedTransform = AdjustedTransform;
	FTransform SnappedPlacementTransform;
	if (HeldPlaceableItem->TryResolveTouchingPlacementSnapTransform(AdjustedTransform, GetOwningPawn(), PreviewActor, SnappedPlacementTransform)
		&& CanServerAcceptPreviewLocation(SnappedPlacementTransform.GetLocation()))
	{
		ApprovedTransform = SnappedPlacementTransform;
	}

	UpdateServerPreviewTransform(ApprovedTransform, EAGASSPlacementValidationReason::Valid);
	FinalizeServerPlacement(ApprovedTransform);
}

void UAGASSInteractionComponent::FinalizeSuccessfulHeldPrimaryAction(const bool bConsumeHeldItem)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || HeldItem == nullptr)
	{
		return;
	}

	if (!bConsumeHeldItem)
	{
		GetOwner()->ForceNetUpdate();
		return;
	}

	AAGASSCarryableItemActor* const ConsumedItem = HeldItem;
	if (AAGASSPlaceableItemActor* const ConsumedPlaceableItem = GetHeldPlaceableItem(); PreviewActor != nullptr && ConsumedPlaceableItem != nullptr)
	{
		if (ConsumedPlaceableItem->ShouldProvideCarryPreviewCollision())
		{
			ConsumedPlaceableItem->TransferCharacterMovementBases(PreviewActor->GetMovementBaseComponent(), nullptr);
		}

		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}

	HeldItem = nullptr;
	ConsumedItem->Destroy();
	ResetPreviewDistanceState();
	ResetPlacementRotationState();
	GetOwner()->ForceNetUpdate();
}

void UAGASSInteractionComponent::FinalizeServerPlacement(const FTransform& ApprovedTransform)
{
	AAGASSPlaceableItemActor* const ItemToPlace = GetHeldPlaceableItem();
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || ItemToPlace == nullptr)
	{
		return;
	}

	if (!ItemToPlace->PlaceAtTransform(ApprovedTransform))
	{
		return;
	}

	if (PreviewActor != nullptr)
	{
		if (ItemToPlace->ShouldProvideCarryPreviewCollision())
		{
			ItemToPlace->TransferCharacterMovementBases(PreviewActor->GetMovementBaseComponent(), ItemToPlace->GetMovementBaseComponent());
		}

		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}

	if (AAGASSScaffoldingPlaceableActor* const ScaffoldingItem = Cast<AAGASSScaffoldingPlaceableActor>(ItemToPlace))
	{
		ScaffoldingItem->RefreshPlacedVisualState();
	}

	ResetPreviewDistanceState();
	ResetPlacementRotationState();
	HeldItem = nullptr;
	UAGASSGameplayEventSubsystem::BroadcastGameplayEvent(this, AGASSGameplayEventNames::ItemPlaced(), 1, 0.f, true);
	ClientPlayPlaceItemSound();
	GetOwner()->ForceNetUpdate();
}

void UAGASSInteractionComponent::ReleaseServerHold(const bool bRestoreSourceTransform)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (HeldItem != nullptr)
	{
		UPrimitiveComponent* const PreviewMovementBaseComponent = PreviewActor != nullptr ? PreviewActor->GetMovementBaseComponent() : nullptr;
		if (AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem())
		{
			HeldPlaceableItem->ReleaseClaim(bRestoreSourceTransform);
			if (HeldPlaceableItem->ShouldProvideCarryPreviewCollision())
			{
				HeldPlaceableItem->TransferCharacterMovementBases(PreviewMovementBaseComponent, HeldPlaceableItem->GetMovementBaseComponent());
			}
		}
		else
		{
			HeldItem->ReleaseFromCarry(ResolveHeldUsableDropTransform());
		}

		HeldItem = nullptr;
	}

	if (PreviewActor != nullptr)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}

	ResetPreviewDistanceState();
	ResetPlacementRotationState();
	GetOwner()->ForceNetUpdate();
}

void UAGASSInteractionComponent::UpdateServerPreviewTransform(const FTransform& NewTransform, const EAGASSPlacementValidationReason ValidationReason)
{
	if (PreviewActor == nullptr)
	{
		return;
	}

	if (AAGASSPlaceableItemActor* const HeldPlaceableItem = GetHeldPlaceableItem();
		HeldPlaceableItem != nullptr && HeldPlaceableItem->ShouldProvideCarryPreviewCollision())
	{
		HeldPlaceableItem->SetActorTransform(NewTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	PreviewActor->SetReplicatedPreviewState(NewTransform, ValidationReason);
}

FTransform UAGASSInteractionComponent::ResolveHeldUsableDropTransform() const
{
	const APawn* const Pawn = GetOwningPawn();
	if (Pawn == nullptr)
	{
		return HeldItem != nullptr ? HeldItem->GetActorTransform() : FTransform::Identity;
	}

	const FVector ForwardVector = Pawn->GetActorForwardVector().GetSafeNormal();
	const FVector DropLocation = Pawn->GetActorLocation() + (ForwardVector * 90.f) + FVector(0.f, 0.f, 45.f);
	return FTransform(Pawn->GetActorRotation(), DropLocation);
}

AAGASSPlaceableItemActor* UAGASSInteractionComponent::GetHeldPlaceableItem() const
{
	return Cast<AAGASSPlaceableItemActor>(HeldItem);
}

AAGASSUsableItemActor* UAGASSInteractionComponent::GetHeldUsableItem() const
{
	return Cast<AAGASSUsableItemActor>(HeldItem);
}

void UAGASSInteractionComponent::ClientPlayPickupItemSound_Implementation()
{
	PlayLocalSound2D(PickupItemSound);
}

void UAGASSInteractionComponent::ClientPlayPlaceItemSound_Implementation()
{
	PlayLocalSound2D(PlaceItemSound);
}

void UAGASSInteractionComponent::PlayLocalSound2D(USoundBase* Sound) const
{
	const APlayerController* const PlayerController = GetOwningPlayerController();
	if (Sound == nullptr || PlayerController == nullptr || !PlayerController->IsLocalController())
	{
		return;
	}

	UGameplayStatics::PlaySound2D(this, Sound);
}
