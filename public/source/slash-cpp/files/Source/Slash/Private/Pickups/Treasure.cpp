
#include "Pickups/Treasure.h"
#include "Characters/SlashMyCharacter.h"

void ATreasure::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	IPickupInterface* PickupInterface = Cast<IPickupInterface>(OtherActor);
	if (PickupInterface)
	{
		PickupInterface->AddGold(this);
		PlayPickupSoundEffect();
		PlayPickupVisualEffect();
		Destroy();
	}
}

