#pragma once

UENUM(BlueprintType)
enum class EItemState : uint8
{
	//UMETA(DisplayName) to have a simplier name in blueprint
	EIS_Hovering UMETA(DisplayName = "Hovering"),
	EIS_Equipped UMETA(DisplayName = "Equipped"),
};
