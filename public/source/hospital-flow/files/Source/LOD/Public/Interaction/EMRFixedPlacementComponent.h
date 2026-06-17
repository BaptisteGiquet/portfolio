#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "EMRFixedPlacementComponent.generated.h"

UCLASS(ClassGroup = (EMR), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRFixedPlacementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEMRFixedPlacementComponent();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "EMR|Interaction")
	void SetAnchorTransform(const FTransform& InTransform);

	UFUNCTION(BlueprintPure, Category = "EMR|Interaction")
	bool HasAnchor() const { return bHasAnchor; }

	UFUNCTION(BlueprintPure, Category = "EMR|Interaction")
	FTransform GetAnchorTransform() const { return AnchorTransform; }

private:
	UPROPERTY(Replicated)
	FTransform AnchorTransform = FTransform::Identity;

	UPROPERTY(Replicated)
	bool bHasAnchor = false;
};
