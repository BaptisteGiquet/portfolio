
#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "MobaAbilitySlotWidget.generated.h"


class UMobaAbilitySystemComponent;
class UAbilitySystemComponent;
class UGameplayAbility;
class UTextBlock;
class UImage;


USTRUCT(BlueprintType)
struct FAbilitySlotWidgetData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FText AbilityName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FGameplayTag AbilityGameplayTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> AbilityIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FText AbilityDescription;
};


UCLASS()
class UMobaAbilitySlotWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	void InitializeAbilityUpgradePoints();
	virtual void NativeConstruct() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	
	void ChangeAbilityIcon(const FAbilitySlotWidgetData* AbilityWidgetData);

private:
	FGameplayAbilitySpec* GetAbilitySpec();
	
	void OnAbilityCommitted(UGameplayAbility* AbilityCommitted);

	void StartCooldown(const float CooldownTimeRemaining, const float CooldownDuration);
	void OnUpdateCooldown();
	void OnCooldownFinished();

	void OnAbilitySpecUpdated(const FGameplayAbilitySpec& AbilitySpec);
	void OnManaUpdated(const FOnAttributeChangeData& ManaData);
	void OnUpgradePointUpdated(const FOnAttributeChangeData& UpgradePointData);
	void UpdateCanCastParam();
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Cooldown")
	float CooldownUpdateFrequency = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FName IconMaterialParamName = TEXT("Icon");

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FName CooldownPercentParamName = TEXT("CooldownPercent");

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FName AbilityLevelParamName = TEXT("AbilityCurrentLevel");
	
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FName CanCastAbilityParamName = TEXT("CanCastAbility");

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FName UpgradeAvailableParamName = TEXT("UpgradeAvailable");

	
	UPROPERTY()
	TObjectPtr<UGameplayAbility> AbilityCDOForThisSlot;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedOwnerAbilitySystemComponent;

	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_AbilityLevelBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_CooldownCounter;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_CooldownDuration;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_Cost;

	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Cooldown")
	bool bIsCooldownTwoDigit = true;
	
	FNumberFormattingOptions WholeNumberFormattingOptions;
	FNumberFormattingOptions TwoDigitNumberFormattingOptions;

	
	float CachedCooldownDuration = 0.f;
	float CachedCooldownTimeRemaining = 0.f;
	
	FTimerHandle AbilityCooldownTimerHandle;
	FTimerHandle AbilityCooldownUpdateTimerHandle;
	
	FGameplayAbilitySpecHandle CachedAbilitySpecHandle;

	bool bIsAbilityLearned = false;
};
