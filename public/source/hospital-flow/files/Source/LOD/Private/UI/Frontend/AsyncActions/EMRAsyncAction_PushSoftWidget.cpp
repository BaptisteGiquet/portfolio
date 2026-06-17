


#include "UI/Frontend/AsyncActions/EMRAsyncAction_PushSoftWidget.h"

#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"


UEMRAsyncAction_PushSoftWidget* UEMRAsyncAction_PushSoftWidget::PushSoftWidget(const UObject* WorldContextObject, APlayerController* OwningPlayerController, TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> InSoftWidgetClass, UPARAM(meta = (Categories = "EMR.UI.WidgetStack")) FGameplayTag InWidgetStackTag, bool bFocusOnNewlyPushedWidget)
{
	checkf(!InSoftWidgetClass.IsNull(), TEXT("PushSoftWidgetToStack was passed a null SoftWidgetClass"));

	if (!GEngine) return nullptr;
	
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return nullptr;
	
	UEMRAsyncAction_PushSoftWidget* Node = NewObject<UEMRAsyncAction_PushSoftWidget>();
	if (Node)
	{
		Node->CachedOwningWorld = World;
		Node->CachedOwningPC = OwningPlayerController;
		Node->CachedWidgetStackTag = InWidgetStackTag;
		Node->CachedSoftWidgetClass = InSoftWidgetClass;
		Node->bCachedFocusOnNewlyPushedWidget = bFocusOnNewlyPushedWidget;
		
		Node->RegisterWithGameInstance(World);

		return Node;
	}

	return nullptr;
}


void UEMRAsyncAction_PushSoftWidget::Activate()
{
	Super::Activate();

	UEMRUIManagerSubsystem* UISubsystem = UEMRUIManagerSubsystem::Get(CachedOwningWorld.Get());
	if (!UISubsystem) return;

	UISubsystem->PushSoftWidgetToStackAsync(CachedWidgetStackTag, CachedSoftWidgetClass,
		[this](EAsyncPushWidgetState InPushState, UEMRFrontendCommonActivatableWidgetBase* PushedWidget)
		{
			switch (InPushState)
			{
			case EAsyncPushWidgetState::OnCreatedBeforePush:
				PushedWidget->SetOwningPlayer(CachedOwningPC.Get());

				OnWidgetCreatedBeforePush.Broadcast(PushedWidget);
				
				break;
				
			case EAsyncPushWidgetState::AfterPush:
				
				AfterPush.Broadcast(PushedWidget);

				if (bCachedFocusOnNewlyPushedWidget)
				{
					if (UWidget* WidgetToFocus = PushedWidget->GetDesiredFocusTarget())
					{
						WidgetToFocus->SetFocus();
					}
				}

				SetReadyToDestroy();
				
				break;
				
			default:
				break;
			}
		}
		);
}
