#include "UI/AGASSGameMenuScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Subsystems/AGASSPlatformMenuSubsystem.h"
#include "Subsystems/AGASSSessionSubsystem.h"
#include "Subsystems/AGASSUIManagerSubsystem.h"
#include "UI/AGASSFrontendButtonBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSGameMenuScreen, Log, All);

namespace
{
	APlayerController* ResolveCurrentOwningPlayerController(const UCommonActivatableWidget* Widget)
	{
		if (Widget == nullptr)
		{
			return nullptr;
		}

		if (ULocalPlayer* OwningLocalPlayer = Widget->GetOwningLocalPlayer())
		{
			if (APlayerController* LocalPlayerController = OwningLocalPlayer->GetPlayerController(Widget->GetWorld()))
			{
				return LocalPlayerController;
			}
		}

		return Widget->GetOwningPlayer();
	}
}

UAGASSGameMenuScreenWidget::UAGASSGameMenuScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
	OverrideBackActionDisplayName = NSLOCTEXT("AGASSFrontend", "CloseGameMenuActionName", "Close");
}

FText UAGASSGameMenuScreenWidget::GetTitleText() const
{
	return NSLOCTEXT("AGASSFrontend", "GameMenuTitle", "Game Menu");
}

void UAGASSGameMenuScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveWidgetByName(Button_SteamInvite, TEXT("Button_SteamInvite"));
	ResolveWidgetByName(Button_Options, TEXT("Button_Options"));
	ResolveWidgetByName(Button_MainMenu, TEXT("Button_MainMenu"));
	ResolveWidgetByName(Button_RegenerateInviteCode, TEXT("Button_RegenerateInviteCode"));
	ResolveWidgetByName(Text_InviteCodeValue, TEXT("Text_InviteCodeValue"));

	if (Button_SteamInvite != nullptr)
	{
		Button_SteamInvite->OnClicked().RemoveAll(this);
		Button_SteamInvite->OnClicked().AddUObject(this, &ThisClass::HandleSteamInviteClicked);
	}

	if (Button_Options != nullptr)
	{
		Button_Options->OnClicked().RemoveAll(this);
		Button_Options->OnClicked().AddUObject(this, &ThisClass::HandleOptionsClicked);
	}

	if (Button_MainMenu != nullptr)
	{
		Button_MainMenu->OnClicked().RemoveAll(this);
		Button_MainMenu->OnClicked().AddUObject(this, &ThisClass::HandleMainMenuClicked);
	}

	if (Button_RegenerateInviteCode != nullptr)
	{
		Button_RegenerateInviteCode->OnClicked().RemoveAll(this);
		Button_RegenerateInviteCode->OnClicked().AddUObject(this, &ThisClass::HandleRegenerateInviteCodeClicked);
	}

	RefreshInviteCodeText();
	RefreshActionAvailability();
}

void UAGASSGameMenuScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	RefreshInviteCodeText();
	RefreshActionAvailability();
}

bool UAGASSGameMenuScreenWidget::NativeOnHandleBackAction()
{
	if (UAGASSUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->DismissGameMenu(GetOwningLocalPlayer());
		return true;
	}

	return Super::NativeOnHandleBackAction();
}

void UAGASSGameMenuScreenWidget::BuildFallbackScreenContent(UVerticalBox* ContentBox)
{
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "GameMenuInviteCodeLabel", "Session Invite Code"), 18);
	Text_InviteCodeValue = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_InviteCodeValue"));
	Text_InviteCodeValue->SetAutoWrapText(true);
	if (UVerticalBoxSlot* InviteCodeSlot = ContentBox->AddChildToVerticalBox(Text_InviteCodeValue))
	{
		InviteCodeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	Button_RegenerateInviteCode = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "RegenerateInviteCodeButton", "Regenerate Invite Code"), TEXT("Button_RegenerateInviteCode"));
	AddSpacer(ContentBox, 8.f);
	Button_SteamInvite = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "GameMenuSteamInviteButton", "Steam Invite"), TEXT("Button_SteamInvite"));
	Button_Options = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "GameMenuOptionsButton", "Options"), TEXT("Button_Options"));
	Button_MainMenu = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "GameMenuMainMenuButton", "Main Menu"), TEXT("Button_MainMenu"));
}

void UAGASSGameMenuScreenWidget::RefreshFromSessionState()
{
	Super::RefreshFromSessionState();

	RefreshInviteCodeText();
	RefreshActionAvailability();
}

UWidget* UAGASSGameMenuScreenWidget::ResolveInitialFocusTarget() const
{
	if (Button_SteamInvite != nullptr)
	{
		return Button_SteamInvite.Get();
	}

	if (Button_Options != nullptr)
	{
		return Button_Options.Get();
	}

	return nullptr;
}

#if WITH_EDITOR
void UAGASSGameMenuScreenWidget::GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const
{
	OutRequiredWidgets.Append({
		TEXT("Button_SteamInvite"),
		TEXT("Button_Options"),
		TEXT("Button_MainMenu"),
		TEXT("Button_RegenerateInviteCode"),
		TEXT("Text_InviteCodeValue")
	});
}
#endif

