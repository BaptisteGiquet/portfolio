#include "UI/AGASSLoadingScreenWidget.h"

#include "Subsystems/AGASSSessionSubsystem.h"
#include "UI/AGASSFrontendButtonBase.h"

UAGASSLoadingScreenWidget::UAGASSLoadingScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
	OverrideBackActionDisplayName = NSLOCTEXT("AGASSFrontend", "LoadingBackActionName", "Return To Frontend");
}

FText UAGASSLoadingScreenWidget::GetTitleText() const
{
	return NSLOCTEXT("AGASSFrontend", "LoadingTitle", "Connecting");
}

void UAGASSLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveWidgetByName(Button_ReturnToFrontend, TEXT("Button_ReturnToFrontend"));

	if (Button_ReturnToFrontend != nullptr)
	{
		Button_ReturnToFrontend->OnClicked().RemoveAll(this);
		Button_ReturnToFrontend->OnClicked().AddUObject(this, &ThisClass::HandleCancelClicked);
	}
}

void UAGASSLoadingScreenWidget::BuildFallbackScreenContent(UVerticalBox* ContentBox)
{
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "LoadingBody", "AGASS is creating, joining, or recovering a session. Failures return the user to FrontendMap automatically."), 16);
	Button_ReturnToFrontend = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "LoadingCancelButton", "Return To Frontend"), TEXT("Button_ReturnToFrontend"));
}

bool UAGASSLoadingScreenWidget::NativeOnHandleBackAction()
{
	HandleCancelClicked();
	return true;
}

UWidget* UAGASSLoadingScreenWidget::ResolveInitialFocusTarget() const
{
	return Button_ReturnToFrontend;
}

void UAGASSLoadingScreenWidget::HandleCancelClicked()
{
	if (UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		SessionSubsystem->ReturnToFrontend(TEXT("Cancelled by player."), true);
	}
}
