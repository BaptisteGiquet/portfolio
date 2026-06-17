#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AGASSGameInstance.generated.h"

UCLASS()
class AGASS_API UAGASSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
};
