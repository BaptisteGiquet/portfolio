#include "UI/AGASSActivatableWidget.h"

#include "Engine/GameInstance.h"
#include "Subsystems/AGASSUIManagerSubsystem.h"
#include "UI/AGASSFrontendPrimaryLayoutWidget.h"

UAGASSActivatableWidget::UAGASSActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRestoreFocus = true;
	bSupportsActivationFocus = true;
	SetIsFocusable(true);
}

UAGASSUIManagerSubsystem* UAGASSActivatableWidget::GetUIManager() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UAGASSUIManagerSubsystem>();
	}

	return nullptr;
}

UAGASSFrontendPrimaryLayoutWidget* UAGASSActivatableWidget::GetPrimaryGameLayout() const
{
	if (UAGASSUIManagerSubsystem* UIManager = GetUIManager())
	{
		return UIManager->GetRootLayout(GetOwningLocalPlayer());
	}

	return nullptr;
}
