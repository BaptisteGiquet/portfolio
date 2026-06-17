#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "EMRAIController.generated.h"


class AEMRPatient;
struct FAIStimulus;
class UAISenseConfig_Sight;
struct FEMRNavigationIntentMessage;
class UPathFollowingComponent;

UCLASS()
class AEMRAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEMRAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    /** Re-run the behavior tree / blackboard setup (used after seamless travel or pool reuse). */
    void RestartBehaviorTree();

    /** Clear navigation-related blackboard entries when the patient returns to the pool. */
    void ResetBlackboardState();

protected:
    virtual void BeginPlay() override;

private:
    void InitBehaviorTreeAndBlackboard();
	
	void RegisterToGameplayMessageSubsystem();
    void HandleNavigationIntentMessage(FGameplayTag ChannelTag, const FEMRNavigationIntentMessage& Message);
	
	UPROPERTY(EditDefaultsOnly, Category = "EMR|Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTree = nullptr;

	UPROPERTY(Transient, VisibleDefaultsOnly, Category = "EMR|Behavior")
	TObjectPtr<UBlackboardComponent> BlackboardComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Behavior")
    FName PatientTargetKeyName = TEXT("Target");

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Behavior")
	FName PatientWaitPointLocationKeyName = TEXT("PatientWaitPointLocation");

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Behavior")
	FName PatientWaitPointRotationKeyName = TEXT("PatientWaitPointRotation");

    FGameplayMessageListenerHandle NavigationIntentListenerHandle;
	
    TWeakObjectPtr<AEMRPatient> ControlledPatient;
};
