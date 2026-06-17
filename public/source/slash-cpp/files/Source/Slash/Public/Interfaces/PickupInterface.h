
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PickupInterface.generated.h"

class ATreasure;
class ASoul;
class AItem;

UINTERFACE()
class UPickupInterface : public UInterface
{
	GENERATED_BODY()
};


class SLASH_API IPickupInterface
{
	GENERATED_BODY()
	
public:
	virtual void SetOverlappingItem(AItem* InItem);
	virtual void AddSouls(ASoul* InSoul);
	virtual void AddGold(ATreasure* InTreasure);
};
