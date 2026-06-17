
#include "Widgets/MobaGameplayWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "MobaAbilityListViewWidget.h"
#include "MobaGameplayMenuWidget.h"
#include "MobaHUDResourceBarWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/WidgetSwitcher.h"
#include "GAS/MobaAbilitySystemComponent.h"
#include "GAS/Attributes/MobaAttributeSet.h"
#include "Shop/MobaShopWidget.h"


void UMobaGameplayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	OwnerAbilitySystemComponent = Cast<UMobaAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPlayerPawn()));
	if (!IsValid(OwnerAbilitySystemComponent)) { return; }
	
	if (OwnerAbilitySystemComponent->GetClass()->ImplementsInterface(UMobaAbilityInterface::StaticClass()))
	{
		const IMobaAbilityInterface* OwnerAbilityInterface = Cast<IMobaAbilityInterface>(OwnerAbilitySystemComponent);
		if (!OwnerAbilityInterface) { return; }

		ConfigureAbilities(OwnerAbilityInterface->GetMainAbilities());
	}

	GetOwningPlayer()->SetShowMouseCursor(false);
	SetFocusToGameOnly();
	
	if (Widget_HealthBar)
	{
		Widget_HealthBar->InitializeAndBindToAttributes(OwnerAbilitySystemComponent, UMobaAttributeSet::GetHealthAttribute(), UMobaAttributeSet::GetMaxHealthAttribute());
		Widget_ManaBar->InitializeAndBindToAttributes(OwnerAbilitySystemComponent, UMobaAttributeSet::GetManaAttribute(), UMobaAttributeSet::GetMaxManaAttribute());
	}

	if (Widget_Shop)
	{
		FWidgetTransform ShopTransform;
		ShopTransform.Scale = FVector2D::ZeroVector;
		Widget_Shop->SetRenderTransform(ShopTransform);

		Widget_Shop->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (Widget_GameplayMenu)
	{
		Widget_GameplayMenu->GetResumeButtonClickedEventDelegate().AddDynamic(this, &ThisClass::ToggleGameplayMenu);
	}
}




void UMobaGameplayWidget::ConfigureAbilities(const TMap<EAbilityInputID, TSubclassOf<UGameplayAbility>>& MainAbilities)
{
	ListView_Abilities->ConfigureAbilities(MainAbilities);
}


void UMobaGameplayWidget::ToggleShopVisibility()
{
	if (Widget_Shop->GetVisibility() == ESlateVisibility::HitTestInvisible)
	{
		OpenShop();
	}
	else
	{
		CloseShop();
	}
}


void UMobaGameplayWidget::ToggleGameplayMenu()
{
	if (!WidgetSwitcher_Main || !RootPanel_GameplayMenuWidget || !RootPanel_GameplayWidget) { return; }

	if (WidgetSwitcher_Main->GetActiveWidget() == RootPanel_GameplayMenuWidget)
	{
		WidgetSwitcher_Main->SetActiveWidget(RootPanel_GameplayWidget);
		SetOwningPawnInputEnabled(true);
		GetOwningPlayer()->SetShowMouseCursor(false);
		SetFocusToGameOnly();
	}
	else
	{
		ShowGameplayMenu();
	}
}


void UMobaGameplayWidget::ShowGameplayMenu()
{
	if (!WidgetSwitcher_Main || !RootPanel_GameplayMenuWidget) { return; }
	
	WidgetSwitcher_Main->SetActiveWidget(RootPanel_GameplayMenuWidget);
	SetOwningPawnInputEnabled(false);
	GetOwningPlayer()->SetShowMouseCursor(true);
	SetFocusToGameAndUI();
}


void UMobaGameplayWidget::SetGameplayMenuTitle(const FString& NewTitle)
{
	if (!Widget_GameplayMenu) { return; }
	
	Widget_GameplayMenu->SetTitleText(NewTitle);
}


void UMobaGameplayWidget::OpenShop()
{
	Widget_Shop->SetVisibility(ESlateVisibility::Visible);
	PlayAnimationForward(ShopPopupAnimation);
	GetOwningPlayer()->SetShowMouseCursor(true);
	SetFocusToGameAndUI();

	Widget_Shop->SetFocus();
}



void UMobaGameplayWidget::CloseShop()
{
	PlayAnimationReverse(ShopPopupAnimation);
	Widget_Shop->SetVisibility(ESlateVisibility::HitTestInvisible);
	GetOwningPlayer()->SetShowMouseCursor(false);
	SetFocusToGameOnly();
}



void UMobaGameplayWidget::SetOwningPawnInputEnabled(bool bIsPawnInputEnabled)
{
	if (bIsPawnInputEnabled)
	{
		GetOwningPlayerPawn()->EnableInput(GetOwningPlayer());
	}
	else
	{
		GetOwningPlayerPawn()->DisableInput(GetOwningPlayer());
	}
}



void UMobaGameplayWidget::SetFocusToGameAndUI()
{
	FInputModeGameAndUI GameAndUIInputMode;
	GameAndUIInputMode.SetHideCursorDuringCapture(false);
	
	GetOwningPlayer()->SetInputMode(GameAndUIInputMode);
}



void UMobaGameplayWidget::SetFocusToGameOnly()
{
	FInputModeGameOnly GameOnlyInputMode;

	GetOwningPlayer()->SetInputMode(GameOnlyInputMode);
}
