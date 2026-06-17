#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "EMRPlayerAnimInstance.generated.h"

class UCharacterMovementComponent;
class AEMRPlayerCharacter;
class UAnimSequenceBase;
/**
 * 
 */
UCLASS()
class LOD_API UEMRPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UEMRPlayerAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool IsMoving;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool IsFalling;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool IsSeated;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	FGameplayTag SeatedAnimationTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
    TObjectPtr<UAnimSequenceBase> DefaultSeatedAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TMap<FGameplayTag, TObjectPtr<UAnimSequenceBase>> SeatedAnimationMap;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> ActiveSeatedAnimation = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bLookingAtWatch = false;





	
    UPROPERTY(BlueprintReadOnly, Category = "Character")
    AEMRPlayerCharacter* PlayerCharacter;

    UPROPERTY(BlueprintReadOnly, Category = "Character")
    UCharacterMovementComponent* CharacterMovementComponent;

    UPROPERTY(BlueprintReadOnly, Category = "Character")
    FVector LookTargetLocation;

};
