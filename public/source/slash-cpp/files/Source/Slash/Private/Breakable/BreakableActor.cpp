

#include "Breakable/BreakableActor.h"

#include "Algo/RandomShuffle.h"
#include "Components/CapsuleComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Pickups/Treasure.h"


ABreakableActor::ABreakableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	GeometryCollection = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollection"));
	SetRootComponent(GeometryCollection);
	GeometryCollection->SetGenerateOverlapEvents(true);
	GeometryCollection->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GeometryCollection->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	Capsule->SetupAttachment(GetRootComponent());
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void ABreakableActor::BeginPlay()
{
	Super::BeginPlay();
	//C++ version will occur only one time when the actor break
	GeometryCollection->OnChaosBreakEvent.AddDynamic(this, &ABreakableActor::OnBreak);
}

void ABreakableActor::OnBreak(const FChaosBreakEvent& BreakEvent)
{
	if (bBroken) return;
	bBroken = true;
	SpawnTreasure();
}


void ABreakableActor::GetHit_Implementation(const FVector& InImpactPoint, AActor* Hitter)
{
}

void ABreakableActor::SpawnTreasure()
{
	if (GetWorld() && TreasureClasses.Num() > 0)
	{
		FVector Location = GetActorLocation();
		const int32 Index = FMath::RandHelper(TreasureClasses.Num());
		GetWorld()->SpawnActor<ATreasure>(TreasureClasses[Index], GetActorLocation(), GetActorRotation());	
	}
}


void ABreakableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

