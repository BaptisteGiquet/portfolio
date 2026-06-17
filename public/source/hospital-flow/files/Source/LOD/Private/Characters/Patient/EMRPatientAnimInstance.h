#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "EMRPatientAnimInstance.generated.h"

class UCharacterMovementComponent;
class AEMRPatient;
class UAnimSequenceBase;
/**
 * 
 */
UCLASS()
class LOD_API UEMRPatientAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	
	UEMRPatientAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed;
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
	
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	AEMRPatient* PatientCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	UCharacterMovementComponent* CharacterMovementComponent;
};
