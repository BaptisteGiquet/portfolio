#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRFrontendLobbySocialsScreenWidget.generated.h"

class UEMRFrontendLobbyFriendList;
class UEMRListDataObjectLobbyFriend;
class UEMRLobbyFriendsSubsystem;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRFrontendLobbySocialsScreenWidget : public UEMRFrontendCommonActivatableWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetFriendListItems(const TArray<UEMRListDataObjectLobbyFriend*>& InFriendListItems);

protected:
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual void NativeOnInitialized() override;

private:
	void OnBackBoundActionTriggered();
	void RefreshFriendsList();
	void HandleFriendsListUpdated();

	//***** Bound Widgets *****//
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendLobbyFriendList> CommonListView_FriendList;
	//***** Bound Widgets *****//

	TWeakObjectPtr<UEMRLobbyFriendsSubsystem> CachedFriendsSubsystem;

	FUIActionBindingHandle BackActionHandle;
};
