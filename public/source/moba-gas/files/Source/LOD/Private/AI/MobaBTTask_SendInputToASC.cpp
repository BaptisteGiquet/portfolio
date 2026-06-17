
#include "AI/MobaBTTask_SendInputToASC.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"

EBTNodeResult::Type UMobaBTTask_SendInputToASC::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* OwnerAIController = OwnerComp.GetAIOwner();
	if (!OwnerAIController)
	{
		return EBTNodeResult::Failed;	
	}
	
	UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerAIController->GetPawn());
	if (!OwnerASC)
	{
		return EBTNodeResult::Failed;
	}

	
	OwnerASC->PressInputID(static_cast<int32>(InputID));
	return EBTNodeResult::Succeeded;
}
