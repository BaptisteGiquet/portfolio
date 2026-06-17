// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Weapon.generated.h"

/**
 * 
 */
UCLASS()
class SLASH_API AWeapon : public AItem
{
	GENERATED_BODY()

public:
	AWeapon();
	
	void AttachMeshToSocket(USceneComponent* InParent, FName InSocketName);
	void PlayEquipSound();
	void DisableSphereDetectionCollision();
	void DeactivateVisualEffect();
	void Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator);

	UPROPERTY(VisibleInstanceOnly)
	TArray<AActor*> ActorsToIgnore;
	
protected:
	
	//UFUNCTION() not needed on override function that already had it
	virtual void BeginPlay() override;
	void ExecuteGetHit(FHitResult BoxHit);
	bool IsActorSameType(AActor* OtherActor);

	virtual void OnHitBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	UFUNCTION(BlueprintImplementableEvent)
	void CreateFields(const FVector& FieldLocation);

	
	UPROPERTY(VisibleAnywhere)
	USceneComponent* HitBoxTraceStart;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* HitBoxTraceEnd;

	
private:

	void BoxTrace(FHitResult& BoxHit);

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	FVector BoxTraceExtent = FVector(5.f);

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	bool bShowBoxDebug = false;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	USoundBase* EquipSound;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float BaseDamage = 10.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	TSubclassOf<UDamageType> DamageType = UDamageType::StaticClass();
};
