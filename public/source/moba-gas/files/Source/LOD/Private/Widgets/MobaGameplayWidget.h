
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GAS/Abilities/MobaGameplayTypes.h"
#include "MobaGameplayWidget.generated.h"


class UMobaCrosshairWidget;
class UCanvasPanel;
class UWidgetSwitcher;
class UMobaGameplayMenuWidget;
class UMobaMatchStatsWidget;
class UMobaSkeletalMeshSceneCaptureWidget;
class UMobaInventoryWidget;
class UMobaShopWidget;
class UMobaPlayerStatWidget;
class UMobaAbilityListViewWidget;
class UGameplayAbility;
class UMobaAbilitySystemComponent;
class UMobaHUDResourceBarWidget;

UCLASS()
class UMobaGameplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void ToggleShopVisibility();
	
	void ShowGameplayMenu();
	void SetGameplayMenuTitle(const FString& NewTitle);

	UFUNCTION()
	void ToggleGameplayMenu();
	
private:
	void ConfigureAbilities(const TMap<EAbilityInputID, TSubclassOf<UGameplayAbility>>& MainAbilities);
	
	void OpenShop();
	void CloseShop();
	
	void SetOwningPawnInputEnabled(bool bIsPawnInputEnabled);
	void SetFocusToGameAndUI();
	void SetFocusToGameOnly();
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaHUDResourceBarWidget> Widget_HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaHUDResourceBarWidget> Widget_ManaBar;
	

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaPlayerStatWidget> Widget_AttackDamageStat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaPlayerStatWidget> Widget_AbilityPowerStat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaPlayerStatWidget> Widget_AttackSpeedStat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaPlayerStatWidget> Widget_ArmorStat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaPlayerStatWidget> Widget_MagicResistanceStat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaPlayerStatWidget> Widget_MoveSpeedStat;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaPlayerStatWidget> Widget_CooldownReductionStat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaPlayerStatWidget> Widget_ExperienceStat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaPlayerStatWidget> Widget_GoldStat;

	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaAbilityListViewWidget> ListView_Abilities;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaShopWidget> Widget_Shop;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaInventoryWidget> Widget_Inventory;

	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaSkeletalMeshSceneCaptureWidget> Widget_HeroPortrait;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaCrosshairWidget> Widget_Crosshair;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaMatchStatsWidget> Widget_MatchStats;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaGameplayMenuWidget> Widget_GameplayMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_Main;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootPanel_GameplayWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootPanel_GameplayMenuWidget;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> ShopPopupAnimation;

	UPROPERTY()
	TObjectPtr<UMobaAbilitySystemComponent> OwnerAbilitySystemComponent;
};
