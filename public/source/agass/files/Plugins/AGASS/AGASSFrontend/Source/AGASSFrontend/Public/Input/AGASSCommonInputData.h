#pragma once

#include "CommonInputBaseTypes.h"
#include "AGASSCommonInputData.generated.h"

UCLASS()
class AGASSFRONTEND_API UAGASSCommonUIInputData : public UCommonUIInputData
{
	GENERATED_BODY()

public:
	UAGASSCommonUIInputData();
};

UCLASS()
class AGASSFRONTEND_API UAGASSMouseKeyboardControllerData : public UCommonInputBaseControllerData
{
	GENERATED_BODY()

public:
	UAGASSMouseKeyboardControllerData();
};

UCLASS()
class AGASSFRONTEND_API UAGASSGamepadControllerData : public UCommonInputBaseControllerData
{
	GENERATED_BODY()

public:
	UAGASSGamepadControllerData();
};
