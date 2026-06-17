
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "MobaCrosshairWidget.generated.h"


struct FGameplayEventData;
class UCanvasPanelSlot;
class UAbilitySystemComponent;
class UImage;

UCLASS()
class LOD_API UMobaCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
private:
	void CrosshairTagUpdated(const FGameplayTag CrosshairTag, int32 CrosshairTagCount);
	void UpdateCrosshairPosition();

	void AimTargetUpdated(const FGameplayEventData* EventData);

	UPROPERTY(EditDefaultsOnly, Category = "View")
	FLinearColor HasTargetColor = FLinearColor::Green;

	UPROPERTY(EditDefaultsOnly, Category = "View")
	FLinearColor NoTargetColor = FLinearColor::White;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Crosshair;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> OwnerASC;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> CanvasSlot_Crosshair;

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY()
	TObjectPtr<const AActor> AimTarget;

	bool bIsCrosshairActive = false;
};
