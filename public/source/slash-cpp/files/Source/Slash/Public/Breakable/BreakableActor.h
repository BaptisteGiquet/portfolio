
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/HitInterface.h"
#include "BreakableActor.generated.h"

struct FChaosBreakEvent;
class UCapsuleComponent;
class ATreasure;
class UGeometryCollectionComponent;

UCLASS()
class SLASH_API ABreakableActor : public AActor, public IHitInterface
{
	GENERATED_BODY()
	
public:	
	ABreakableActor();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
	virtual void GetHit_Implementation(const FVector& InImpactPoint, AActor* Hitter) override;

	UFUNCTION()
	void SpawnTreasure();

	UFUNCTION()
	void OnBreak(const FChaosBreakEvent& BreakEvent);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UCapsuleComponent* Capsule;

private:
	UPROPERTY(VisibleAnywhere)
	UGeometryCollectionComponent* GeometryCollection;

	//TSubclassOf force the type of TreasureClass to be from APickup type, but still gives us a UClass that we can use for SpawnActor
	//With this designer won't be able to chose anything else than APickup class from the blueprint detail panel
	//UPROPERTY(EditAnywhere)
	//TSubclassOf<APickup> TreasureClass;

	// We make an array that can contain APickup class objects
	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<ATreasure>> TreasureClasses;

	UPROPERTY(VisibleAnywhere)
	bool bBroken = false;
};
