

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "EMRAsyncAction_PushSoftWidget.generated.h"

class UEMRFrontendCommonActivatableWidgetBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPushSoftWidgetDelegate, UEMRFrontendCommonActivatableWidgetBase*, PushedWidget);
/**
 * 
 */
UCLASS()
class LOD_API UEMRAsyncAction_PushSoftWidget : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", HidePin = "WorldContextObject", BlueprintInternalUseOnly = "true", DisplayName = "Push Soft Widget To Widget Stack"))
	static UEMRAsyncAction_PushSoftWidget* PushSoftWidget(const UObject* WorldContextObject,
		APlayerController* OwningPlayerController,
		TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> InSoftWidgetClass,
		UPARAM(meta = (Categories = "EMR.UI.WidgetStack")) FGameplayTag InWidgetStackTag,
		bool bFocusOnNewlyPushedWidget = true);

	virtual void Activate() override;
	
	UPROPERTY(BlueprintAssignable)
	FOnPushSoftWidgetDelegate OnWidgetCreatedBeforePush;

	UPROPERTY(BlueprintAssignable)
	FOnPushSoftWidgetDelegate AfterPush;

private:
	TWeakObjectPtr<UWorld> CachedOwningWorld;
	TWeakObjectPtr<APlayerController> CachedOwningPC;
	TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> CachedSoftWidgetClass;
	FGameplayTag CachedWidgetStackTag;
	bool bCachedFocusOnNewlyPushedWidget = false;
};
