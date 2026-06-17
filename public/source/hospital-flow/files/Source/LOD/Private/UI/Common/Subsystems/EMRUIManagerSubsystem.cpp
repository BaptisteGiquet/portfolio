

#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/AssetManager.h"
#include "GAS/EMRTags.h"
#include "UI/Frontend/Core/EMRCommonPrimaryLayoutWidget.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "UI/Frontend/Screens/EMRConfirmScreenWidget.h"
#include "UI/Frontend/Utils/EMRFrontendFunctionLibrary.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

UEMRUIManagerSubsystem* UEMRUIManagerSubsystem::Get(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
		if (World)
		{
			APlayerController* ContextPC = nullptr;
			if (const UUserWidget* UserWidget = Cast<UUserWidget>(WorldContextObject))
			{
				ContextPC = UserWidget->GetOwningPlayer();
			}
			else if (const APlayerController* AsPC = Cast<APlayerController>(WorldContextObject))
			{
				ContextPC = const_cast<APlayerController*>(AsPC);
			}

			ULocalPlayer* LocalPlayer = ContextPC ? ContextPC->GetLocalPlayer() : nullptr;
			if (!LocalPlayer)
			{
				LocalPlayer = World->GetFirstLocalPlayerFromController();
			}

			if (LocalPlayer)
			{
				UEMRUIManagerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEMRUIManagerSubsystem>();
				return Subsystem;
			}
		}
	}

	return nullptr;
}

bool UEMRUIManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Outer);
	if (!LocalPlayer)
	{
		return false;
	}

	const UWorld* World = LocalPlayer->GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	TArray<UClass*> FoundClasses;
	GetDerivedClasses(GetClass(), FoundClasses);

	return FoundClasses.IsEmpty();
}


UEMRCommonPrimaryLayoutWidget* UEMRUIManagerSubsystem::CreatePrimaryLayoutWidget(TSubclassOf<UEMRCommonPrimaryLayoutWidget> InWidgetClass)
{
	if (IsValid(CreatedPrimaryLayout))
	{
		return CreatedPrimaryLayout;
	}

	if (!InWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePrimaryLayoutWidget: InWidgetClass is null"));
		return nullptr;
	}

	ULocalPlayer* LP = GetLocalPlayer();
	UWorld* World = GetWorld();
	if (!LP || !World)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePrimaryLayoutWidget: Missing LocalPlayer/World"));
		return nullptr;
	}

	APlayerController* PC = LP->GetPlayerController(World);
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePrimaryLayoutWidget: Missing PlayerController"));
		return nullptr;
	}

	UEMRCommonPrimaryLayoutWidget* CreatedWidget = CreateWidget<UEMRCommonPrimaryLayoutWidget>(PC, InWidgetClass);
	
	if (!CreatedWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePrimaryLayoutWidget: CreateWidget failed"));
		return nullptr;
	}

	CreatedWidget->AddToViewport();

	CreatedPrimaryLayout = CreatedWidget;
	return CreatedPrimaryLayout;
}


UEMRCommonPrimaryLayoutWidget* UEMRUIManagerSubsystem::EnsurePrimaryLayout(APlayerController* CurrentPC, TSubclassOf<UEMRCommonPrimaryLayoutWidget> InWidgetClass)
{
	const bool bHasExistingLayout = IsValid(CreatedPrimaryLayout);
	const bool bIsInViewport = bHasExistingLayout ? CreatedPrimaryLayout->IsInViewport() : false;
	const APlayerController* ExistingOwner = bHasExistingLayout ? CreatedPrimaryLayout->GetOwningPlayer() : nullptr;

	if (!CurrentPC)
	{
		return CreatedPrimaryLayout;
	}

	if (!InWidgetClass)
	{
		return CreatedPrimaryLayout;
	}

	const bool bNeedsRebuild = !bHasExistingLayout || !bIsInViewport || ExistingOwner != CurrentPC;
	if (!bNeedsRebuild)
	{
		return CreatedPrimaryLayout;
	}

	if (bHasExistingLayout)
	{
		CreatedPrimaryLayout->RemoveFromParent();
	}

	UEMRCommonPrimaryLayoutWidget* CreatedWidget = CreateWidget<UEMRCommonPrimaryLayoutWidget>(CurrentPC, InWidgetClass);
	if (!CreatedWidget)
	{
		CreatedPrimaryLayout = nullptr;
		return nullptr;
	}

	CreatedWidget->AddToViewport();
	RegisterCreatedPrimaryLayoutWidget(CreatedWidget);

	return CreatedPrimaryLayout;
}

