#include "Interaction/EMRTreatmentBedFolder.h"

#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Net/UnrealNetwork.h"
#include "UI/Machines/Triage/EMRTriagePatientCard.h"
#include "UI/Patient/EMRPatientUIController.h"
#include "Interaction/EMRFixedPlacementComponent.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Interaction/EMRCarryableComponent.h"
#include "GAS/EMRTags.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/Array.h"
#include "GameFramework/Pawn.h"

AEMRTreatmentBedFolder::AEMRTreatmentBedFolder()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = true;

	PatientCardWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PatientCardWidget"));
	PatientCardWidgetComponent->SetupAttachment(GetRootComponent());
	PatientCardWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	PatientCardWidgetComponent->SetDrawAtDesiredSize(true);
	PatientCardWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PatientCardWidgetComponent->SetVisibility(false);
	PatientCardWidgetComponent->SetHiddenInGame(true);

	FixedPlacementComponent = CreateDefaultSubobject<UEMRFixedPlacementComponent>(TEXT("FixedPlacementComponent"));

	AnchorTraceComponent = CreateDefaultSubobject<USphereComponent>(TEXT("AnchorTraceComponent"));
	AnchorTraceComponent->SetupAttachment(GetRootComponent());
	AnchorTraceComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AnchorTraceComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	AnchorTraceComponent->SetGenerateOverlapEvents(false);
	AnchorTraceComponent->SetUsingAbsoluteLocation(true);
	AnchorTraceComponent->SetUsingAbsoluteRotation(true);
	AnchorTraceComponent->SetUsingAbsoluteScale(true);
}

void AEMRTreatmentBedFolder::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEMRTreatmentBedFolder, AssignedPatient);
}

void AEMRTreatmentBedFolder::BeginPlay()
{
	Super::BeginPlay();

	if (UEMRInteractableComponent* Interactable = FindComponentByClass<UEMRInteractableComponent>())
	{
		Interactable->SetInteractionEventTag(EMRTags::Event::Item::Pick);
	}

	if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
	{
		Carryable->SetPlaceObjectEventTag(FGameplayTag());
		Carryable->SetLockedInPlace(true);
		bWasCarried = Carryable->IsCarried();
	}

	UpdateAnchorTraceCollision(bWasCarried);

	if (HasAuthority())
	{
		SetReturnTraceComponent(AnchorTraceComponent);
	}

	SyncAnchorTraceToFixedAnchor(true);

	RefreshPatientCardWidget();
}

void AEMRTreatmentBedFolder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Keep the confirm trace target pinned to the fixed world anchor.
	SyncAnchorTraceToFixedAnchor(false);
	RefreshPatientCardWorldWidgetVisibility();

	if (!HasAuthority())
	{
		return;
	}

	const UEMRCarryableComponent* Carryable = GetCarryableComponent();
	const bool bIsCarriedNow = Carryable && Carryable->IsCarried();
	if (bIsCarriedNow != bWasCarried)
	{
		bWasCarried = bIsCarriedNow;
		UpdateAnchorTraceCollision(bIsCarriedNow);
	}
}

void AEMRTreatmentBedFolder::SetAssignedPatient(AEMRPatient* Patient)
{
	if (!HasAuthority())
	{
		return;
	}

	AssignedPatient = Patient;
	RefreshPatientCardWidget();
}

void AEMRTreatmentBedFolder::ClearAssignedPatient()
{
	if (!HasAuthority())
	{
		return;
	}

	AssignedPatient = nullptr;
	RefreshPatientCardWidget();
}

void AEMRTreatmentBedFolder::SetAnchorTransform(const FTransform& InTransform)
{
	if (!HasAuthority())
	{
		return;
	}

	if (FixedPlacementComponent)
	{
		FixedPlacementComponent->SetAnchorTransform(InTransform);
	}

	SyncAnchorTraceToFixedAnchor(true);
}

void AEMRTreatmentBedFolder::ReturnToAnchor()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!FixedPlacementComponent || !FixedPlacementComponent->HasAnchor())
	{
		return;
	}

	const FTransform AnchorTransform = FixedPlacementComponent->GetAnchorTransform();

	if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
	{
		Carryable->DropAtLocation(AnchorTransform.GetLocation());
		Carryable->SetLockedInPlace(true);
	}

	SetActorRotation(AnchorTransform.GetRotation());
	UpdateAnchorTraceCollision(false);
}

void AEMRTreatmentBedFolder::SetReturnTraceComponent(UPrimitiveComponent* InComponent)
{
	if (!HasAuthority())
	{
		return;
	}

	ReturnTraceComponent = InComponent;
	UpdateAnchorTraceCollision(bWasCarried);
}

