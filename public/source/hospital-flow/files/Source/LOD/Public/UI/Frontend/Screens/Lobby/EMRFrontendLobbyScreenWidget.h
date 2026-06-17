#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRFrontendLobbyScreenWidget.generated.h"

class UEMRFrontendCommonButtonBase;
class UCommonTextBlock;
class UEditableTextBox;
class UWidget;
class AEMRPlayerState;
class AEMRLobbyGameState;
class APlayerState;
struct FLobbyPlayerInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSocialsWidgetPushed, UEMRFrontendCommonActivatableWidgetBase*, PushedWidget);

/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRFrontendLobbyScreenWidget : public UEMRFrontendCommonActivatableWidgetBase
{
	GENERATED_BODY()

public:
	UEMRFrontendLobbyScreenWidget(const FObjectInitializer& ObjectInitializer);

	void FocusCharactersButton();
	void FocusInviteFriendsButton();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

public:
	UPROPERTY(BlueprintAssignable, Category = "EMR|Lobby Screen")
	FOnSocialsWidgetPushed OnSocialsWidgetPushed;

	UPROPERTY(BlueprintReadOnly, Category = "EMR|Lobby Screen")
	TWeakObjectPtr<UEMRFrontendCommonActivatableWidgetBase> LastPushedSocialsWidget;

	UPROPERTY(BlueprintReadOnly, Category = "EMR|Lobby Screen")
	TWeakObjectPtr<UEMRFrontendCommonActivatableWidgetBase> LastPushedCharactersWidget;

private:
	void OnBackBoundActionTriggered();

	UFUNCTION(BlueprintCallable)
	void HandleSocialsButtonClicked();

	UFUNCTION(BlueprintCallable)
	void HandleCharactersButtonClicked();

	UFUNCTION(BlueprintCallable)
	void HandleReadyButtonClicked();

	UFUNCTION(BlueprintCallable)
	void HandleStartGameClicked();

	UFUNCTION()
	void HandleLobbyPlayerInfoChanged(const FLobbyPlayerInfo& Info);

	void HandleLobbyPlayerStateAdded(APlayerState* PlayerState);
	void HandleLobbyPlayerStateRemoved(APlayerState* PlayerState);
	void BindToLobbyGameState();
	void ScheduleBindToLobbyGameStateRetry();
	void RefreshStartGameState();
	bool AreNonHostPlayersReady() const;

	UFUNCTION()
	void HandleValidateInviteCodeClicked();

	UFUNCTION()
	void HandleRegenerateInviteCodeClicked();

	UFUNCTION()
	void HandleBackToMenuClicked();

	void RefreshInviteCodeDisplay(const FString& NewCode);

	//***** Bound Widgets *****//
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_InviteFriends;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Characters;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Ready;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_StartGame;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_InviteCode;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEditableTextBox> EditableTextBox_InviteCode;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_ValidateInviteCode;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_RegenerateInviteCode;
	//***** Bound Widgets *****//
	
	
	UPROPERTY(EditDefaultsOnly, Category = "EMR|Lobby Screen")
	TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> SocialsScreenWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Lobby Screen", meta = (Categories = "EMR.UI.WidgetStack"))
	FGameplayTag SocialsScreenStackTag;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Lobby Screen")
	TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> CharactersScreenWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Lobby Screen", meta = (Categories = "EMR.UI.WidgetStack"))
	FGameplayTag CharactersScreenStackTag;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Lobby Screen|Back")
	FText BackConfirmTitle;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Lobby Screen|Back")
	FText BackConfirmMessage;

	TSet<TWeakObjectPtr<AEMRPlayerState>> BoundLobbyPlayerStates;
	TWeakObjectPtr<AEMRLobbyGameState> BoundLobbyGameState;

	FUIActionBindingHandle BackActionHandle;
	mutable TWeakObjectPtr<UWidget> PendingFocusWidget;
	FTimerHandle LobbyGameStateBindRetryHandle;
	int32 LobbyGameStateBindRetryCount = 0;
};
