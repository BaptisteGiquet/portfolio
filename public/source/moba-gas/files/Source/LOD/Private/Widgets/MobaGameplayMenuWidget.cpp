

#include "MobaGameplayMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"


void UMobaGameplayMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Button_MainMenu->OnClicked.AddDynamic(this, &ThisClass::BackToMainMenu);
	Button_QuitGame->OnClicked.AddDynamic(this, &ThisClass::QuitGame);
}


void UMobaGameplayMenuWidget::SetTitleText(const FString& NewTitle)
{
	if (!Text_MenuTitle) { return; }
	
	Text_MenuTitle->SetText(FText::FromString(NewTitle));
}


FOnButtonClickedEvent& UMobaGameplayMenuWidget::GetResumeButtonClickedEventDelegate()
{
	return Button_Resume->OnClicked;
}


void UMobaGameplayMenuWidget::BackToMainMenu()
{
	
}


void UMobaGameplayMenuWidget::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, true);
}
