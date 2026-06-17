
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "MobaAnimNotify_SendTargetsPositions.generated.h"


UCLASS()
class UMobaAnimNotify_SendTargetsPositions : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	void SendLocalGameplayCue(const FHitResult& HitResult) const;
	
	UPROPERTY(EditAnywhere, Category = "Gameplay Abilities")
	float SweepSphereRadius = 60.f;
	
	UPROPERTY(EditAnywhere, Category = "Gameplay Abilities")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, Category = "Gameplay Abilities")
	bool bIgnoreOwner = true;
	
	UPROPERTY(EditAnywhere, Category = "Gameplay Abilities")
	FGameplayTagContainer GameplayCueTagsToTrigger;
	
	UPROPERTY(EditAnywhere, Category = "Gameplay Abilities")
	FGameplayTag EventTag;
	
	UPROPERTY(EditAnywhere, Category = "Gameplay Abilities")
	TArray<FName> TargetSocketNames;
};
