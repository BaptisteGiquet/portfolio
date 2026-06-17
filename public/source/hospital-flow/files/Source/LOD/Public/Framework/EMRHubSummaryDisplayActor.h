#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRHubSummaryDisplayActor.generated.h"

class USceneComponent;
class UWidgetComponent;
class UUserWidget;

UCLASS()
class LOD_API AEMRHubSummaryDisplayActor : public AActor
{
    GENERATED_BODY()

public:
    AEMRHubSummaryDisplayActor();
    virtual void BeginPlay() override;

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|Hub|Summary")
    TObjectPtr<USceneComponent> Root = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Hub|Summary")
    TObjectPtr<UWidgetComponent> SummaryWidgetComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hub|Summary")
    TSubclassOf<UUserWidget> SummaryWidgetClass;

private:
    void InitializeSummaryWidget();
};

