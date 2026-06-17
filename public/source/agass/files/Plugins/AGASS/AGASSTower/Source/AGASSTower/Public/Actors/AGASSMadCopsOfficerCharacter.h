#pragma once

#include "CoreMinimal.h"
#include "Interfaces/AGASSInteractableInterface.h"
#include "GameFramework/Character.h"
#include "AGASSMadCopsOfficerCharacter.generated.h"

class AAGASSMadCopsSessionEventActor;
class USoundBase;

UCLASS(Blueprintable)
class AGASSTOWER_API AAGASSMadCopsOfficerCharacter : public ACharacter, public IAGASSInteractableInterface
{
	GENERATED_BODY()

public:
	AAGASSMadCopsOfficerCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetOwningMadCopsEvent(AAGASSMadCopsSessionEventActor* InOwningEvent);

	virtual bool CanAGASSInteract_Implementation(AController* InteractingController) const override;
	virtual float GetAGASSInteractHoldDurationSeconds_Implementation(AController* InteractingController) const override;
	virtual void HandleAGASSInteract_Implementation(AController* InteractingController) override;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayBribeAcceptedSound(FVector_NetQuantize SoundLocation);

private:
	UPROPERTY(Replicated)
	TObjectPtr<AAGASSMadCopsSessionEventActor> OwningMadCopsEvent;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Mad Cops", meta = (ClampMin = "0.0"))
	float InteractionDistance = 350.f;

	UPROPERTY(EditAnywhere, Category = "AGASS|Mad Cops|Audio")
	TObjectPtr<USoundBase> BribeAcceptedSound;
};
