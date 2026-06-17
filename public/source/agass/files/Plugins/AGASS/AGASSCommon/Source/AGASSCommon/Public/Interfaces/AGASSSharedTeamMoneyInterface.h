#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AGASSSharedTeamMoneyInterface.generated.h"

UINTERFACE(Blueprintable)
class AGASSCOMMON_API UAGASSSharedTeamMoneyInterface : public UInterface
{
	GENERATED_BODY()
};

class AGASSCOMMON_API IAGASSSharedTeamMoneyInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AGASS|Economy")
	int32 GetAGASSCurrentSharedMoney() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AGASS|Economy")
	bool CanAGASSAffordSharedMoney(int32 Amount) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AGASS|Economy")
	bool TrySpendAGASSSharedMoney(int32 Amount);
};