void UEMRUIManagerSubsystem::RegisterCreatedPrimaryLayoutWidget(UEMRCommonPrimaryLayoutWidget* InCreatedWidget)
{
	check(InCreatedWidget);

	CreatedPrimaryLayout = InCreatedWidget;
}


UEMRCommonPrimaryLayoutWidget* UEMRUIManagerSubsystem::GetPrimaryLayoutWidget() const
{
	return CreatedPrimaryLayout;
}

void UEMRUIManagerSubsystem::ClearPrimaryLayoutWidgets()
{
	if (!CreatedPrimaryLayout)
	{
		return;
	}

	CreatedPrimaryLayout->ClearAllWidgetStacks();
}


void UEMRUIManagerSubsystem::PushSoftWidgetToStackAsync(const FGameplayTag& InWidgetStackTag, TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> InSoftWidgetClass, TFunction<void(EAsyncPushWidgetState, UEMRFrontendCommonActivatableWidgetBase*)> AsyncPushStateCallback)
{
	check(!InSoftWidgetClass.IsNull());

	UE_LOG(LogTemp, Log, TEXT("[UIManager] PushSoftWidgetToStackAsync Tag=%s Class=%s"),
		*InWidgetStackTag.ToString(),
		*InSoftWidgetClass.ToSoftObjectPath().ToString());

	UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		InSoftWidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda(
			[InSoftWidgetClass, this, InWidgetStackTag, AsyncPushStateCallback]()
			{
				UClass* LoadedWidgetClass = InSoftWidgetClass.Get();
				if (!LoadedWidgetClass)
				{
					UE_LOG(LogTemp, Warning, TEXT("[UIManager] PushSoftWidgetToStackAsync failed: LoadedWidgetClass null for %s"),
						*InSoftWidgetClass.ToSoftObjectPath().ToString());
					return;
				}

				if (!CreatedPrimaryLayout)
				{
					UE_LOG(LogTemp, Warning, TEXT("[UIManager] PushSoftWidgetToStackAsync failed: PrimaryLayout missing. Tag=%s Class=%s"),
						*InWidgetStackTag.ToString(),
						*LoadedWidgetClass->GetPathName());
					return;
				}

				UCommonActivatableWidgetContainerBase* FoundWidgetStack = CreatedPrimaryLayout->FindWidgetStackByTag(InWidgetStackTag);
				if (!FoundWidgetStack)
				{
					UE_LOG(LogTemp, Warning, TEXT("[UIManager] PushSoftWidgetToStackAsync failed: Stack not found. Tag=%s Class=%s"),
						*InWidgetStackTag.ToString(),
						*LoadedWidgetClass->GetPathName());
					return;
				}

				UEMRFrontendCommonActivatableWidgetBase* CreatedWidget = FoundWidgetStack->AddWidget<UEMRFrontendCommonActivatableWidgetBase>(
					LoadedWidgetClass,
					[AsyncPushStateCallback](UEMRFrontendCommonActivatableWidgetBase& CreatedWidgetInstance)
					{
						AsyncPushStateCallback(EAsyncPushWidgetState::OnCreatedBeforePush, &CreatedWidgetInstance);
					}
					);

				UE_LOG(LogTemp, Log, TEXT("[UIManager] PushSoftWidgetToStackAsync pushed: Tag=%s Widget=%s"),
					*InWidgetStackTag.ToString(),
					*GetNameSafe(CreatedWidget));

				AsyncPushStateCallback(EAsyncPushWidgetState::AfterPush, CreatedWidget);
			}
			));
}

void UEMRUIManagerSubsystem::PushOrReplaceSoftWidgetToStackAsync(const FGameplayTag& InWidgetStackTag, TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> InSoftWidgetClass, TFunction<void(EAsyncPushWidgetState, UEMRFrontendCommonActivatableWidgetBase*)> AsyncPushStateCallback)
{
	if (InSoftWidgetClass.IsNull())
	{
		return;
	}

	UEMRCommonPrimaryLayoutWidget* PrimaryLayout = GetPrimaryLayoutWidget();
	if (!PrimaryLayout)
	{
		PushSoftWidgetToStackAsync(InWidgetStackTag, InSoftWidgetClass, AsyncPushStateCallback);
		return;
	}

	UCommonActivatableWidgetContainerBase* WidgetStack = PrimaryLayout->FindWidgetStackByTag(InWidgetStackTag);
	if (!WidgetStack)
	{
		PushSoftWidgetToStackAsync(InWidgetStackTag, InSoftWidgetClass, AsyncPushStateCallback);
		return;
	}

	const FSoftObjectPath TargetPath = InSoftWidgetClass.ToSoftObjectPath();
	UCommonActivatableWidget* ExistingTarget = nullptr;

	TArray<UCommonActivatableWidget*> ExistingWidgets = WidgetStack->GetWidgetList();
	for (UCommonActivatableWidget* ExistingWidget : ExistingWidgets)
	{
		if (!ExistingWidget)
		{
			continue;
		}

		if (ExistingWidget->GetClass()->GetPathName() == TargetPath.ToString())
		{
			ExistingTarget = ExistingWidget;
			continue;
		}

		WidgetStack->RemoveWidget(*ExistingWidget);
	}

	if (ExistingTarget)
	{
		if (UEMRFrontendCommonActivatableWidgetBase* ExistingFrontendWidget = Cast<UEMRFrontendCommonActivatableWidgetBase>(ExistingTarget))
		{
			AsyncPushStateCallback(EAsyncPushWidgetState::AfterPush, ExistingFrontendWidget);
		}
		return;
	}

	PushSoftWidgetToStackAsync(InWidgetStackTag, InSoftWidgetClass, AsyncPushStateCallback);
}

