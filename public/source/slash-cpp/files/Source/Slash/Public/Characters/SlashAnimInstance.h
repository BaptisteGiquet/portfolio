// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterTypes.h"
#include "SlashAnimInstance.generated.h"

class ASlashMyCharacter;
class UCharacterMovementComponent;
/**
 * 
 */
UCLASS()
class SLASH_API USlashAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	USlashAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool IsMoving;
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool IsFalling;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	ASlashMyCharacter* SlashCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	UCharacterMovementComponent* CharacterMovementComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	EEquipmentState EquipmentState;

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	ECharacterState CharacterState;

};
