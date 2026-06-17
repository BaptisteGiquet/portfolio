#include "Actors/AGASSTimedRunStartActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "Interfaces/AGASSTimedRunRuntimeInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAGASSTimedRunStartActor::AAGASSTimedRunStartActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCanEverAffectNavigation(false);
	MeshComponent->SetSimulatePhysics(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		FallbackMesh = CubeMesh.Object;
		MeshComponent->SetStaticMesh(CubeMesh.Object);
	}
}

bool AAGASSTimedRunStartActor::CanAGASSInteract_Implementation(AController* InteractingController) const
{
	return HasAuthority() && !bHasStartedRun && InteractingController != nullptr;
}

float AAGASSTimedRunStartActor::GetAGASSInteractHoldDurationSeconds_Implementation(AController* InteractingController) const
{
	return 0.f;
}

void AAGASSTimedRunStartActor::HandleAGASSInteract_Implementation(AController* InteractingController)
{
	if (!HasAuthority() || bHasStartedRun || InteractingController == nullptr)
	{
		return;
	}

	AGameModeBase* const AuthGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode() : nullptr;
	IAGASSTimedRunRuntimeInterface* const TimedRunRuntime = Cast<IAGASSTimedRunRuntimeInterface>(AuthGameMode);
	if (TimedRunRuntime == nullptr || !TimedRunRuntime->HandleAGASSTimedRunStarted(InteractingController, this))
	{
		return;
	}

	bHasStartedRun = true;
	if (bDisableAfterSuccessfulStart && MeshComponent != nullptr)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AAGASSTimedRunStartActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPresentation();
}

void AAGASSTimedRunStartActor::RefreshPresentation()
{
	if (MeshComponent == nullptr)
	{
		return;
	}

	UStaticMesh* const ResolvedMesh = DisplayMesh.LoadSynchronous();
	MeshComponent->SetStaticMesh(ResolvedMesh != nullptr ? ResolvedMesh : FallbackMesh.Get());
	MeshComponent->SetRelativeScale3D(MeshScale);
}

void AAGASSTimedRunStartActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, bHasStartedRun);
}
