#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AGASSCharacterAnimInstance.generated.h"

class AAGASSCharacter;

UCLASS(Transient, Blueprintable)
class AGASS_API UAGASSCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "AGASS|Animation")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "AGASS|Animation")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "AGASS|Animation")
	bool bHasCarriedUsableItem = false;

	UPROPERTY(BlueprintReadOnly, Category = "AGASS|Animation")
	FName CarriedUsableAnimationId;

	UPROPERTY(BlueprintReadOnly, Category = "AGASS|Animation")
	TObjectPtr<AAGASSCharacter> AGASSCharacter = nullptr;
};
