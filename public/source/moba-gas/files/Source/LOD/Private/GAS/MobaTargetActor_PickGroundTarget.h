
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "MobaTargetActor_PickGroundTarget.generated.h"


UCLASS()
class AMobaTargetActor_PickGroundTarget : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:
	AMobaTargetActor_PickGroundTarget();
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void ConfirmTargetingAndContinue() override;
	
	void SetTargetingAreaRadius(const float NewRadius);
	void SetShouldDrawDebug(const bool bDrawDebug) { bShouldDrawDebug = bDrawDebug; }
	void SetTargetingMaxRange(const float MaxRange) { TargetingMaxRange = MaxRange; }
	void SetShouldTargetFriendly(const bool bTargetFriendly) { bShouldTargetFriendly = bTargetFriendly; }
	void SetShouldTargetHostile(const bool bTargetHostile) { bShouldTargetHostile = bTargetHostile; }
	
private:
	FVector GetTargetPoint() const;

	UPROPERTY(VisibleDefaultsOnly, Category = "Visual")
	TObjectPtr<UDecalComponent> DecalComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Targeting")
	float TargetingAreaRadius = 300.f;
	
	UPROPERTY(VisibleAnywhere, Category = "Targeting")
	float TargetingMaxRange = 2000.f;

	UPROPERTY(VisibleAnywhere, Category = "Targeting")
	bool bShouldTargetHostile = true;
	
	UPROPERTY(VisibleAnywhere, Category = "Targeting")
	bool bShouldTargetFriendly = false;

	UPROPERTY(VisibleAnywhere, Category = "Targeting")
	bool bShouldDrawDebug = false;
};
