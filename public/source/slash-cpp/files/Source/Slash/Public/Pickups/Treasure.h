
#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Treasure.generated.h"


class ASlashMyCharacter;

UCLASS()
class SLASH_API ATreasure : public AItem
{
	GENERATED_BODY()

public:

	
protected:
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	
	UPROPERTY(VisibleAnywhere)
	ASlashMyCharacter* SlashCharacter;

	
private:
	UPROPERTY(EditAnywhere, BlueprintGetter = GetTreasureValue)
	int32 TreasureValue;


public:
	UFUNCTION(BlueprintPure)
	FORCEINLINE int32 GetTreasureValue() const { return TreasureValue; }
	
};
