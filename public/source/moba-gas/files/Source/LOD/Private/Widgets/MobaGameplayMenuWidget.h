
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "MobaGameplayMenuWidget.generated.h"


class UButton;
class UTextBlock;

UCLASS()
class LOD_API UMobaGameplayMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void SetTitleText(const FString& NewTitle);
	
	FOnButtonClickedEvent& GetResumeButtonClickedEventDelegate();
	
private:
	UFUNCTION()
	void BackToMainMenu();

	UFUNCTION()
	void QuitGame();
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_MenuTitle;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Resume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_MainMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_QuitGame;
};
