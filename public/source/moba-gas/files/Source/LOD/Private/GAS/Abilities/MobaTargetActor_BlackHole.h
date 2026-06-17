
#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "MobaTargetActor_BlackHole.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class USphereComponent;

UCLASS()
class LOD_API AMobaTargetActor_BlackHole : public AGameplayAbilityTargetActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AMobaTargetActor_BlackHole();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Destroyed() override;
	
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void ConfirmTargetingAndContinue() override;
	virtual void CancelTargeting() override;
	

	void ConfigureBlackHole(const float InBlackHoleRange, const float InBlackHolePullSpeed, const float InBlackHoleDuration, const FGenericTeamId& InTeamID);


private:
	UFUNCTION()
	void ActorEnterBlackHoleRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	
	UFUNCTION()
	void ActorLeaveBlackHoleRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_BlackHoleRange();

	void TryAddTarget(AActor* TargetToAdd);
	void RemoveTarget(AActor* TargetToRemove);

	void StopBlackHole();
	


	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> BlackHoleLinkVFX;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	FName BlackHoleVFXOriginParamName = TEXT("Origin");
	
	UPROPERTY(ReplicatedUsing = "OnRep_BlackHoleRange")
	float BlackHoleRange = 2000.f;

	UPROPERTY(Replicated)
	FGenericTeamId OwnerTeamID;

	UPROPERTY()
	TMap<AActor*, UNiagaraComponent*> ActorsInRangeMap;


	float BlackHolePullSpeed = 200.f;
	float BlackHoleDuration = 8.f;

	FTimerHandle BlackHoleDurationTimerHandle;


	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	TObjectPtr<USceneComponent> RootComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	TObjectPtr<USphereComponent> DetectionSphereComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	TObjectPtr<UParticleSystemComponent> VFXComp;
};

