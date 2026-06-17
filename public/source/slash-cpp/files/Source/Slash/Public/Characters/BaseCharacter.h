#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/HitInterface.h"
#include "BaseCharacter.generated.h"

enum class EDeadPose : uint8;
class UAttributeComponent;
class AWeapon;

UCLASS()
class SLASH_API ABaseCharacter : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	virtual void Tick(float DeltaTime) override;


protected:
	virtual void BeginPlay() override;
	virtual void GetHit_Implementation(const FVector& InImpactPoint, AActor* Hitter) override;

	UFUNCTION(BlueprintCallable)
	void SetWeaponCollisionEnabled(ECollisionEnabled::Type InCollision);
	
	virtual bool CanAttack();
	virtual void Attack();
	virtual int32 PlayAttackMontage();
	virtual void HandleDamage(float DamageAmount);
	void HandleHitReactDirection(const FVector& InImpactPoint);
	void PlayHitReactMontage(const FName& SectionName);
	void PlayHitSound(const FVector& InImpactPoint);
	void SpawnHitParticles(const FVector& InImpactPoint);
	virtual void Die();
	virtual int32 PlayDeathMontage();
	virtual void PlayDodgeMontage();
	void DisableCapsuleCollision();
	void DisableMeshCollision();
	bool IsAlive();
	void StopAttackMontage();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EDeadPose DeadPose;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> CombatTarget;
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void OnAttackEnd();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void OnDodgeEnd();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FVector GetTranslationWarpTarget();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FVector GetRotationWarpTarget();

	UPROPERTY(EditAnywhere, Category = "Combat")
	double WarpTargetDistance = 75.f;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	AWeapon* EquippedWeapon;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAttributeComponent* AttributeComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Montages")
	UAnimMontage* AttackMontage;

private:
	void PlayMontageSection(UAnimMontage* AnimMontage, const FName& SectionName);
	int32 PlayRandomMontageSection(UAnimMontage* AnimMontage);

	UPROPERTY(EditAnywhere, Category = "Combat")
	USoundBase* HitReactSound;

	UPROPERTY(EditAnywhere, Category = "Combat")
	UParticleSystem* HitReactParticle;
	
	UPROPERTY(EditDefaultsOnly, Category = "AnimMontage")
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditDefaultsOnly, Category = "AnimMontage")
	UAnimMontage* DeathMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "AnimMontage")
	TObjectPtr<UAnimMontage> DodgeMontage;
};