void UAGASSGameMenuScreenWidget::HandleSteamInviteClicked()
{
	APlayerController* const OwningPlayerController = ResolveCurrentOwningPlayerController(this);
	UE_LOG(
		LogAGASSGameMenuScreen,
		Display,
		TEXT("HandleSteamInviteClicked controller=%s localPlayer=%s platformMenuSubsystem=%d"),
		*GetNameSafe(OwningPlayerController),
		*GetNameSafe(GetOwningLocalPlayer()),
		GetPlatformMenuSubsystem() != nullptr);

	if (UAGASSPlatformMenuSubsystem* PlatformMenuSubsystem = GetPlatformMenuSubsystem())
	{
		if (!PlatformMenuSubsystem->CanShowSessionInviteUI(OwningPlayerController))
		{
			UE_LOG(LogAGASSGameMenuScreen, Warning, TEXT("HandleSteamInviteClicked ignored because the Steam invite dialog is not currently available."));
			return;
		}

		const bool bRequested = PlatformMenuSubsystem->RequestShowSessionInviteUI(OwningPlayerController);
		UE_LOG(
			LogAGASSGameMenuScreen,
			Display,
			TEXT("HandleSteamInviteClicked requestIssued=%d controller=%s"),
			bRequested,
			*GetNameSafe(OwningPlayerController));
	}
	else
	{
		UE_LOG(LogAGASSGameMenuScreen, Warning, TEXT("HandleSteamInviteClicked failed because PlatformMenuSubsystem was null."));
	}
}

void UAGASSGameMenuScreenWidget::HandleOptionsClicked()
{
	if (UAGASSUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->ShowOptionsScreenInGameMenu(GetOwningLocalPlayer());
	}
}

void UAGASSGameMenuScreenWidget::HandleMainMenuClicked()
{
	if (UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		const APlayerController* const OwningPlayerController = ResolveCurrentOwningPlayerController(this);
		const bool bDestroySession = OwningPlayerController != nullptr && OwningPlayerController->HasAuthority();
		SessionSubsystem->ReturnToFrontend(TEXT("Returned to the main menu."), bDestroySession);
	}
}

void UAGASSGameMenuScreenWidget::HandleRegenerateInviteCodeClicked()
{
	if (APlayerController* OwningPlayerController = ResolveCurrentOwningPlayerController(this))
	{
		if (UFunction* RequestFunction = OwningPlayerController->FindFunction(TEXT("RequestSessionInviteCodeRegeneration")))
		{
			OwningPlayerController->ProcessEvent(RequestFunction, nullptr);
		}
	}
}

void UAGASSGameMenuScreenWidget::RefreshInviteCodeText()
{
	if (Text_InviteCodeValue == nullptr)
	{
		return;
	}

	const FString InviteCode = GetSessionSubsystem() != nullptr ? GetSessionSubsystem()->GetCurrentInviteCode() : FString();
	Text_InviteCodeValue->SetText(InviteCode.IsEmpty()
		? NSLOCTEXT("AGASSFrontend", "InviteCodeUnavailable", "Unavailable")
		: FText::FromString(InviteCode));
}

void UAGASSGameMenuScreenWidget::RefreshActionAvailability()
{
	const UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	APlayerController* const OwningPlayerController = ResolveCurrentOwningPlayerController(this);
	const bool bHasNamedSession = SessionSubsystem != nullptr && SessionSubsystem->HasNamedSession();
	const bool bCanOpenSteamInvite = SessionSubsystem != nullptr
		&& bHasNamedSession
		&& GetPlatformMenuSubsystem() != nullptr
		&& GetPlatformMenuSubsystem()->CanShowSessionInviteUI(OwningPlayerController);
	const bool bCanReturnToMainMenu = SessionSubsystem != nullptr;
	const bool bCanRegenerateInviteCode = SessionSubsystem != nullptr && SessionSubsystem->CanRegenerateInviteCode();

	UE_LOG(
		LogAGASSGameMenuScreen,
		Verbose,
		TEXT("RefreshActionAvailability controller=%s hasNamedSession=%d canOpenSteamInvite=%d platformMenuSubsystem=%d"),
		*GetNameSafe(OwningPlayerController),
		bHasNamedSession,
		bCanOpenSteamInvite,
		GetPlatformMenuSubsystem() != nullptr);

	if (Button_SteamInvite != nullptr)
	{
		Button_SteamInvite->SetButtonText(NSLOCTEXT("AGASSFrontend", "GameMenuSteamInviteButton", "Steam Invite"));
		Button_SteamInvite->SetIsEnabled(bCanOpenSteamInvite);
	}

	if (Button_Options != nullptr)
	{
		Button_Options->SetIsEnabled(true);
	}

	if (Button_MainMenu != nullptr)
	{
		Button_MainMenu->SetIsEnabled(bCanReturnToMainMenu);
	}

	if (Button_RegenerateInviteCode != nullptr)
	{
		Button_RegenerateInviteCode->SetIsEnabled(bCanRegenerateInviteCode);
	}
}
