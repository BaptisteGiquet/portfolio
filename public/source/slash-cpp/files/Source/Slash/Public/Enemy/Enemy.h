#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Characters/CharacterTypes.h"
#include "Enemy.generated.h"

class ASoul;
class UPawnSensingComponent;
class UHealthBarWidgetComponent;
class AAIController;



UCLASS()
class SLASH_API AEnemy : public ABaseCharacter
{
	GENERATED_BODY()

public:
	AEnemy();
	/* <AActor> */
	virtual void Tick(float DeltaTime) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Destroyed() override;
	
	/* <IHitInterface> */
	virtual void GetHit_Implementation(const FVector& InImpactPoint, AActor* Hitter) override;
	

protected:
	/* <AActor> */
	virtual void BeginPlay() override;

	/* <ABaseCharacter> */
	virtual bool CanAttack() override;
	virtual void Attack() override;
	virtual int32 PlayAttackMontage() override;
	virtual void OnAttackEnd() override;
	virtual void HandleDamage(float DamageAmount) override;
	virtual void Die() override;
	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EEnemyStates EnemyState = EEnemyStates::EES_Patrolling;

	
private:
	/* AI Behavior */
	void SpawnSoul();
	void InitializeEnemy();
	void CheckPatrolTarget();
	void CheckCombatTarget();
	void SetHealthBarVisibility(bool InBool);
	void LoseInterest();
	void StartPatrolling();
	void ChaseTarget();
	void ClearPatrolTimer();
	void StartAttackTimer();
	void ClearAttackTimer();
	void PatrolTimerFinished();
	bool IsOutsideAttackRadius();
	bool IsOutsideCombatRadius();
	bool IsChasing();
	bool IsInsideAttackRadius();
	bool IsAttacking();
	bool IsDead();
	bool IsEngaged();
	void MoveToTarget(AActor* InTarget);
	AActor* ChooseNewPatrolTarget();
	bool IsInTargetRange(AActor* Target, double AcceptanceRadius);
	void SpawnDefaultWeapon();

	UFUNCTION()
	void PawnSeen(APawn* PawnSeen); // Callback for OwnPawnSeen in UPawnSensing
	
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AWeapon> WeaponClass;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<ASoul> SoulClass;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UHealthBarWidgetComponent> HealthBarWidget;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPawnSensingComponent> PawnSensingComponent;
	
	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	TObjectPtr<AActor> CurrentPatrolTarget;
	
	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	TArray<TObjectPtr<AActor>> AllPatrolTargets;

	UPROPERTY(VisibleAnywhere, Category = "AI Navigation")
	TObjectPtr<AAIController> EnemyController;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	double CombatRadius = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	double AttackRadius = 150.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	double AcceptanceMoveToRadius = 20.f;

	FTimerHandle AttackTimer;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackTimerMin = 0.5f;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackTimerMax = 1.5;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float ChasingSpeed = 300.f;
	
	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	double PatrolStopRadius = 200.f;
	
	UPROPERTY(VisibleAnywhere)
	FTimerHandle PatrolTimer;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float PatrolWaitMax = 5.f;
	
	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float PatrolWaitMin = 3.f;

	UPROPERTY(EditAnywhere, Category = "AI Navigation")
	float PatrollingSpeed = 150.f;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float DeathLifeSpan = 6.f;
	
	
};
