
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "MobaAIController.generated.h"



struct FAIStimulus;
class UAISenseConfig_Sight;

UCLASS()
class AMobaAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMobaAIController();
	virtual void OnPossess(APawn* InPawn) override;

protected:
	virtual void BeginPlay() override;

private:
	void SetupSightPerception();
	
	void SetPawnGenericTeamID(APawn* InPawn);
	
	void GetPawnBlackboardComponent();
	void BindToGameplayTagChange();
	
	void SetCurrentTarget(AActor* NewTarget);
	AActor* GetCurrentTarget() const;

	void EnableAllSenses();
	void ClearAndDisableAllSenses();

	
	void OnStunTagUpdated(const FGameplayTag Tag, const int32 StunTagCount);
	
	void OnDeathTagUpdated(const FGameplayTag Tag, const int32 DeadTagCount);

	UFUNCTION()
	void TargetPerceptionUpdate(AActor* ActorPerceived, FAIStimulus Stimulus);

	void ForgetActorIfDead(AActor* ActorToForget);

	UFUNCTION()
	void TargetPerceptionForgotten(AActor* ForgottenActor);
	
	AActor* GetNextPerceivedActor() const;

	bool PawnIsStun() { return bPawnIsStun; };
	
	bool bPawnIsStun = false;

	bool PawnIsDead() { return bPawnIsDead; };
	
	bool bPawnIsDead = false;

	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTree = nullptr;

	UPROPERTY(Transient, VisibleDefaultsOnly, Category = "Behavior")
	TObjectPtr<UBlackboardComponent> BlackboardComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	FName TargetBlackboardKeyName = TEXT("Target");
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Perception|Sight")
	TObjectPtr<UAISenseConfig_Sight> SightConfig = nullptr;
};
