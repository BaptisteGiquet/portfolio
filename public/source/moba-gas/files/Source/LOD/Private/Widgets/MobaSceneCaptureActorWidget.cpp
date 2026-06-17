

#include "MobaSceneCaptureActorWidget.h"

#include "SceneCapture/MobaSceneCaptureActor.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/TextureRenderTarget2D.h"

void UMobaSceneCaptureActorWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	SizeBox_Render->SetWidthOverride(RenderSize.X);
	SizeBox_Render->SetHeightOverride(RenderSize.Y);
	
}


void UMobaSceneCaptureActorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SpawnSceneCaptureActor();
	ConfigureSceneCaptureActor();
	BeginRenderScene();
}


void UMobaSceneCaptureActorWidget::BeginDestroy()
{
	Super::BeginDestroy();

	StopRenderScene();
}


void UMobaSceneCaptureActorWidget::ConfigureSceneCaptureActor()
{
	if (!GetSceneCaptureActor())
	{
		UE_LOG(LogTemp, Warning, TEXT("No RenderActor"))
		return;
	}

	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->InitAutoFormat(RenderSize.X, RenderSize.Y);
	RenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
	GetSceneCaptureActor()->SetRenderTarget(RenderTarget);

	UMaterialInstanceDynamic* DisplayImageDynamicMaterial = Image_Display->GetDynamicMaterial();
	if (DisplayImageDynamicMaterial)
	{
		DisplayImageDynamicMaterial->SetTextureParameterValue(DisplayImageRenderTargetParamName, RenderTarget);
	}
}


void UMobaSceneCaptureActorWidget::BeginRenderScene()
{
	RenderTickFrequency = 1.f / RenderFrameRate;

	const UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(RenderTimerHandle, this, &ThisClass::UpdateSceneCapture, RenderTickFrequency, true);
	}
}


void UMobaSceneCaptureActorWidget::UpdateSceneCapture()
{
	if (GetSceneCaptureActor())
	{
		GetSceneCaptureActor()->UpdateSceneCapture();
	}
}


void UMobaSceneCaptureActorWidget::StopRenderScene()
{
	const UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(RenderTimerHandle);
	}
}
