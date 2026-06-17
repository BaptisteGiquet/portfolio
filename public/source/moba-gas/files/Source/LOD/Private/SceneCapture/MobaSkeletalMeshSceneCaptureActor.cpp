

#include "SceneCapture/MobaSkeletalMeshSceneCaptureActor.h"


AMobaSkeletalMeshSceneCaptureActor::AMobaSkeletalMeshSceneCaptureActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(GetRootComponent());
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetLightingChannels(false, true, false);
	
}


void AMobaSkeletalMeshSceneCaptureActor::ConfigureSkeletalMesh(USkeletalMesh* MeshAsset, TSubclassOf<UAnimInstance> AnimBlueprint)
{
	if (!MeshComponent) { return; }

	MeshComponent->SetSkeletalMeshAsset(MeshAsset);

		MeshComponent->SetAnimInstanceClass(AnimBlueprint);
}


void AMobaSkeletalMeshSceneCaptureActor::BeginPlay()
{
	Super::BeginPlay();

	// The actor won't be visible by gameplay Camera
	MeshComponent->SetVisibleInSceneCaptureOnly(true);
	MeshComponent->SetCastShadow(false);
	MeshComponent->bCastDynamicShadow = false;
	MeshComponent->bCastStaticShadow = false;

	MeshComponent->SetForcedLOD(2);
}



