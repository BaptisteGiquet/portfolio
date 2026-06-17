

#include "MobaSceneCaptureActor.h"

#include "Components/SceneCaptureComponent2D.h"


AMobaSceneCaptureActor::AMobaSceneCaptureActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	SetRootComponent(RootComp);
	
	CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
	CaptureComponent->SetupAttachment(GetRootComponent());

	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->PostProcessBlendWeight = 0.f;
	CaptureComponent->ShowFlags.SetTemporalAA(false);
	CaptureComponent->ShowFlags.SetMotionBlur(false);
	CaptureComponent->FOVAngle = 30.f;
}


void AMobaSceneCaptureActor::BeginPlay()
{
	Super::BeginPlay();
	
	CaptureComponent->ShowOnlyActorComponents(this);

	// Put the actor really far so it doesn't interact with the game
	SetActorLocation(FVector(0.f, 0.f, 100000.f));
}

// Configures where the captured image will be rendered
void AMobaSceneCaptureActor::SetRenderTarget(UTextureRenderTarget2D* InRenderTarget)
{
	CaptureComponent->TextureTarget = InRenderTarget;
}

// Captures the frame to render
void AMobaSceneCaptureActor::UpdateSceneCapture()
{
	if (CaptureComponent)
	{
		CaptureComponent->CaptureScene();
	}
}


