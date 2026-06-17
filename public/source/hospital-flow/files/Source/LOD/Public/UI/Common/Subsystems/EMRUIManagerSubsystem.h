
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/Frontend/Utils/EMRFrontendEnumTypes.h"
#include "EMRUIManagerSubsystem.generated.h"

class UEMRCommonPrimaryLayoutWidget;
class UEMRFrontendCommonActivatableWidgetBase;
class UEMRFrontendCommonButtonBase;
class APlayerController;
struct FGameplayTag;

enum class EAsyncPushWidgetState : uint8
{
	OnCreatedBeforePush,
	AfterPush
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnButtonDescriptionTextUpdatedDelegate, UEMRFrontendCommonButtonBase*, BroadcastingButton, FText, DescriptionText);

UCLASS()
class LOD_API UEMRUIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	
	static UEMRUIManagerSubsystem* Get(const UObject* WorldContextObject);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	UFUNCTION(BlueprintCallable)
	UEMRCommonPrimaryLayoutWidget* CreatePrimaryLayoutWidget(TSubclassOf<UEMRCommonPrimaryLayoutWidget> InWidgetClass);

	UFUNCTION(BlueprintCallable)
	UEMRCommonPrimaryLayoutWidget* EnsurePrimaryLayout(APlayerController* CurrentPC, TSubclassOf<UEMRCommonPrimaryLayoutWidget> InWidgetClass);

	UFUNCTION(BlueprintCallable)
	void RegisterCreatedPrimaryLayoutWidget(UEMRCommonPrimaryLayoutWidget* InCreatedWidget);

    UFUNCTION(BlueprintCallable, BlueprintPure)
    UEMRCommonPrimaryLayoutWidget* GetPrimaryLayoutWidget() const;

    UFUNCTION(BlueprintCallable)
    void ClearPrimaryLayoutWidgets();

	void PushSoftWidgetToStackAsync(const FGameplayTag& InWidgetStackTag, TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> InSoftWidgetClass, TFunction<void(EAsyncPushWidgetState, UEMRFrontendCommonActivatableWidgetBase*)> AsyncPushStateCallback);
	void PushOrReplaceSoftWidgetToStackAsync(const FGameplayTag& InWidgetStackTag, TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> InSoftWidgetClass, TFunction<void(EAsyncPushWidgetState, UEMRFrontendCommonActivatableWidgetBase*)> AsyncPushStateCallback);

	void RemoveWidgetFromStack(const FGameplayTag& InWidgetStackTag, UEMRFrontendCommonActivatableWidgetBase* WidgetToRemove);
	void RemoveAllWidgetsFromStack(const FGameplayTag& InWidgetStackTag);

	void PushConfirmScreenToModalStackAsync(EConfirmScreenType InScreenType, const FText& InScreenTitle, const FText& InScreenMessage, TFunction<void(EConfirmScreenButtonType)> ButtonClickedCallback);

	UPROPERTY(BlueprintAssignable)
	FOnButtonDescriptionTextUpdatedDelegate OnButtonDescriptionTextUpdated;

private:
	UPROPERTY()
	TObjectPtr<UEMRCommonPrimaryLayoutWidget> CreatedPrimaryLayout;
};
