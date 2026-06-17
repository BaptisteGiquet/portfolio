
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Actor.h"
#include "MobaProjectileActor.generated.h"

UCLASS()
class LOD_API AMobaProjectileActor : public AActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AMobaProjectileActor();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamID; }

	void ShootProjectile(float InSpeed, float InMaxDistance, AActor* InTarget, FGenericTeamId InTeamID, FGameplayEffectSpecHandle InHitEffectHandle);
	
	
protected:
	virtual void BeginPlay() override;

private:
	void TravelMaxDistanceReached();
	void SendLocalGameplayCue(AActor* CueTargetActor, const FHitResult& HitResult);
	
	UPROPERTY(EditDefaultsOnly, Category = "GameplayCue")
	FGameplayTag HitGameplayCueTag;
	
	UPROPERTY(Replicated)
	FGenericTeamId TeamID;

	UPROPERTY(Replicated)
	FVector MoveDirection;

	UPROPERTY(Replicated)
	float ProjectileSpeed;

	UPROPERTY()
	TObjectPtr<AActor> Target;

	FGameplayEffectSpecHandle HitEffectSpecHandle;
	FTimerHandle ShootTimerHandle;
};
