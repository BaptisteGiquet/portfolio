
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "GAS/Attributes/MobaAttributeSet.h"
#include "MobaPlayerStatWidget.generated.h"


class UMobaAbilitySystemComponent;
class UTextBlock;
class UImage;

UCLASS()
class LOD_API UMobaPlayerStatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	
private:
	void OnAttributeChanged(const FOnAttributeChangeData& AttributeData);
	
	void SetAttributeValue(const float NewValue);
	
	UPROPERTY()
	TObjectPtr<UMobaAbilitySystemComponent> OwnerAbilitySystemComponent = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Attribute")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, Category = "Visual")
	TObjectPtr<UTexture2D> IconTexture = nullptr;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_AttributeValue = nullptr;

	FNumberFormattingOptions NumberFormatingOptions;
};
