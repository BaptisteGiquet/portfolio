#pragma once

#include "CoreMinimal.h"
#include "Actors/AGASSCarryableItemActor.h"
#include "AGASSUsableItemActor.generated.h"

class AController;
class APawn;
class UAGASSUsableItemDefinition;
class UAnimMontage;
class UStaticMeshComponent;
class USkeletalMeshComponent;

UCLASS(Abstract, Blueprintable)
class AGASSTOWER_API AAGASSUsableItemActor : public AAGASSCarryableItemActor
{
	GENERATED_BODY()

public:
	AAGASSUsableItemActor();

	virtual void BeginPlay() override;
	virtual EAGASSCarriedItemType GetCarriedItemType() const override;
	virtual bool TryUse(AController* UsingController);
	virtual bool ConsumeOnSuccessfulUse() const;

	const UAGASSUsableItemDefinition* GetUsableItemDefinition() const;
	FName GetCarryAttachSocketName() const;
	FTransform GetCarryAttachRelativeTransform() const;
	FName GetCarryAnimationId() const;
	UAnimMontage* GetUseMontage() const;
	void PlayUseMontageForController(AController* UsingController, float PlayRate = 1.f) const;

protected:
	virtual void HandleItemDefinitionChanged() override;
	virtual void HandleCarryStarted(AController* ClaimingController) override;
	virtual void HandleCarryEnded() override;
	virtual void HandleCarryStateChanged(bool bNowCarried) override;
	virtual void HandleFirstPersonCarryPresentationUpdated();

	UStaticMeshComponent* GetMutableItemMeshComponent() const
	{
		return ItemMeshComponent;
	}

	UStaticMeshComponent* GetFirstPersonPresentationMeshComponent() const
	{
		return FirstPersonItemMeshComponent;
	}

	bool IsUsingFirstPersonCarryPresentation() const;

private:
	void ApplyResolvedItemVisuals();
	void ApplyCarryState();
	void AttachToCarrierPawn(APawn* CarrierPawn);
	void AttachFirstPersonPresentationToCarrierPawn(APawn* CarrierPawn);
	void ResetFirstPersonPresentationAttachment();
	bool CanUseFirstPersonCarryPresentation(APawn* CarrierPawn) const;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Usable")
	TObjectPtr<UStaticMeshComponent> ItemMeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Usable|FirstPerson")
	TObjectPtr<UStaticMeshComponent> FirstPersonItemMeshComponent;

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AGASS|Usable|Legacy")
	FName LegacyCarryAttachSocketName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AGASS|Usable|Legacy")
	FTransform LegacyCarryAttachRelativeTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AGASS|Usable|Legacy")
	FName LegacyCarryAnimationId;

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AGASS|Usable|Legacy")
	TSoftObjectPtr<UAnimMontage> LegacyUseMontage;

	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "AGASS|Usable|Legacy")
	bool bConsumeOnSuccessfulUse = true;

	TWeakObjectPtr<APawn> CurrentCarrierPawn;
};
