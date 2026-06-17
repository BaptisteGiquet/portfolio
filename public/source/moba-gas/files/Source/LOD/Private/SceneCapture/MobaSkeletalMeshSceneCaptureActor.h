
#pragma once

#include "CoreMinimal.h"
#include "MobaSceneCaptureActor.h"
#include "MobaSkeletalMeshSceneCaptureActor.generated.h"

UCLASS()
class LOD_API AMobaSkeletalMeshSceneCaptureActor : public AMobaSceneCaptureActor
{
	GENERATED_BODY()

public:
	AMobaSkeletalMeshSceneCaptureActor();

	void ConfigureSkeletalMesh(USkeletalMesh* MeshAsset, TSubclassOf<UAnimInstance> AnimBlueprint);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "SkeletalMesh Render")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;
};
