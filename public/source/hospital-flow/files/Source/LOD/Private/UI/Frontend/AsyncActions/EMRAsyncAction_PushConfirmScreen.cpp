


#include "UI/Frontend/AsyncActions/EMRAsyncAction_PushConfirmScreen.h"

#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"



UEMRAsyncAction_PushConfirmScreen* UEMRAsyncAction_PushConfirmScreen::PushConfirmScreen(const UObject* WorldContextObject, EConfirmScreenType ScreenType, FText InScreenTitle, FText InScreenMessage)
{
	if (GEngine)
	{
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		{
			UEMRAsyncAction_PushConfirmScreen* Node = NewObject<UEMRAsyncAction_PushConfirmScreen>();
			Node->CachedOwningWorld = World;
			Node->CachedScreenType = ScreenType;
			Node->CachedScreenTitle = InScreenTitle;
			Node->CachedScreenMessage = InScreenMessage;

			Node->RegisterWithGameInstance(World);

			return Node;
		}

		return nullptr;
	}

	return nullptr;
}


void UEMRAsyncAction_PushConfirmScreen::Activate()
{
	Super::Activate();

	UEMRUIManagerSubsystem::Get(CachedOwningWorld.Get())->PushConfirmScreenToModalStackAsync(
		CachedScreenType,
		CachedScreenTitle,
		CachedScreenMessage,
		[this](EConfirmScreenButtonType ClickedButtonType)
		{
			OnButtonClicked.Broadcast(ClickedButtonType);

			SetReadyToDestroy();
		});
}
