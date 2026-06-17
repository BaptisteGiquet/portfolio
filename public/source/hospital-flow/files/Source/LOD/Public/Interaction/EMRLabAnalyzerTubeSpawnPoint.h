#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRLabAnalyzerTubeSpawnPoint.generated.h"

class USceneComponent;
class UArrowComponent;

UCLASS()
class LOD_API AEMRLabAnalyzerTubeSpawnPoint : public AActor
{
    GENERATED_BODY()

public:
    AEMRLabAnalyzerTubeSpawnPoint();

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer")
    USceneComponent* GetSpawnRoot() const { return SpawnRoot; }

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer")
    TObjectPtr<USceneComponent> SpawnRoot = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer")
    TObjectPtr<UArrowComponent> ArrowComponent = nullptr;
};
