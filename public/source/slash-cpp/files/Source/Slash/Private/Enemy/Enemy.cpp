
#include "Enemy/Enemy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/HealthBarWidgetComponent.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/AttributeComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Items/Soul.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/PawnSensingComponent.h"



AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	GetMesh()->SetCollisionObjectType(ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);
	GetMesh()->SetGenerateOverlapEvents(true);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	
	HealthBarWidget = CreateDefaultSubobject<UHealthBarWidgetComponent>(TEXT("HealthBarWidgetComponent"));
	HealthBarWidget->SetupAttachment(GetRootComponent());

	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComponent"));
	PawnSensingComponent->SightRadius = 1000.f;
	PawnSensingComponent->SetPeripheralVisionAngle(45.f);
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDead()) return;
	if (EnemyState == EEnemyStates::EES_Patrolling)
	{
		CheckPatrolTarget();
	}
	else
	{
		CheckCombatTarget();
	}
}

float AEnemy::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	HandleDamage(DamageAmount);
	CombatTarget = EventInstigator->GetPawn();
	if (IsInsideAttackRadius())
	{
		EnemyState = EEnemyStates::EES_Attacking;
	}
	else if (IsOutsideAttackRadius())
	{
		ChaseTarget();	
	}
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AEnemy::Destroyed()
{
	//Called when Enemy is destroyed
	Super::Destroyed();
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
	}
}

void AEnemy::GetHit_Implementation(const FVector& InImpactPoint, AActor* Hitter)
{
	Super::GetHit_Implementation(InImpactPoint, Hitter);
	ClearAttackTimer();
	ClearPatrolTimer();
	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
	StopAttackMontage();
	if (!IsDead()) SetHealthBarVisibility(true);
}


void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	if (PawnSensingComponent) PawnSensingComponent->OnSeePawn.AddDynamic(this, &AEnemy::PawnSeen);
	InitializeEnemy();
	Tags.Add(FName("Enemy"));
}

bool AEnemy::CanAttack()
{
	return IsInsideAttackRadius() && !IsAttacking()	&& !IsDead() && !IsEngaged();
}

void AEnemy::Attack()
{
	Super::Attack();
	if (CombatTarget && AttackMontage)
	{
		EnemyState = EEnemyStates::EES_Engaged;
		PlayAttackMontage();
	}
}

int32 AEnemy::PlayAttackMontage()
{
	return Super::PlayAttackMontage();
}

void AEnemy::OnAttackEnd()
{
	Super::OnAttackEnd();
	EnemyState = EEnemyStates::EES_NoState;
	CheckCombatTarget();
}

void AEnemy::HandleDamage(float DamageAmount)
{
	Super::HandleDamage(DamageAmount);
	if (AttributeComponent && HealthBarWidget)
	{
		float NewHealthPercent = AttributeComponent->GetHealthPercent();
		HealthBarWidget->SetHealthPercent(NewHealthPercent);
	}
}

void AEnemy::Die()
{
	Super::Die();
	EnemyState = EEnemyStates::EES_Dead;
	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
	ClearAttackTimer();
	ClearPatrolTimer();
	DisableCapsuleCollision();
	SetLifeSpan(DeathLifeSpan);
	SetHealthBarVisibility(false);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	SpawnSoul();
}

void AEnemy::SpawnSoul()
{
	UWorld* World = GetWorld();
	if (World && SoulClass && AttributeComponent)
	{
		ASoul* SpawnedSoul = World->SpawnActorDeferred<ASoul>(SoulClass, GetActorTransform(), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (SpawnedSoul)
		{
			SpawnedSoul->SetSoulsAmount(AttributeComponent->GetSouls());
			UGameplayStatics::FinishSpawningActor(SpawnedSoul, GetActorTransform());
		}

	}
}

void AEnemy::InitializeEnemy()
{
	EnemyController = Cast<AAIController>(GetController());
	SetHealthBarVisibility(false);
	GetCharacterMovement()->MaxWalkSpeed = PatrollingSpeed;
	if (CurrentPatrolTarget) MoveToTarget(CurrentPatrolTarget);
	SpawnDefaultWeapon();
}

void AEnemy::CheckPatrolTarget()
{
	if (IsInTargetRange(CurrentPatrolTarget, PatrolStopRadius))
	{
		CurrentPatrolTarget = ChooseNewPatrolTarget();
		const float PatrolWaitTime = FMath::RandRange(PatrolWaitMin, PatrolWaitMax);
		GetWorldTimerManager().SetTimer(PatrolTimer, this, &AEnemy::PatrolTimerFinished, PatrolWaitTime);
	}
}

void AEnemy::CheckCombatTarget()
{
	if (IsOutsideCombatRadius())
	{
		LoseInterest();
		ClearAttackTimer();
		if (!IsEngaged()) StartPatrolling();	
	}
	else if (IsOutsideAttackRadius() && !IsChasing())
	{
		ClearAttackTimer();
		if (!IsEngaged()) ChaseTarget();
	}
	else if (CanAttack())
	{
		StartAttackTimer();
	}
}

void AEnemy::SetHealthBarVisibility(bool InBool)
{
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(InBool);
	}
}

