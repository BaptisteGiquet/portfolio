
#pragma once

#include "CoreMinimal.h"
#include "MobaSceneCaptureActorWidget.h"
#include "MobaSkeletalMeshSceneCaptureWidget.generated.h"


class AMobaSkeletalMeshSceneCaptureActor;

UCLASS()
class LOD_API UMobaSkeletalMeshSceneCaptureWidget : public UMobaSceneCaptureActorWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void SpawnSceneCaptureActor() override;
	virtual AMobaSceneCaptureActor* GetSceneCaptureActor() const override;


private:
	UPROPERTY(EditDefaultsOnly, Category = "SkeletalMesh Capture")
	TSubclassOf<AMobaSkeletalMeshSceneCaptureActor> SkeletalMeshSceneCaptureClass;

	UPROPERTY()
	TObjectPtr<AMobaSkeletalMeshSceneCaptureActor> SkeletalMeshSceneCaptureActor;
};
