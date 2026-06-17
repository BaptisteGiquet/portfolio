#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AGASSGameplayEventSubsystem.generated.h"

USTRUCT(BlueprintType)
struct AGASSCOMMON_API FAGASSGameplayEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AGASS|Gameplay Event")
	FName EventName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AGASS|Gameplay Event")
	int32 IntValue = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AGASS|Gameplay Event")
	float FloatValue = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AGASS|Gameplay Event")
	bool bFlagValue = false;
};

namespace AGASSGameplayEventNames
{
	inline const FName& SessionEntered()
	{
		static const FName Name(TEXT("SessionEntered"));
		return Name;
	}

	inline const FName& RunCompleted()
	{
		static const FName Name(TEXT("RunCompleted"));
		return Name;
	}

	inline const FName& MoneyEarned()
	{
		static const FName Name(TEXT("MoneyEarned"));
		return Name;
	}

	inline const FName& MoneySpent()
	{
		static const FName Name(TEXT("MoneySpent"));
		return Name;
	}

	inline const FName& ItemPurchased()
	{
		static const FName Name(TEXT("ItemPurchased"));
		return Name;
	}

	inline const FName& ItemSold()
	{
		static const FName Name(TEXT("ItemSold"));
		return Name;
	}

	inline const FName& ItemPlaced()
	{
		static const FName Name(TEXT("ItemPlaced"));
		return Name;
	}
}

DECLARE_MULTICAST_DELEGATE_OneParam(FAGASSGameplayEventReceivedEvent, const FAGASSGameplayEvent&);

UCLASS()
class AGASSCOMMON_API UAGASSGameplayEventSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void BroadcastGameplayEvent(const FAGASSGameplayEvent& GameplayEvent);
	void BroadcastGameplayEvent(FName EventName, int32 IntValue = 0, float FloatValue = 0.f, bool bFlagValue = false);

	static void BroadcastGameplayEvent(UObject* WorldContextObject, FName EventName, int32 IntValue = 0, float FloatValue = 0.f, bool bFlagValue = false);

	FAGASSGameplayEventReceivedEvent& OnGameplayEventReceived()
	{
		return GameplayEventReceivedEvent;
	}

private:
	FAGASSGameplayEventReceivedEvent GameplayEventReceivedEvent;
};
