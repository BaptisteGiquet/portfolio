
#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "MobaTargetActor_Line.generated.h"

class USphereComponent;
class UNiagaraComponent;

UCLASS()
class LOD_API AMobaTargetActor_Line : public AGameplayAbilityTargetActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AMobaTargetActor_Line();

	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void BeginDestroy() override;
	
	void ConfigureTargetSetting(const float InTargetRange, const float InCylinderDetectionRadius, const float InTargetingFrequency, const FGenericTeamId InOwnerTeamID, const bool InbShouldDrawDebug);
	

protected:
	virtual void BeginPlay() override;

private:
	void DoTargetCheckAndReport();
	void CreateSweepMulti(FVector SweepEndLocation, TArray<FHitResult>& HitResults);
	void UpdateTargetTrace();
	bool ShouldReportActorAsTarget(const AActor* ActorToCheck) const;

	
	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	FName TargetActorFXLengthParamName = TEXT("Length");
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	TObjectPtr<USceneComponent> RootComp = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	TObjectPtr<UNiagaraComponent> TargetActorVFX = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	TObjectPtr<USphereComponent> TargetEndDetectionSphere = nullptr;

	
	UPROPERTY(Replicated)
	TObjectPtr<const AActor> AvatarActor = nullptr;

	UPROPERTY(Replicated)
	float TargetRange = 0.f;

	UPROPERTY(Replicated)
	FGenericTeamId OwnerTeamID;
	
	UPROPERTY(Replicated)
	float CylinderDetectionRadius = 0.f;
	
	
	UPROPERTY()
	float TargetingFrequency = 0.f;
	
	UPROPERTY()
	bool bShouldDrawDebug = false;

	FTimerHandle PeriodicalTargetingTimerHandle;
};
