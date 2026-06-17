
#include "Items/Item.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Interfaces/PickupInterface.h"
#include "Kismet/GameplayStatics.h"


AItem::AItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMeshComponent"));
	RootComponent = ItemMesh;
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	OverlapSphereDetection = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	OverlapSphereDetection->SetupAttachment(GetRootComponent());

	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	HitBox->SetupAttachment(GetRootComponent());

	HoveringVisualEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	HoveringVisualEffect->SetupAttachment(GetRootComponent());
}

void AItem::BeginPlay()
{
	Super::BeginPlay();

	//Bind this object as an observer of the delegate OnComponentBeginOverlap
	//When the delegate is 
	OverlapSphereDetection->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnSphereBeginOverlap);
	OverlapSphereDetection->OnComponentEndOverlap.AddDynamic(this, &AItem::OnSphereEndOverlap);
	HitBox->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnHitBoxBeginOverlap);
}

float AItem::GetRunningTime() const
{
	return RunningTime;
}

bool AItem::GetEnableFloating() const
{
	return bEnableFloating;
}

void AItem::SetEnableFloating(bool InEnableFloat)
{
	bEnableFloating = InEnableFloat;
}

void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RunningTime += DeltaTime;

	if (ItemState == EItemState::EIS_Hovering)
	{
		AddActorWorldOffset(FVector(0.f, 0.f, TransformedSin()));
	}
}



float AItem::GetAmplitude() const
{
	return Amplitude;
}

void AItem::SetAmplitude(float InAmplitude)
{
	Amplitude = InAmplitude;
}

float AItem::GetPeriod() const
{
	return Period;
}

void AItem::SetPeriod(float InPeriod)
{
	Period = InPeriod;
}

float AItem::TransformedSin()
{
	return Amplitude * FMath::Sin(RunningTime * Period);
}

float AItem::TransformedCos()
{
	return Amplitude * FMath::Cos(RunningTime * Period);
}

//Callback function called by delegate OnComponentOverlap
void AItem::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor)
	{
		IPickupInterface* PickupInterface = Cast<IPickupInterface>(OtherActor);
		if (PickupInterface)
		{
			PickupInterface->SetOverlappingItem(this);
		}
	}
}

void AItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		IPickupInterface* PickupInterface = Cast<IPickupInterface>(OtherActor);
		if (PickupInterface)
		{
			PickupInterface->SetOverlappingItem(nullptr);
		}
	}
}

void AItem::OnHitBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
}

void AItem::PlayPickupVisualEffect()
{
	if (PickupVisualEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PickupVisualEffect, GetActorLocation());
	}
}

void AItem::PlayPickupSoundEffect()
{
	if (PickupSoundEffect)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSoundEffect, GetActorLocation());
	}
}


