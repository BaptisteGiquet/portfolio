#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AGASSCarriedUsablePresentationInterface.generated.h"

class AActor;
class UAnimMontage;
class USkeletalMeshComponent;

UINTERFACE(MinimalAPI)
class UAGASSCarriedUsablePresentationInterface : public UInterface
{
	GENERATED_BODY()
};

class AGASSCOMMON_API IAGASSCarriedUsablePresentationInterface
{
	GENERATED_BODY()

public:
	virtual USkeletalMeshComponent* GetAGASSCarriedUsableAttachMesh() const = 0;
	virtual USkeletalMeshComponent* GetAGASSLocalViewCarriedUsableAttachMesh() const = 0;
	virtual void SetAGASSCarriedUsablePresentation(AActor* UsableItemActor, FName CarryAnimationId) = 0;
	virtual void ClearAGASSCarriedUsablePresentation(AActor* UsableItemActor) = 0;
	virtual void PlayAGASSCarriedUsableUseMontage(UAnimMontage* UseMontage, float PlayRate = 1.f) = 0;
};
