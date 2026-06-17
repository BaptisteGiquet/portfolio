
#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Soul.generated.h"


class UNiagaraSystem;

UCLASS()
class SLASH_API ASoul : public AItem
{
	GENERATED_BODY()

public:
	ASoul();
	virtual void Tick(float DeltaTime) override;
	int32 GetSoulsAmount();
	void SetSoulsAmount(int32 NumberOfSouls);
	
protected:
	virtual void BeginPlay() override;
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult) override;

private:
	UPROPERTY(EditAnywhere)
	int32 SoulsAmount;
};
