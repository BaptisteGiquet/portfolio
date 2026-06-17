#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGASSObjectiveActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class AGASSTOWER_API AAGASSObjectiveActor : public AActor
{
	GENERATED_BODY()

public:
	AAGASSObjectiveActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	FText GetObjectiveNameText() const;
	FText GetObjectiveDescriptionText() const;
	FText GetCompletionTitleText() const;
	FText GetCompletionBodyText() const;
	FText GetReturnToFrontendReasonText() const;
	float GetReturnToFrontendDelaySeconds() const;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	bool ResolveCompletingController(AActor* OtherActor, AController*& OutCompletingController) const;
	void RefreshObjectivePresentation();

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Objective")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Objective")
	TObjectPtr<UStaticMeshComponent> ObjectiveMeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Objective")
	TObjectPtr<UBoxComponent> ObjectiveTriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Objective", meta = (AllowPrivateAccess = "true"))
	FText ObjectiveName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Objective", meta = (AllowPrivateAccess = "true"))
	FText ObjectiveDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Presentation", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UStaticMesh> ObjectiveMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Objective", meta = (AllowPrivateAccess = "true"))
	FVector TriggerExtent = FVector(160.f, 160.f, 180.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Presentation", meta = (AllowPrivateAccess = "true"))
	FVector MeshRelativeOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Presentation", meta = (AllowPrivateAccess = "true"))
	FVector MeshRelativeScale = FVector(1.5f, 1.5f, 1.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Flow", meta = (AllowPrivateAccess = "true"))
	FText CompletionTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Flow", meta = (AllowPrivateAccess = "true"))
	FText CompletionBody;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Flow", meta = (AllowPrivateAccess = "true"))
	FText ReturnToFrontendReason;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Flow", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ReturnToFrontendDelaySeconds = 8.f;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FallbackObjectiveMesh;

	bool bObjectiveConsumed = false;
};