void UEMRUIManagerSubsystem::RemoveWidgetFromStack(const FGameplayTag& InWidgetStackTag, UEMRFrontendCommonActivatableWidgetBase* WidgetToRemove)
{
	if (!WidgetToRemove)
	{
		return;
	}

	if (!CreatedPrimaryLayout)
	{
		return;
	}

	UCommonActivatableWidgetContainerBase* FoundWidgetStack = CreatedPrimaryLayout->FindWidgetStackByTag(InWidgetStackTag);
	if (!FoundWidgetStack)
	{
		return;
	}
	
	WidgetToRemove->DeactivateWidget();
	FoundWidgetStack->RemoveWidget(*WidgetToRemove);
}

void UEMRUIManagerSubsystem::RemoveAllWidgetsFromStack(const FGameplayTag& InWidgetStackTag)
{
	if (!CreatedPrimaryLayout)
	{
		return;
	}

	UCommonActivatableWidgetContainerBase* WidgetStack = CreatedPrimaryLayout->FindWidgetStackByTag(InWidgetStackTag);
	if (!WidgetStack)
	{
		return;
	}

	const TArray<UCommonActivatableWidget*> WidgetList = WidgetStack->GetWidgetList();
	for (UCommonActivatableWidget* Widget : WidgetList)
	{
		if (!Widget)
		{
			continue;
		}

		Widget->DeactivateWidget();
		WidgetStack->RemoveWidget(*Widget);
	}
}


void UEMRUIManagerSubsystem::PushConfirmScreenToModalStackAsync(EConfirmScreenType InScreenType, const FText& InScreenTitle, const FText& InScreenMessage, TFunction<void(EConfirmScreenButtonType)> ButtonClickedCallback)
{
	UConfirmScreenInfoObject* CreatedScreenInfoObject = nullptr;

	switch (InScreenType)
	{
		case EConfirmScreenType::Ok:
		CreatedScreenInfoObject = UConfirmScreenInfoObject::CreateOkScreen(InScreenTitle, InScreenMessage);
			break;

		case EConfirmScreenType::YesNo:
		CreatedScreenInfoObject = UConfirmScreenInfoObject::CreateYesNoScreen(InScreenTitle, InScreenMessage);
			break;

		case EConfirmScreenType::OkCancel:
		CreatedScreenInfoObject = UConfirmScreenInfoObject::CreateOkCancelScreen(InScreenTitle, InScreenMessage);
			break;

		case EConfirmScreenType::Unknown:
			break;

		default:
			break;
	}

	check(CreatedScreenInfoObject);

	UE_LOG(LogTemp, Log, TEXT("[UIManager] PushConfirmScreenToModalStackAsync Type=%d Title='%s' Message='%s'"),
		static_cast<int32>(InScreenType),
		*InScreenTitle.ToString(),
		*InScreenMessage.ToString());

	PushSoftWidgetToStackAsync(
		EMRTags::UI::WidgetStack::Modal,
		UEMRFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(EMRTags::UI::Widgets::ConfirmScreen),
		[CreatedScreenInfoObject, ButtonClickedCallback](EAsyncPushWidgetState InPushState, UEMRFrontendCommonActivatableWidgetBase* PushedWidget)
		{
			if (InPushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				UEMRConfirmScreenWidget* CreatedConfirmScreen = CastChecked<UEMRConfirmScreenWidget>(PushedWidget);
				CreatedConfirmScreen->InitConfirmScreen(CreatedScreenInfoObject, ButtonClickedCallback);
			}
		});
}
