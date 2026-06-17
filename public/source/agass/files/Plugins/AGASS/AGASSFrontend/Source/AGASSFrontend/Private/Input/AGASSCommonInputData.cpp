#include "Input/AGASSCommonInputData.h"

#include "Engine/DataTable.h"
#include "UObject/ConstructorHelpers.h"

UAGASSCommonUIInputData::UAGASSCommonUIInputData()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> GenericInputActions(TEXT("/CommonUI/GenericInputActionDataTable.GenericInputActionDataTable"));
	if (GenericInputActions.Succeeded())
	{
		DefaultClickAction.DataTable = GenericInputActions.Object;
		DefaultClickAction.RowName = TEXT("GenericAccept");
		DefaultBackAction.DataTable = GenericInputActions.Object;
		DefaultBackAction.RowName = TEXT("GenericBack");
	}
}

UAGASSMouseKeyboardControllerData::UAGASSMouseKeyboardControllerData()
{
	InputType = ECommonInputType::MouseAndKeyboard;
}

UAGASSGamepadControllerData::UAGASSGamepadControllerData()
{
	InputType = ECommonInputType::Gamepad;
	GamepadName = FCommonInputDefaults::GamepadGeneric;
	GamepadDisplayName = NSLOCTEXT("AGASSFrontend", "GenericGamepadDisplayName", "Generic Gamepad");
	GamepadCategory = NSLOCTEXT("AGASSFrontend", "GenericGamepadCategory", "Gamepad");
	GamepadPlatformName = NSLOCTEXT("AGASSFrontend", "WindowsGamepadPlatform", "Windows");
}
