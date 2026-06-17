#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGASSShopTerminal.generated.h"

class AAGASSCarryableItemActor;
class UActorComponent;
class UBoxComponent;
class UFont;
class UAGASSInteractionComponent;
class UAGASSShopCatalog;
class UAGASSItemDefinition;
class UAGASSTeamWalletComponent;
class UPrimitiveComponent;
class USceneComponent;
class USoundBase;
class UStaticMeshComponent;
class UStaticMesh;
class UTextRenderComponent;

struct FHitResult;
struct FAGASSShopCatalogEntry;

UENUM()
enum class EAGASSShopPurchaseFailureReason : uint8
{
	None,
	InsufficientFunds,
	HoldingItem,
	ContentError
};

USTRUCT()
struct FAGASSGeneratedShopSlot
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> SlotRoot = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> DisplayMeshComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PlatformMeshComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> PurchaseTriggerComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> PriceTextComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> PurchaseSpawnPoint = nullptr;

	int32 CatalogIndex = INDEX_NONE;
};

UCLASS()
class AGASSECONOMY_API AAGASSShopTerminal : public AActor
{
	GENERATED_BODY()

public:
	AAGASSShopTerminal();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RebuildGeneratedShopSlots();
	void ClearGeneratedShopSlots();
	const UAGASSShopCatalog* ResolveCatalog() const;
	const FAGASSShopCatalogEntry* ResolveCatalogEntryByIndex(const UAGASSShopCatalog& Catalog, int32 EntryIndex) const;
	UAGASSItemDefinition* ResolveEntryItemDefinition(const FAGASSShopCatalogEntry& Entry) const;
	UClass* ResolveEntryItemClass(const FAGASSShopCatalogEntry& Entry) const;
	UStaticMesh* ResolveEntryDisplayMesh(const FAGASSShopCatalogEntry& Entry) const;
	FText ResolveEntryPriceText(const FAGASSShopCatalogEntry& Entry) const;
	FTransform ResolveSlotPurchaseSpawnTransform(const FAGASSShopCatalogEntry& Entry, const USceneComponent& SlotPurchaseSpawnPoint) const;
	float ResolveDisplayScale(const UStaticMesh& DisplayMesh) const;
	bool TryPurchaseSlot(int32 GeneratedSlotIndex, APawn* PurchasingPawn, EAGASSShopPurchaseFailureReason& OutFailureReason);
	void PlayFailureSoundAtSlot(int32 GeneratedSlotIndex, EAGASSShopPurchaseFailureReason FailureReason);
	USoundBase* ResolveFailureSound(EAGASSShopPurchaseFailureReason FailureReason) const;
	void ApplyLegacyTerminalPresentationState();
	void DestroyGeneratedComponent(UActorComponent* Component);
	UAGASSTeamWalletComponent* GetWalletComponent() const;
	UAGASSInteractionComponent* FindInteractionComponent(const AController* InteractingController) const;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayFailureSound(FVector_NetQuantize SoundLocation, EAGASSShopPurchaseFailureReason FailureReason);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayPurchaseSuccessSound(FVector_NetQuantize SoundLocation);

	UFUNCTION()
	void HandlePurchaseTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Shop")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Shop")
	TObjectPtr<UStaticMeshComponent> TerminalMeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Shop")
	TObjectPtr<USceneComponent> PurchaseSpawnPoint;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	TSoftObjectPtr<UAGASSShopCatalog> ShopCatalog;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "AGASS|Shop|Legacy")
	FName CatalogEntryId;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "AGASS|Shop|Legacy", meta = (ClampMin = "0.0"))
	float InteractionDistance = 300.f;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	TObjectPtr<UStaticMesh> PlatformMesh;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	TObjectPtr<UFont> PriceFont;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	TObjectPtr<USoundBase> InsufficientFundsFailureSound;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	TObjectPtr<USoundBase> HandsFullFailureSound;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	TObjectPtr<USoundBase> PurchaseSuccessSound;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop", meta = (ClampMin = "0.0"))
	float EntrySpacing = 180.f;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop", meta = (ClampMin = "1.0"))
	float DisplayMaxDimension = 60.f;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	FVector DisplayOffset = FVector(0.f, 0.f, 70.f);

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	FRotator DisplayRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	FVector PlatformOffset = FVector(90.f, 0.f, 10.f);

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	FVector PlatformScale = FVector(1.f, 1.f, 0.25f);

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	FVector PriceTextOffset = FVector(0.f, 0.f, 140.f);

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop", meta = (ClampMin = "1.0"))
	float PriceTextWorldSize = 28.f;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop")
	FColor PriceTextColor = FColor::White;

	UPROPERTY(EditAnywhere, Category = "AGASS|Shop", meta = (ClampMin = "0.0"))
	FVector PurchaseTriggerExtent = FVector(60.f, 60.f, 45.f);

	UPROPERTY(Transient)
	TArray<FAGASSGeneratedShopSlot> GeneratedSlots;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UPrimitiveComponent>, int32> PurchaseTriggerToSlotIndex;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FallbackCubeMesh;
};
