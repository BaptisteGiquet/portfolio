
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GAS/Abilities/MobaGameplayTypes.h"
#include "MobaBTTask_SendInputToASC.generated.h"

UCLASS()
class UMobaBTTask_SendInputToASC : public UBTTaskNode
{
	GENERATED_BODY()

public:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	UPROPERTY(EditAnywhere, Category = "Ability")
	EAbilityInputID InputID;
};
