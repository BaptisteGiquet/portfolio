
#include "HUD/SlashHUD.h"

#include "Blueprint/UserWidget.h"
#include "HUD/SlashOverlay.h"

TObjectPtr<USlashOverlay> ASlashHUD::GetSlashOverlay()
{
	return SlashOverlay;
}

void ASlashHUD::BeginPlay()
{
	Super::BeginPlay();
	TObjectPtr<UWorld> World = GetWorld();
	if (World)
	{
		TObjectPtr<APlayerController> PlayerController = World->GetFirstPlayerController();
		if (PlayerController && SlashOverlayClass)
		{
			SlashOverlay = CreateWidget<USlashOverlay>(PlayerController, SlashOverlayClass);
			SlashOverlay->AddToViewport();
		}
	}
}
