
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MobaSceneCaptureActor.generated.h"

class USceneCaptureComponent2D;

UCLASS()
class LOD_API AMobaSceneCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	AMobaSceneCaptureActor();

	void SetRenderTarget(UTextureRenderTarget2D* InRenderTarget);
	void UpdateSceneCapture();

	USceneCaptureComponent2D* GetCaptureComponent() const { return CaptureComponent; }

protected:
	virtual void BeginPlay() override;


private:
	UPROPERTY(VisibleDefaultsOnly, Category = "Render Actor")
	TObjectPtr<USceneComponent> RootComp;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Render Actor")
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent;
};
