#include "Subsystems/AGASSUIPolicy.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Settings/AGASSFrontendDeveloperSettings.h"
#include "Subsystems/AGASSUIManagerSubsystem.h"
#include "UI/AGASSFrontendPrimaryLayoutWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSFrontendPolicy, Log, All);

UWorld* UAGASSUIPolicy::GetWorld() const
{
	if (const UAGASSUIManagerSubsystem* UIManager = GetOwningUIManager())
	{
		return UIManager->GetGameInstance() != nullptr ? UIManager->GetGameInstance()->GetWorld() : nullptr;
	}

	return nullptr;
}

UAGASSUIManagerSubsystem* UAGASSUIPolicy::GetOwningUIManager() const
{
	return GetTypedOuter<UAGASSUIManagerSubsystem>();
}

UAGASSFrontendPrimaryLayoutWidget* UAGASSUIPolicy::GetRootLayout(const ULocalPlayer* LocalPlayer) const
{
	const FAGASSRootViewportLayoutInfo* LayoutInfo = RootViewportLayouts.FindByKey(LocalPlayer);
	return LayoutInfo != nullptr ? LayoutInfo->RootLayout : nullptr;
}

void UAGASSUIPolicy::NotifyPlayerAdded(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr)
	{
		UE_LOG(LogAGASSFrontendPolicy, Warning, TEXT("NotifyPlayerAdded called with null LocalPlayer."));
		return;
	}

	UE_LOG(LogAGASSFrontendPolicy, Log, TEXT("NotifyPlayerAdded for %s. FrontendActive=%s ExistingLayout=%s"),
		*GetNameSafe(LocalPlayer),
		bFrontendActive ? TEXT("true") : TEXT("false"),
		*GetNameSafe(GetRootLayout(LocalPlayer)));

	if (!PlayerControllerChangedHandles.Contains(LocalPlayer))
	{
		TWeakObjectPtr<ULocalPlayer> WeakLocalPlayer(LocalPlayer);
		const FDelegateHandle Handle = LocalPlayer->OnPlayerControllerChanged().AddWeakLambda(
			this,
			[this, WeakLocalPlayer](APlayerController* NewPlayerController)
			{
				if (ULocalPlayer* ResolvedLocalPlayer = WeakLocalPlayer.Get())
				{
					HandlePlayerControllerChanged(ResolvedLocalPlayer, NewPlayerController);
				}
			});

		PlayerControllerChangedHandles.Add(LocalPlayer, Handle);
	}

	if (FAGASSRootViewportLayoutInfo* LayoutInfo = RootViewportLayouts.FindByKey(LocalPlayer))
	{
		if (!LayoutInfo->bAddedToViewport && LayoutInfo->RootLayout != nullptr)
		{
			AddLayoutToViewport(LocalPlayer, LayoutInfo->RootLayout);
			LayoutInfo->bAddedToViewport = true;
		}

		UE_LOG(LogAGASSFrontendPolicy, Log, TEXT("Reusing existing root layout %s for %s. AddedToViewport=%s"),
			*GetNameSafe(LayoutInfo->RootLayout),
			*GetNameSafe(LocalPlayer),
			LayoutInfo->bAddedToViewport ? TEXT("true") : TEXT("false"));
		return;
	}

	CreateLayoutWidget(LocalPlayer);
}

void UAGASSUIPolicy::NotifyPlayerRemoved(ULocalPlayer* LocalPlayer)
{
	if (FAGASSRootViewportLayoutInfo* LayoutInfo = RootViewportLayouts.FindByKey(LocalPlayer))
	{
		if (LayoutInfo->bAddedToViewport && LayoutInfo->RootLayout != nullptr)
		{
			RemoveLayoutFromViewport(LocalPlayer, LayoutInfo->RootLayout);
			LayoutInfo->bAddedToViewport = false;
		}
	}
}

