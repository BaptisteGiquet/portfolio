

#include "MobaCrosshairWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GAS/MobaGameplayTags.h"


void UMobaCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Image_Crosshair->SetVisibility(ESlateVisibility::Hidden);
	
	PlayerController = GetOwningPlayer();
	
	OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPlayerPawn());	
	
	if (OwnerASC)
	{
		OwnerASC->RegisterGameplayTagEvent(MobaStatusTags::Status_Crosshair).AddUObject(this, &ThisClass::CrosshairTagUpdated);
		OwnerASC->GenericGameplayEventCallbacks.Add(MobaTargetTags::Target_Updated).AddUObject(this, &ThisClass::AimTargetUpdated);
	}

	CanvasSlot_Crosshair = Cast<UCanvasPanelSlot>(Slot);
	if (!CanvasSlot_Crosshair)
	{
		UE_LOG(LogTemp, Error, TEXT("[MobaCrosshairWidget] Needs to be parented under a canvas panel"))
	}
}


void UMobaCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);


	if (Image_Crosshair	&& Image_Crosshair->GetVisibility() == ESlateVisibility::Visible)
	{
		UpdateCrosshairPosition();		
	}	
}


void UMobaCrosshairWidget::CrosshairTagUpdated(const FGameplayTag CrosshairTag, int32 CrosshairTagCount)
{
	bIsCrosshairActive = CrosshairTagCount > 0;
	if (bIsCrosshairActive)
	{
		Image_Crosshair->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		Image_Crosshair->SetVisibility(ESlateVisibility::Hidden);
	}
}


void UMobaCrosshairWidget::UpdateCrosshairPosition()
{
	if (!PlayerController || !CanvasSlot_Crosshair) { return; }
	
	float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	
	int32 SizeX = 0;
	int32 SizeY = 0;
	PlayerController->GetViewportSize(SizeX, SizeY);

	if (!AimTarget)
	{
		FVector2D ViewportSize = FVector2D(SizeX, SizeY);
		CanvasSlot_Crosshair->SetPosition((ViewportSize / 2) / ViewportScale);
		return;
	}
	
	FVector2D TargetScreenLocation = FVector2D::ZeroVector;
	PlayerController->ProjectWorldLocationToScreen(AimTarget->GetActorLocation(), TargetScreenLocation);
	
	// Make sure the crosshair doesn't go out of screen
	if (TargetScreenLocation.X > 0 && TargetScreenLocation.Y < SizeX && TargetScreenLocation.Y > 0 && TargetScreenLocation.Y < SizeY)
	{
		CanvasSlot_Crosshair->SetPosition(TargetScreenLocation / ViewportScale);
	}
}


void UMobaCrosshairWidget::AimTargetUpdated(const FGameplayEventData* EventData)
{
	AimTarget = EventData->Target;
	Image_Crosshair->SetColorAndOpacity(AimTarget ? HasTargetColor : NoTargetColor);
}
