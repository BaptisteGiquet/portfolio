#include "Environment/EMRWaitingRoomArea.h"

#include "Components/BoxComponent.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"

AEMRWaitingRoomArea::AEMRWaitingRoomArea()
{
    AreaVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("AreaVolume"));
    WaitingRoomManager = CreateDefaultSubobject<UEMRWaitingRoomManagerComponent>(TEXT("WaitingRoomManager"));
    SetRootComponent(AreaVolume);

	bReplicates = true;
	
    AreaVolume->SetBoxExtent(FVector(200.f));
    AreaVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
