
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "MobaAnimNotify_SendGameplayEvent.generated.h"

UCLASS()
class UMobaAnimNotify_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
	
private:
	UPROPERTY(EditAnywhere, Category = "Gameplay Abilities")
	FGameplayTag EventTag;
};
