#include "Actors/AGASSMadCopsCarActor.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AAGASSMadCopsCarActor::AAGASSMadCopsCarActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CarMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarMesh"));
	CarMeshComponent->SetupAttachment(SceneRoot);
	CarMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	CarMeshComponent->SetGenerateOverlapEvents(false);
	CarMeshComponent->SetCanEverAffectNavigation(false);
	CarMeshComponent->SetSimulatePhysics(false);

	SirenAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("SirenAudio"));
	SirenAudioComponent->SetupAttachment(SceneRoot);
	SirenAudioComponent->bAutoActivate = false;

	DrivingAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("DrivingAudio"));
	DrivingAudioComponent->SetupAttachment(SceneRoot);
	DrivingAudioComponent->bAutoActivate = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		CarMeshComponent->SetStaticMesh(CubeMesh.Object);
		CarMeshComponent->SetRelativeScale3D(FVector(2.2f, 1.1f, 0.8f));
	}
}

void AAGASSMadCopsCarActor::ApplyReplicatedCarTransform(const FTransform& NewTransform)
{
	SetActorTransform(NewTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void AAGASSMadCopsCarActor::BeginPlay()
{
	Super::BeginPlay();

	InitializeLoopAudioComponent(SirenAudioComponent, SirenSound);
	InitializeLoopAudioComponent(DrivingAudioComponent, DrivingLoopSound);
}

void AAGASSMadCopsCarActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SirenAudioComponent != nullptr)
	{
		SirenAudioComponent->Stop();
	}

	if (DrivingAudioComponent != nullptr)
	{
		DrivingAudioComponent->Stop();
	}

	Super::EndPlay(EndPlayReason);
}

void AAGASSMadCopsCarActor::InitializeLoopAudioComponent(UAudioComponent* AudioComponent, USoundBase* Sound) const
{
	if (AudioComponent == nullptr || Sound == nullptr || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	AudioComponent->SetSound(Sound);
	AudioComponent->Play();
}
