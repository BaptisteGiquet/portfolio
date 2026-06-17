
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MobaHUDResourceBarWidget.generated.h"


struct FOnAttributeChangeData;
struct FGameplayAttribute;

class UAbilitySystemComponent;
class UTextBlock;
class UProgressBar;

UCLASS()
class UMobaHUDResourceBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	void InitializeAndBindToAttributes(UAbilitySystemComponent* InAbilitySystemComponent, const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute);
	void SetResourceBarValues(const float InAttributeValue, const float InAttributeMaxValue);
	
private:
	void OnAttributeValueChanged(const FOnAttributeChangeData& AttributeData);
	void OnAttributeMaxValueChanged(const FOnAttributeChangeData& AttributeData);

	float CachedValue;
	float CachedMaxValue;
	
	UPROPERTY(EditAnywhere, Category = "Visual")
	FLinearColor BarColor;

	UPROPERTY(EditAnywhere, Category = "Visual")
	FSlateFontInfo ValueTextFont;

	UPROPERTY(EditAnywhere, Category = "Visual")
	bool bIsValueTextVisible = true;

	UPROPERTY(EditAnywhere, Category = "Visual")
	bool bIsProgressBarVisible = true;
	
	UPROPERTY(VisibleDefaultsOnly, meta=(BindWidget))
	TObjectPtr<UProgressBar> ProgressBar;

	UPROPERTY(VisibleDefaultsOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> ValueText;
};
