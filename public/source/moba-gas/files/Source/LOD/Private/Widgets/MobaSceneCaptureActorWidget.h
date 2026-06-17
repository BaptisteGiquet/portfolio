
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MobaSceneCaptureActorWidget.generated.h"


class AMobaSceneCaptureActor;
class USizeBox;
class UImage;

UCLASS(Abstract)
class LOD_API UMobaSceneCaptureActorWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void BeginDestroy() override;

	
	
private:
	virtual void SpawnSceneCaptureActor() PURE_VIRTUAL(UMobaRenderActorWidget::SpawnSceneCaptureActor, );
	virtual AMobaSceneCaptureActor* GetSceneCaptureActor() const PURE_VIRTUAL(UMobaRenderActorWidget::GetSceneCaptureActor, return nullptr; );

	void ConfigureSceneCaptureActor();

	void BeginRenderScene();
	void UpdateSceneCapture();
	void StopRenderScene();
	
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Display;

	UPROPERTY(meta = (BindWidget))
	USizeBox* SizeBox_Render;

	UPROPERTY(EditDefaultsOnly, Category = "Render Actor")
	FName DisplayImageRenderTargetParamName = TEXT("RenderTarget");

	UPROPERTY(EditDefaultsOnly, Category = "Render Actor")
	FVector2D RenderSize = FVector2D(150.f, 180.f);

	UPROPERTY(EditDefaultsOnly, Category = "Render Actor")
	int32 RenderFrameRate = 24;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RenderTarget = nullptr;
	
	float RenderTickFrequency = 0.f;
	FTimerHandle RenderTimerHandle;
	
};