void UAGASSUIPolicy::NotifyPlayerDestroyed(ULocalPlayer* LocalPlayer)
{
	NotifyPlayerRemoved(LocalPlayer);

	if (LocalPlayer != nullptr)
	{
		if (const FDelegateHandle* Handle = PlayerControllerChangedHandles.Find(LocalPlayer))
		{
			LocalPlayer->OnPlayerControllerChanged().Remove(*Handle);
			PlayerControllerChangedHandles.Remove(LocalPlayer);
		}
	}

	const int32 LayoutInfoIndex = RootViewportLayouts.IndexOfByKey(LocalPlayer);
	if (LayoutInfoIndex != INDEX_NONE)
	{
		UAGASSFrontendPrimaryLayoutWidget* Layout = RootViewportLayouts[LayoutInfoIndex].RootLayout;
		RootViewportLayouts.RemoveAt(LayoutInfoIndex);
		OnRootLayoutReleased(LocalPlayer, Layout);
	}
}

void UAGASSUIPolicy::SetFrontendActive(const bool bInFrontendActive)
{
	if (bFrontendActive == bInFrontendActive)
	{
		UE_LOG(LogAGASSFrontendPolicy, Log, TEXT("SetFrontendActive called with unchanged state %s."), bFrontendActive ? TEXT("true") : TEXT("false"));
		return;
	}

	UE_LOG(LogAGASSFrontendPolicy, Log, TEXT("SetFrontendActive changing from %s to %s."),
		bFrontendActive ? TEXT("true") : TEXT("false"),
		bInFrontendActive ? TEXT("true") : TEXT("false"));
	bFrontendActive = bInFrontendActive;

	if (UGameInstance* GameInstance = GetOwningUIManager() != nullptr ? GetOwningUIManager()->GetGameInstance() : nullptr)
	{
		for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
		{
			NotifyPlayerAdded(LocalPlayer);
		}
	}
}

void UAGASSUIPolicy::OnRootLayoutAddedToViewport(ULocalPlayer* LocalPlayer, UAGASSFrontendPrimaryLayoutWidget* Layout)
{
#if WITH_EDITOR
	if (GIsEditor && LocalPlayer != nullptr && LocalPlayer->IsPrimaryPlayer())
	{
		FSlateApplication::Get().SetUserFocusToGameViewport(0);
	}
#endif
}

void UAGASSUIPolicy::OnRootLayoutRemovedFromViewport(ULocalPlayer* LocalPlayer, UAGASSFrontendPrimaryLayoutWidget* Layout)
{
}

void UAGASSUIPolicy::OnRootLayoutReleased(ULocalPlayer* LocalPlayer, UAGASSFrontendPrimaryLayoutWidget* Layout)
{
}

void UAGASSUIPolicy::AddLayoutToViewport(ULocalPlayer* LocalPlayer, UAGASSFrontendPrimaryLayoutWidget* Layout)
{
	if (LocalPlayer == nullptr || Layout == nullptr || Layout->IsInViewport())
	{
		UE_LOG(LogAGASSFrontendPolicy, Warning, TEXT("AddLayoutToViewport skipped. LocalPlayer=%s Layout=%s IsInViewport=%s"),
			*GetNameSafe(LocalPlayer),
			*GetNameSafe(Layout),
			(Layout != nullptr && Layout->IsInViewport()) ? TEXT("true") : TEXT("false"));
		return;
	}

	Layout->SetPlayerContext(FLocalPlayerContext(LocalPlayer));
	Layout->AddToPlayerScreen(1000);
	UE_LOG(LogAGASSFrontendPolicy, Log, TEXT("Added root layout %s to player screen for %s."),
		*GetNameSafe(Layout),
		*GetNameSafe(LocalPlayer));
	OnRootLayoutAddedToViewport(LocalPlayer, Layout);
}

void UAGASSUIPolicy::RemoveLayoutFromViewport(ULocalPlayer* LocalPlayer, UAGASSFrontendPrimaryLayoutWidget* Layout)
{
	if (Layout == nullptr || !Layout->IsInViewport())
	{
		return;
	}

	Layout->RemoveFromParent();
	OnRootLayoutRemovedFromViewport(LocalPlayer, Layout);
}