bool AEMRTreatmentBedFolder::TryReturnToAnchorFromTrace(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance)
{
	if (!HasAuthority() || !FixedPlacementComponent || !FixedPlacementComponent->HasAnchor() || !ReturnTraceComponent.IsValid() || !RequestingActor)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector SafeViewDirection = ViewDirection.GetSafeNormal();
	if (SafeViewDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector TraceEnd = ViewLocation + (SafeViewDirection * TraceDistance);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TreatmentFolderReturnTrace), false, RequestingActor);
	QueryParams.AddIgnoredActor(RequestingActor);

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent || PrimitiveComponent == ReturnTraceComponent.Get())
		{
			continue;
		}

		QueryParams.AddIgnoredComponent(PrimitiveComponent);
	}

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
	const UPrimitiveComponent* HitComponent = Hit.GetComponent();
	const bool bHitAnchor = bHit && HitComponent && HitComponent == ReturnTraceComponent.Get();
	if (!bHitAnchor)
	{
		return false;
	}

	ReturnToAnchor();
	return true;
}

bool AEMRTreatmentBedFolder::IsCarriedBy(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	return GetAttachParentActor() == Actor;
}

bool AEMRTreatmentBedFolder::EMRReturnToAnchorFromTrace_Implementation(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance)
{
	return TryReturnToAnchorFromTrace(RequestingActor, ViewLocation, ViewDirection, TraceDistance);
}

void AEMRTreatmentBedFolder::EMRReturnToAnchor_Implementation()
{
	ReturnToAnchor();
}

bool AEMRTreatmentBedFolder::EMRIsCarriedBy_Implementation(const AActor* Actor) const
{
	return IsCarriedBy(Actor);
}

void AEMRTreatmentBedFolder::SyncAnchorTraceToFixedAnchor(bool bForce)
{
	if (!FixedPlacementComponent || !FixedPlacementComponent->HasAnchor() || !AnchorTraceComponent)
	{
		return;
	}

	const FTransform AnchorTransform = FixedPlacementComponent->GetAnchorTransform();
	const bool bNeedsSync = bForce || !AnchorTraceComponent->GetComponentTransform().Equals(AnchorTransform, 0.1f);
	if (bNeedsSync)
	{
		AnchorTraceComponent->SetWorldTransform(AnchorTransform);
	}
}

