

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "EMRCommonPrimaryLayoutWidget.generated.h"

class UCommonActivatableWidgetContainerBase;

/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRCommonPrimaryLayoutWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCommonActivatableWidgetContainerBase* FindWidgetStackByTag(const FGameplayTag& Intag) const;
	void ClearAllWidgetStacks();
	
protected:
	virtual void NativeOnInitialized() override;

protected:
	UFUNCTION(BlueprintCallable, meta = (Categories = "EMR.UI.WidgetStack"))
	void RegisterWidgetStack(FGameplayTag InStackTag, UCommonActivatableWidgetContainerBase* InStack);
	
private:
	UPROPERTY(Transient)
	TMap<FGameplayTag, UCommonActivatableWidgetContainerBase*> RegisteredWidgetStackMap;
};
