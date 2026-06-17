#include "Actors/AGASSMadPlaneAircraftActor.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AAGASSMadPlaneAircraftActor::AAGASSMadPlaneAircraftActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.f);
	SetMinNetUpdateFrequency(10.f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	AircraftMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AircraftMesh"));
	AircraftMeshComponent->SetupAttachment(SceneRoot);
	AircraftMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AircraftMeshComponent->SetGenerateOverlapEvents(false);
	AircraftMeshComponent->SetCanEverAffectNavigation(false);
	AircraftMeshComponent->SetSimulatePhysics(false);
	AircraftMeshComponent->SetEnableGravity(false);

	MovingAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MovingAudio"));
	MovingAudioComponent->SetupAttachment(SceneRoot);
	MovingAudioComponent->bAutoActivate = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		AircraftMeshComponent->SetStaticMesh(CubeMesh.Object);
		AircraftMeshComponent->SetRelativeScale3D(FVector(3.f, 1.2f, 0.4f));
	}
}

void AAGASSMadPlaneAircraftActor::ApplyReplicatedPlaneTransform(const FTransform& NewTransform)
{
	SetActorTransform(NewTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void AAGASSMadPlaneAircraftActor::BeginPlay()
{
	Super::BeginPlay();

	InitializeLoopAudioComponent();
}

void AAGASSMadPlaneAircraftActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MovingAudioComponent != nullptr)
	{
		MovingAudioComponent->Stop();
	}

	Super::EndPlay(EndPlayReason);
}

void AAGASSMadPlaneAircraftActor::InitializeLoopAudioComponent() const
{
	if (MovingAudioComponent == nullptr || MovingLoopSound == nullptr || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	MovingAudioComponent->SetSound(MovingLoopSound);
	MovingAudioComponent->Play();
}
