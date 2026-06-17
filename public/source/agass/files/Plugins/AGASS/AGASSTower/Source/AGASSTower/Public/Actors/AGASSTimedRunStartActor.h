#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/AGASSInteractableInterface.h"
#include "AGASSTimedRunStartActor.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class AGASSTOWER_API AAGASSTimedRunStartActor : public AActor, public IAGASSInteractableInterface
{
	GENERATED_BODY()

public:
	AAGASSTimedRunStartActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanAGASSInteract_Implementation(AController* InteractingController) const override;
	virtual float GetAGASSInteractHoldDurationSeconds_Implementation(AController* InteractingController) const override;
	virtual void HandleAGASSInteract_Implementation(AController* InteractingController) override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void RefreshPresentation();

	UPROPERTY(VisibleDefaultsOnly, Category = "AGASS|Timed Run")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleDefaultsOnly, Category = "AGASS|Timed Run")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, Category = "AGASS|Timed Run")
	TSoftObjectPtr<UStaticMesh> DisplayMesh;

	UPROPERTY(EditAnywhere, Category = "AGASS|Timed Run")
	FVector MeshScale = FVector(1.4f, 1.4f, 2.0f);

	UPROPERTY(EditAnywhere, Category = "AGASS|Timed Run")
	bool bDisableAfterSuccessfulStart = true;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FallbackMesh;

	UPROPERTY(Replicated)
	bool bHasStartedRun = false;
};
