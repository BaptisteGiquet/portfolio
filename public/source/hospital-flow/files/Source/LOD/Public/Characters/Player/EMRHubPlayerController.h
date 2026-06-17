#pragma once

#include "Characters/Player/EMRPlayerController.h"
#include "EMRHubPlayerController.generated.h"

UCLASS()
class LOD_API AEMRHubPlayerController : public AEMRPlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
};
