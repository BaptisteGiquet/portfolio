
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "UI/Frontend/Utils/EMRFrontendEnumTypes.h"
#include "EMRAsyncAction_PushConfirmScreen.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConfirmScreenButtonClickedDelegate, EConfirmScreenButtonType, ClickedButtonType);

/**
 * 
 */
UCLASS()
class LOD_API UEMRAsyncAction_PushConfirmScreen : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", HidePin = "WorldContextObject", BlueprintInternalUseOnly = "true", DisplayName = "Show Confirmation Screen"))
	static UEMRAsyncAction_PushConfirmScreen* PushConfirmScreen(
		const UObject* WorldContextObject,
		EConfirmScreenType ScreenType,
		FText InScreenTitle,
		FText InScreenMessage);

	virtual void Activate() override;

	UPROPERTY(BlueprintAssignable)
	FOnConfirmScreenButtonClickedDelegate OnButtonClicked;
	
private:
	TWeakObjectPtr<UWorld> CachedOwningWorld;
	EConfirmScreenType CachedScreenType;
	FText CachedScreenTitle;
	FText CachedScreenMessage;
};
