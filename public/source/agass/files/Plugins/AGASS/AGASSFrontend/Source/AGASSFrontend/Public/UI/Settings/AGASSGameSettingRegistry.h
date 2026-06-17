#pragma once

#include "CoreMinimal.h"
#include "DataSource/GameSettingDataSourceDynamic.h"
#include "GameSettingRegistry.h"
#include "Settings/AGASSSettingsLocal.h"
#include "AGASSGameSettingRegistry.generated.h"

class ULocalPlayer;

#define GET_LOCAL_SETTINGS_FUNCTION_PATH(FunctionOrPropertyName)						\
	MakeShared<FGameSettingDataSourceDynamic>(TArray<FString>({							\
		GET_FUNCTION_NAME_STRING_CHECKED(UAGASSLocalPlayer, GetLocalSettings),			\
		GET_FUNCTION_NAME_STRING_CHECKED(UAGASSSettingsLocal, FunctionOrPropertyName)	\
	}))

UCLASS()
class AGASSFRONTEND_API UAGASSGameSettingRegistry : public UGameSettingRegistry
{
	GENERATED_BODY()

public:
	static const FName VideoCollection;
	static const FName AudioCollection;
	static const FName GameplayCollection;
	static const FName ControlsCollection;
	static const FName MouseKeyboardCollection;
	static const FName GamepadCollection;
	static const FName InterfaceCollection;
	static const FName KeyboardBindingsSection;
	static const FName GamepadBindingsSection;
	static const FName RebindAdjustDistanceKeyboard;
	static const FName RebindInteractKeyboard;
	static const FName RebindJumpKeyboard;
	static const FName RebindMoveLeftKeyboard;
	static const FName RebindMoveRightKeyboard;
	static const FName RebindMoveForwardKeyboard;
	static const FName RebindMoveBackwardKeyboard;
	static const FName RebindAdjustDistanceDecreaseGamepad;
	static const FName RebindAdjustDistanceIncreaseGamepad;
	static const FName RebindInteractGamepad;
	static const FName RebindJumpGamepad;

	virtual void SaveChanges() override;

protected:
	virtual void OnInitialize(ULocalPlayer* InLocalPlayer) override;
};

#undef GET_LOCAL_SETTINGS_FUNCTION_PATH
