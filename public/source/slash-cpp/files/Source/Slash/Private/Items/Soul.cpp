

#include "Items/Soul.h"
#include "Interfaces/PickupInterface.h"


ASoul::ASoul()
{
	PrimaryActorTick.bCanEverTick = true;
}


void ASoul::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

int32 ASoul::GetSoulsAmount()
{
	return SoulsAmount;
}

void ASoul::SetSoulsAmount(int32 NumberOfSouls)
{
	SoulsAmount = NumberOfSouls;
}

void ASoul::BeginPlay()
{
	Super::BeginPlay();
}

void ASoul::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	if (OtherActor)
	{
		IPickupInterface* PickupInterface = Cast<IPickupInterface>(OtherActor);
		if (PickupInterface)
		{
			PickupInterface->AddSouls(this);
			PlayPickupVisualEffect();
			PlayPickupSoundEffect();
			Destroy();
		}
	}
}


