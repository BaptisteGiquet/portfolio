#include "Actors/AGASSObjectiveActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/AGASSObjectiveRuntimeInterface.h"
#include "UObject/ConstructorHelpers.h"

AAGASSObjectiveActor::AAGASSObjectiveActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ObjectiveMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObjectiveMesh"));
	ObjectiveMeshComponent->SetupAttachment(SceneRoot);
	ObjectiveMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ObjectiveMeshComponent->SetGenerateOverlapEvents(false);
	ObjectiveMeshComponent->SetCanEverAffectNavigation(false);
	ObjectiveMeshComponent->SetSimulatePhysics(false);

	ObjectiveTriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ObjectiveTriggerVolume"));
	ObjectiveTriggerVolume->SetupAttachment(SceneRoot);
	ObjectiveTriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	ObjectiveTriggerVolume->SetGenerateOverlapEvents(true);
	ObjectiveTriggerVolume->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		FallbackObjectiveMesh = SphereMesh.Object;
		ObjectiveMeshComponent->SetStaticMesh(SphereMesh.Object);
		ObjectiveMeshComponent->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));
	}
}

void AAGASSObjectiveActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshObjectivePresentation();
}

void AAGASSObjectiveActor::BeginPlay()
{
	Super::BeginPlay();

	RefreshObjectivePresentation();

	if (HasAuthority() && ObjectiveTriggerVolume != nullptr)
	{
		ObjectiveTriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleTriggerBeginOverlap);
	}
}

FText AAGASSObjectiveActor::GetObjectiveNameText() const
{
	return !ObjectiveName.IsEmpty()
		? ObjectiveName
		: NSLOCTEXT("AGASSTower", "ObjectiveNameFallback", "Tower Objective");
}

FText AAGASSObjectiveActor::GetObjectiveDescriptionText() const
{
	return ObjectiveDescription;
}

FText AAGASSObjectiveActor::GetCompletionTitleText() const
{
	return !CompletionTitle.IsEmpty()
		? CompletionTitle
		: NSLOCTEXT("AGASSTower", "CompletionTitleFallback", "Objective Complete");
}

FText AAGASSObjectiveActor::GetCompletionBodyText() const
{
	return !CompletionBody.IsEmpty()
		? CompletionBody
		: NSLOCTEXT("AGASSTower", "CompletionBodyFallback", "The team reached the tower objective. Returning to FrontendMap shortly.");
}

FText AAGASSObjectiveActor::GetReturnToFrontendReasonText() const
{
	return !ReturnToFrontendReason.IsEmpty()
		? ReturnToFrontendReason
		: NSLOCTEXT("AGASSTower", "ReturnToFrontendReasonFallback", "Run complete. Returning to FrontendMap.");
}

float AAGASSObjectiveActor::GetReturnToFrontendDelaySeconds() const
{
	return FMath::Max(ReturnToFrontendDelaySeconds, 0.f);
}

void AAGASSObjectiveActor::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	const int32 OtherBodyIndex,
	const bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || bObjectiveConsumed)
	{
		return;
	}

	AController* CompletingController = nullptr;
	if (!ResolveCompletingController(OtherActor, CompletingController))
	{
		return;
	}

	AGameModeBase* const AuthGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode() : nullptr;
	IAGASSObjectiveRuntimeInterface* const ObjectiveRuntime = Cast<IAGASSObjectiveRuntimeInterface>(AuthGameMode);
	if (ObjectiveRuntime == nullptr || !ObjectiveRuntime->HandleAGASSObjectiveCompleted(CompletingController, this))
	{
		return;
	}

	bObjectiveConsumed = true;
	ObjectiveTriggerVolume->SetGenerateOverlapEvents(false);
	ObjectiveTriggerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool AAGASSObjectiveActor::ResolveCompletingController(AActor* OtherActor, AController*& OutCompletingController) const
{
	OutCompletingController = nullptr;

	const APawn* const OverlappingPawn = Cast<APawn>(OtherActor);
	if (OverlappingPawn == nullptr)
	{
		return false;
	}

	OutCompletingController = OverlappingPawn->GetController();
	return OutCompletingController != nullptr;
}

void AAGASSObjectiveActor::RefreshObjectivePresentation()
{
	if (ObjectiveTriggerVolume == nullptr || ObjectiveMeshComponent == nullptr)
	{
		return;
	}

	ObjectiveTriggerVolume->SetBoxExtent(TriggerExtent.ComponentMax(FVector(25.f, 25.f, 25.f)));
	ObjectiveMeshComponent->SetRelativeLocation(MeshRelativeOffset);
	ObjectiveMeshComponent->SetRelativeScale3D(MeshRelativeScale);

	UStaticMesh* const ResolvedObjectiveMesh = ObjectiveMesh.LoadSynchronous();
	ObjectiveMeshComponent->SetStaticMesh(ResolvedObjectiveMesh != nullptr ? ResolvedObjectiveMesh : FallbackObjectiveMesh.Get());
}
