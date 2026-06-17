#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGASSMadPlaneAircraftActor.generated.h"

class USceneComponent;
class UAudioComponent;
class USoundBase;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class AGASSTOWER_API AAGASSMadPlaneAircraftActor : public AActor
{
	GENERATED_BODY()

public:
	AAGASSMadPlaneAircraftActor();

	void ApplyReplicatedPlaneTransform(const FTransform& NewTransform);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Plane")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Plane")
	TObjectPtr<UStaticMeshComponent> AircraftMeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Plane|Audio")
	TObjectPtr<UAudioComponent> MovingAudioComponent;

	UPROPERTY(EditAnywhere, Category = "AGASS|Mad Plane|Audio")
	TObjectPtr<USoundBase> MovingLoopSound;

	void InitializeLoopAudioComponent() const;
};
