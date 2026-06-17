#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "AGASSSellZone.generated.h"

class AAGASSPlaceableItemActor;
class UCanvas;
class UCanvasRenderTarget2D;
class UFont;
class UAGASSTeamWalletComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;
class USoundBase;
class UStaticMeshComponent;
struct FPropertyChangedEvent;
struct FHitResult;

UCLASS()
class AGASSECONOMY_API AAGASSSellZone : public AActor
{
	GENERATED_BODY()

public:
	AAGASSSellZone();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void HandleLocalizedBannerTextChanged();
	void RefreshBannerTextTexture();
	void UpdateBannerMaterialInstances();
	bool ShouldRenderLocalizedBannerText() const;
	void ScanSellVolume();

	UFUNCTION()
	void HandleSellVolumeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleBannerRenderTargetUpdate(UCanvas* Canvas, int32 Width, int32 Height);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlaySellSuccessSound(FVector_NetQuantize SoundLocation);

	bool TrySellPlaceable(AAGASSPlaceableItemActor* PlaceableItem);
	UAGASSTeamWalletComponent* GetWalletComponent() const;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Economy")
	TObjectPtr<UBoxComponent> SellVolume;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Economy")
	TObjectPtr<UStaticMeshComponent> SellZoneVisualMesh;

	UPROPERTY(EditAnywhere, Category = "AGASS|Economy|Audio")
	TObjectPtr<USoundBase> SellSuccessSound;

	UPROPERTY(EditAnywhere, Category = "AGASS|Economy|Banner", meta = (MultiLine = "true"))
	FText BannerText;

	UPROPERTY(EditAnywhere, Category = "AGASS|Economy|Banner")
	TObjectPtr<UFont> BannerFont;

	UPROPERTY(EditAnywhere, Category = "AGASS|Economy|Banner", meta = (ClampMin = "128", UIMin = "128"))
	int32 BannerTextureWidth;

	UPROPERTY(EditAnywhere, Category = "AGASS|Economy|Banner", meta = (ClampMin = "32", UIMin = "32"))
	int32 BannerTextureHeight;

	UPROPERTY(EditAnywhere, Category = "AGASS|Economy|Banner", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BannerFontSize;

	UPROPERTY(EditAnywhere, Category = "AGASS|Economy|Banner", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float BannerTextScale;

	UPROPERTY(EditAnywhere, Category = "AGASS|Economy|Banner", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BannerTextSpacing;

	UPROPERTY(EditAnywhere, Category = "AGASS|Economy|Banner")
	FName BannerTextureParameterName;

	UPROPERTY(EditAnywhere, Category = "AGASS|Economy|Banner")
	FName BannerTextScaleParameterName;

	UPROPERTY(EditAnywhere, Category = "AGASS|Economy|Banner")
	FName BannerTextPeriodScaleParameterName;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasRenderTarget2D> BannerRenderTarget;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BannerDynamicMaterials;

	float CachedBannerTextPeriodScale;
	FTimerHandle SellVolumeScanTimerHandle;
	FDelegateHandle TextRevisionChangedHandle;
};