void AEnemy::LoseInterest()
{
	CombatTarget = nullptr;
	SetHealthBarVisibility(false);
}

void AEnemy::StartPatrolling()
{
	EnemyState = EEnemyStates::EES_Patrolling;
	GetCharacterMovement()->MaxWalkSpeed = 125.f;
	MoveToTarget(CurrentPatrolTarget);
}

void AEnemy::ChaseTarget()
{
	EnemyState = EEnemyStates::EES_Chasing;
	GetCharacterMovement()->MaxWalkSpeed = ChasingSpeed;
	MoveToTarget(CombatTarget);
}

void AEnemy::ClearPatrolTimer()
{
	GetWorldTimerManager().ClearTimer(PatrolTimer);
}

void AEnemy::StartAttackTimer()
{
	EnemyState = EEnemyStates::EES_Attacking;
	const float AttackTime = FMath::RandRange(AttackTimerMin, AttackTimerMax);
	GetWorldTimerManager().SetTimer(AttackTimer, this, &AEnemy::Attack, AttackTime);
}

void AEnemy::ClearAttackTimer()
{
	GetWorldTimerManager().ClearTimer(AttackTimer);
}

void AEnemy::PatrolTimerFinished()
{
	MoveToTarget(CurrentPatrolTarget);
}

bool AEnemy::IsOutsideAttackRadius()
{
	return !IsInTargetRange(CombatTarget, AttackRadius);
}

bool AEnemy::IsOutsideCombatRadius()
{
	return !IsInTargetRange(CombatTarget, CombatRadius);
}

bool AEnemy::IsChasing()
{
	return EnemyState == EEnemyStates::EES_Chasing;
}

bool AEnemy::IsInsideAttackRadius()
{
	return IsInTargetRange(CombatTarget, AttackRadius);
}

bool AEnemy::IsAttacking()
{
	return EnemyState == EEnemyStates::EES_Attacking;
}

bool AEnemy::IsDead()
{
	return EnemyState == EEnemyStates::EES_Dead;
}

bool AEnemy::IsEngaged()
{
	return EnemyState == EEnemyStates::EES_Engaged;
}

void AEnemy::MoveToTarget(AActor* InTarget)
{
	if (EnemyController == nullptr || InTarget == nullptr) return;
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(InTarget);
	MoveRequest.SetAcceptanceRadius(AcceptanceMoveToRadius);
	EnemyController->MoveTo(MoveRequest);
}

AActor* AEnemy::ChooseNewPatrolTarget()
{
	TArray<AActor*> ValidTargets;
	for (AActor* Target : AllPatrolTargets)
	{
		if (CurrentPatrolTarget != Target)
		{
			ValidTargets.AddUnique(Target);
		}
	}
	if (ValidTargets.Num() > 0)
	{
		const int32 RandIndex = FMath::RandHelper(ValidTargets.Num());
		return ValidTargets[RandIndex];
	}
	return nullptr;
}

bool AEnemy::IsInTargetRange(AActor* Target, double AcceptanceRadius)
{
	if (Target == nullptr) return false;
	const double DistanceToTarget = (Target->GetActorLocation() - GetActorLocation()).Length();
	//PRINT_To_Screen(TEXT("%d"), DistanceToTarget <= AcceptanceRadius)
	return DistanceToTarget <= AcceptanceRadius;
}

void AEnemy::SpawnDefaultWeapon()
{
	if (GetWorld() && WeaponClass)
	{
		EquippedWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass);
		EquippedWeapon->Equip(GetMesh(), FName("WeaponSocket"), this, this);
	}
}

void AEnemy::PawnSeen(APawn* PawnSeen)
{
	const bool bShouldChaseTarget = !IsChasing() && !IsAttacking() && !IsDead() && !IsEngaged() && PawnSeen->ActorHasTag(FName("EngageableTarget"));   
	if (bShouldChaseTarget)
	{
		CombatTarget = PawnSeen;
		ClearPatrolTimer();
		ChaseTarget();
	}
}


