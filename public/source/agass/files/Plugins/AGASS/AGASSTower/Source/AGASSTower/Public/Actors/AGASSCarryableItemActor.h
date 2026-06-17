#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGASSCarryableItemActor.generated.h"

class AController;
class APlayerState;
class UAGASSItemDefinition;

UENUM(BlueprintType)
enum class EAGASSCarriedItemType : uint8
{
	Placeable,
	Usable
};

UCLASS(Abstract)
class AGASSTOWER_API AAGASSCarryableItemActor : public AActor
{
	GENERATED_BODY()

public:
	AAGASSCarryableItemActor();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual EAGASSCarriedItemType GetCarriedItemType() const PURE_VIRTUAL(AAGASSCarryableItemActor::GetCarriedItemType, return EAGASSCarriedItemType::Placeable;);
	virtual bool CanBeClaimedBy(const AController* Controller) const;
	virtual bool Claim(AController* Controller);
	virtual bool ClaimForImmediateCarry(AController* Controller);
	virtual void ReleaseClaim(bool bRestoreTransform);
	virtual void ReleaseFromCarry(const FTransform& ReleaseTransform);

	void SetItemDefinition(UAGASSItemDefinition* NewItemDefinition);
	const UAGASSItemDefinition* GetItemDefinition() const;
	bool IsCarried() const;
	UClass* ResolveSpawnActorClass() const;

protected:
	bool BeginCarry(AController* Controller, bool bSkipClaimValidation);
	void EndCarry(bool bRestoreTransform, const FTransform* ReleaseTransformOverride);
	virtual bool CanBeClaimedByController(const AController* Controller) const;
	virtual void HandleItemDefinitionChanged();
	virtual void HandleCarryStarted(AController* ClaimingController);
	virtual void HandleCarryEnded();
	virtual void HandleCarryStateChanged(bool bNowCarried);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ItemDefinition, Category = "AGASS|Item")
	TObjectPtr<UAGASSItemDefinition> ItemDefinition;

	UPROPERTY(ReplicatedUsing = OnRep_HoldingPlayerState)
	TObjectPtr<APlayerState> HoldingPlayerState;

	UPROPERTY(ReplicatedUsing = OnRep_IsCarried)
	bool bIsCarried = false;

	FTransform ClaimStartTransform = FTransform::Identity;

private:
	UFUNCTION()
	void OnRep_ItemDefinition();

	UFUNCTION()
	void OnRep_HoldingPlayerState();

	UFUNCTION()
	void OnRep_IsCarried();
};
