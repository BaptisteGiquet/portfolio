#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRHospitalExit.generated.h"

class UArrowComponent;
class USphereComponent;
class AEMRPatient;
class AEMRPlayerCharacter;
class UPrimitiveComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FEMRHospitalExitPlayerOverlapSignature, AEMRPlayerCharacter*);

/**
 * Marker actor used as the navigation target for patients leaving the hospital.
 */
UCLASS()
class LOD_API AEMRHospitalExit : public AActor
{
    GENERATED_BODY()

public:
    AEMRHospitalExit();

    UFUNCTION(BlueprintCallable, Category = "EMR|Navigation")
    FTransform GetExitTransform() const;

    FEMRHospitalExitPlayerOverlapSignature& OnPlayerExitOverlapNative() { return PlayerExitOverlapDelegate; }

protected:
    UFUNCTION()
    void HandleExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|Components")
    TObjectPtr<UArrowComponent> ExitPoint;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Components")
    TObjectPtr<USphereComponent> ExitTrigger;

private:
    FEMRHospitalExitPlayerOverlapSignature PlayerExitOverlapDelegate;
};
