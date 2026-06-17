// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "Bird.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UCapsuleComponent;
class USkeletalMeshComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class SLASH_API ABird : public APawn
{
	GENERATED_BODY()

public:
	ABird();
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* BirdInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input) 
	UInputAction* LookAction;

	
	void Look(const FInputActionValue& Value);

	void Move(const FInputActionValue& Value);
	
private:
	UPROPERTY(VisibleAnywhere)
	UCapsuleComponent* Capsule;

	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* BirdSkeletalMesh;

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* BirdSpringArm;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* BirdCamera;
};
