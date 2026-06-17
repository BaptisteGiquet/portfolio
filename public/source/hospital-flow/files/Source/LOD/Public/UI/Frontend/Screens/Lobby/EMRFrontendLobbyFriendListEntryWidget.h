#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonListEntryWidgetBase.h"
#include "EMRFrontendLobbyFriendListEntryWidget.generated.h"

class UCommonLazyImage;
class UCommonTextBlock;
class UEMRFrontendCommonButtonBase;
class UEMRListDataObjectLobbyFriend;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRFrontendLobbyFriendListEntryWidget : public UEMRFrontendCommonListEntryWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject) override;
	virtual void OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason) override;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Invite Button Clicked"))
	void BP_OnInviteButtonClicked(UEMRListDataObjectLobbyFriend* FriendData);

private:
	void HandleInviteButtonClicked();
	void RefreshFromData();

	//***** Bound Widgets *****//
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonLazyImage> CommonLazyImage_FriendPortrait;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_FriendName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Invite;
	//***** Bound Widgets *****//

	UPROPERTY(Transient)
	TObjectPtr<UEMRListDataObjectLobbyFriend> CachedFriendDataObject;
};