void AEMRTreatmentBedFolder::UpdateAnchorTraceCollision(bool bEnableBlocking)
{
	if (!HasAuthority() || !ReturnTraceComponent.IsValid())
	{
		return;
	}

	UPrimitiveComponent* TraceComponent = ReturnTraceComponent.Get();
	TraceComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	TraceComponent->SetGenerateOverlapEvents(false);

	if (bEnableBlocking)
	{
		TraceComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TraceComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	else
	{
		TraceComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AEMRTreatmentBedFolder::OnRep_AssignedPatient()
{
	RefreshPatientCardWidget();
}

void AEMRTreatmentBedFolder::RefreshPatientCardWidget()
{
	if (!PatientCardWidgetComponent)
	{
		return;
	}

	const bool bHasPatient = (AssignedPatient != nullptr);
	SetFolderEnabled(bHasPatient);

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (!bHasPatient)
	{
		if (PatientCardUIController)
		{
			PatientCardUIController->StopMonitoring();
			PatientCardUIController = nullptr;
		}

		if (UEMRTriagePatientCard* CardWidget = ResolvePatientCardWidget())
		{
			CardWidget->ResetCard();
		}

		CachedPatientCardPatient.Reset();
		return;
	}

	UEMRTriagePatientCard* CardWidget = ResolvePatientCardWidget();
	if (!CardWidget)
	{
		return;
	}

	ConfigurePatientCard(CardWidget);

	if (CachedPatientCardPatient.Get() == AssignedPatient && PatientCardUIController)
	{
		return;
	}

	if (PatientCardUIController)
	{
		PatientCardUIController->StopMonitoring();
	}

	PatientCardUIController = NewObject<UEMRPatientUIController>(this);
	if (!PatientCardUIController)
	{
		return;
	}

	PatientCardUIController->BindToPatient(AssignedPatient);
	CardWidget->InitializeCard(AssignedPatient, PatientCardUIController);
	CachedPatientCardPatient = AssignedPatient;
}

UEMRTriagePatientCard* AEMRTreatmentBedFolder::ResolvePatientCardWidget()
{
	if (CachedPatientCardWidget.IsValid())
	{
		return CachedPatientCardWidget.Get();
	}

	if (!PatientCardWidgetComponent)
	{
		return nullptr;
	}

	if (PatientCardWidgetClass && PatientCardWidgetComponent->GetWidgetClass() != PatientCardWidgetClass)
	{
		PatientCardWidgetComponent->SetWidgetClass(PatientCardWidgetClass);
	}

	if (!PatientCardWidgetComponent->GetUserWidgetObject())
	{
		PatientCardWidgetComponent->InitWidget();
	}

	UUserWidget* UserWidget = PatientCardWidgetComponent->GetUserWidgetObject();
	if (!UserWidget)
	{
		return nullptr;
	}

	UEMRTriagePatientCard* CardWidget = Cast<UEMRTriagePatientCard>(UserWidget);
	if (!CardWidget)
	{
		return nullptr;
	}

	CachedPatientCardWidget = CardWidget;
	return CardWidget;
}

void AEMRTreatmentBedFolder::ConfigurePatientCard(UEMRTriagePatientCard* CardWidget) const
{
	if (!CardWidget)
	{
		return;
	}

	CardWidget->SetLabAnalyzerWaitingPathologyOverride(
		bEnableLabAnalyzerWaitingPathologyOverride,
		LabAnalyzerWaitingPathologyText);
}

void AEMRTreatmentBedFolder::SetFolderEnabled(bool bEnabled)
{
	RefreshPatientCardWorldWidgetVisibility();

	if (UEMRInteractableComponent* Interactable = FindComponentByClass<UEMRInteractableComponent>())
	{
		Interactable->SetEnabled(bEnabled);
	}
}

void AEMRTreatmentBedFolder::RefreshPatientCardWorldWidgetVisibility()
{
	if (!PatientCardWidgetComponent)
	{
		return;
	}

	const bool bHasPatient = AssignedPatient != nullptr;
	const APawn* CarrierPawn = Cast<APawn>(GetAttachParentActor());
	const bool bHideForLocalCarrier = CarrierPawn && CarrierPawn->IsLocallyControlled();

	const bool bShouldShow = bHasPatient && !bHideForLocalCarrier;
	PatientCardWidgetComponent->SetHiddenInGame(!bShouldShow);
	PatientCardWidgetComponent->SetVisibility(bShouldShow);
}

bool AEMRTreatmentBedFolder::IsInteractableEnabled_Implementation() const
{
	if (!Super::IsInteractableEnabled_Implementation())
	{
		return false;
	}

	const UEMRCarryableComponent* Carryable = GetCarryableComponent();
	return !(Carryable && Carryable->IsCarried());
}

bool AEMRTreatmentBedFolder::GetFirstPersonCarryWidgetSettings_Implementation(FEMRFirstPersonCarryWidgetSettings& OutSettings) const
{
	TSubclassOf<UUserWidget> WidgetClass = PatientCardWidgetClass;
	if (!WidgetClass && PatientCardWidgetComponent)
	{
		WidgetClass = PatientCardWidgetComponent->GetWidgetClass();
	}

	if (!WidgetClass)
	{
		return false;
	}

	OutSettings.WidgetClass = WidgetClass;
	OutSettings.bDrawAtDesiredSize = true;
	OutSettings.bShouldShow = AssignedPatient != nullptr;

	const UStaticMeshComponent* MeshComponent = FindComponentByClass<UStaticMeshComponent>();
	const FTransform BaseTransform = MeshComponent ? MeshComponent->GetComponentTransform() : GetActorTransform();
	OutSettings.RelativeTransform = PatientCardWidgetComponent
	? PatientCardWidgetComponent->GetComponentTransform().GetRelativeTransform(BaseTransform)
	: FTransform::Identity;

	return true;
}

void AEMRTreatmentBedFolder::UpdateFirstPersonCarryWidget_Implementation(UUserWidget* Widget)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UEMRTriagePatientCard* CardWidget = Cast<UEMRTriagePatientCard>(Widget);
	if (!CardWidget)
	{
		return;
	}

	ConfigurePatientCard(CardWidget);

	const bool bHasPatient = AssignedPatient != nullptr;
	if (!bHasPatient)
	{
		CardWidget->ResetCard();
		if (FirstPersonPatientCardUIController)
		{
			FirstPersonPatientCardUIController->StopMonitoring();
			FirstPersonPatientCardUIController = nullptr;
		}
		FirstPersonCachedPatient.Reset();
		return;
	}

	if (FirstPersonCachedPatient.Get() == AssignedPatient && FirstPersonPatientCardUIController)
	{
		return;
	}

	if (FirstPersonPatientCardUIController)
	{
		FirstPersonPatientCardUIController->StopMonitoring();
	}

	FirstPersonPatientCardUIController = NewObject<UEMRPatientUIController>(this);
	if (!FirstPersonPatientCardUIController)
	{
		return;
	}

	FirstPersonPatientCardUIController->BindToPatient(AssignedPatient);
	CardWidget->InitializeCard(AssignedPatient, FirstPersonPatientCardUIController);
	FirstPersonCachedPatient = AssignedPatient;
}

void AEMRTreatmentBedFolder::ResetFirstPersonCarryWidget_Implementation(UUserWidget* Widget)
{
	if (UEMRTriagePatientCard* CardWidget = Cast<UEMRTriagePatientCard>(Widget))
	{
		CardWidget->ResetCard();
	}

	if (FirstPersonPatientCardUIController)
	{
		FirstPersonPatientCardUIController->StopMonitoring();
		FirstPersonPatientCardUIController = nullptr;
	}

	FirstPersonCachedPatient.Reset();
}
