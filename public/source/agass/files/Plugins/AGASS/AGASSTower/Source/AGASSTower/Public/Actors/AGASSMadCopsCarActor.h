#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGASSMadCopsCarActor.generated.h"

class USceneComponent;
class UAudioComponent;
class USoundBase;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class AGASSTOWER_API AAGASSMadCopsCarActor : public AActor
{
	GENERATED_BODY()

public:
	AAGASSMadCopsCarActor();

	void ApplyReplicatedCarTransform(const FTransform& NewTransform);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Cops")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Cops")
	TObjectPtr<UStaticMeshComponent> CarMeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Cops|Audio")
	TObjectPtr<UAudioComponent> SirenAudioComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Cops|Audio")
	TObjectPtr<UAudioComponent> DrivingAudioComponent;

	UPROPERTY(EditAnywhere, Category = "AGASS|Mad Cops|Audio")
	TObjectPtr<USoundBase> SirenSound;

	UPROPERTY(EditAnywhere, Category = "AGASS|Mad Cops|Audio")
	TObjectPtr<USoundBase> DrivingLoopSound;

	void InitializeLoopAudioComponent(UAudioComponent* AudioComponent, USoundBase* Sound) const;
};