void UAGASSUIPolicy::CreateLayoutWidget(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr)
	{
		UE_LOG(LogAGASSFrontendPolicy, Warning, TEXT("CreateLayoutWidget skipped. LocalPlayer=%s"),
			*GetNameSafe(LocalPlayer));
		return;
	}

	APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld());
	if (PlayerController == nullptr)
	{
		UE_LOG(LogAGASSFrontendPolicy, Warning, TEXT("CreateLayoutWidget failed for %s because PlayerController is null."),
			*GetNameSafe(LocalPlayer));
		return;
	}

	TSubclassOf<UAGASSFrontendPrimaryLayoutWidget> LayoutWidgetClass = GetLayoutWidgetClass(LocalPlayer);
	if (LayoutWidgetClass == nullptr || LayoutWidgetClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogAGASSFrontendPolicy, Warning, TEXT("CreateLayoutWidget failed for %s because LayoutWidgetClass is invalid: %s"),
			*GetNameSafe(LocalPlayer),
			*GetNameSafe(LayoutWidgetClass));
		return;
	}

	UAGASSFrontendPrimaryLayoutWidget* NewLayoutObject = CreateWidget<UAGASSFrontendPrimaryLayoutWidget>(PlayerController, LayoutWidgetClass);
	if (NewLayoutObject == nullptr)
	{
		UE_LOG(LogAGASSFrontendPolicy, Warning, TEXT("CreateLayoutWidget failed for %s because CreateWidget returned null. Class=%s"),
			*GetNameSafe(LocalPlayer),
			*GetNameSafe(LayoutWidgetClass));
		return;
	}

	UE_LOG(LogAGASSFrontendPolicy, Log, TEXT("Created root layout %s for %s using class %s."),
		*GetNameSafe(NewLayoutObject),
		*GetNameSafe(LocalPlayer),
		*GetNameSafe(LayoutWidgetClass));

	RootViewportLayouts.Emplace(LocalPlayer, NewLayoutObject, false);
	AddLayoutToViewport(LocalPlayer, NewLayoutObject);

	if (FAGASSRootViewportLayoutInfo* LayoutInfo = RootViewportLayouts.FindByKey(LocalPlayer))
	{
		LayoutInfo->bAddedToViewport = NewLayoutObject->IsInViewport();
	}

	if (UAGASSUIManagerSubsystem* UIManager = GetOwningUIManager())
	{
		UIManager->HandleRootLayoutReady(LocalPlayer);
	}
}

TSubclassOf<UAGASSFrontendPrimaryLayoutWidget> UAGASSUIPolicy::GetLayoutWidgetClass(ULocalPlayer* LocalPlayer) const
{
	const UAGASSFrontendDeveloperSettings* FrontendSettings = UAGASSFrontendDeveloperSettings::Get();
	TSubclassOf<UAGASSFrontendPrimaryLayoutWidget> LayoutClass = FrontendSettings != nullptr
		? FrontendSettings->FrontendPrimaryLayoutClass.LoadSynchronous()
		: nullptr;

	if (LayoutClass == nullptr)
	{
		LayoutClass = UAGASSFrontendPrimaryLayoutWidget::StaticClass();
	}

	return LayoutClass;
}

void UAGASSUIPolicy::HandlePlayerControllerChanged(ULocalPlayer* LocalPlayer, APlayerController* NewPlayerController)
{
	NotifyPlayerRemoved(LocalPlayer);

	if (FAGASSRootViewportLayoutInfo* LayoutInfo = RootViewportLayouts.FindByKey(LocalPlayer))
	{
		if (LayoutInfo->RootLayout != nullptr)
		{
			AddLayoutToViewport(LocalPlayer, LayoutInfo->RootLayout);
			LayoutInfo->bAddedToViewport = LayoutInfo->RootLayout->IsInViewport();
		}
	}
	else
	{
		CreateLayoutWidget(LocalPlayer);
		return;
	}

	if (UAGASSUIManagerSubsystem* UIManager = GetOwningUIManager())
	{
		UIManager->HandleRootLayoutReady(LocalPlayer);
	}
}
