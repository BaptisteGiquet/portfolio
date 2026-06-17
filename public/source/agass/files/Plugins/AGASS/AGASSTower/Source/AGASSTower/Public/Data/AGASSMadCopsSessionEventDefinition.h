#pragma once

#include "CoreMinimal.h"
#include "Data/AGASSSessionEventDefinition.h"
#include "AGASSMadCopsSessionEventDefinition.generated.h"

class AAGASSMadCopsCarActor;
class AAGASSMadCopsOfficerCharacter;

UCLASS(BlueprintType)
class AGASSTOWER_API UAGASSMadCopsSessionEventDefinition : public UAGASSSessionEventDefinition
{
	GENERATED_BODY()

public:
	virtual void ConfigureEventActor_Implementation(AAGASSSessionEventActor* EventActor, UAGASSSessionEventManagerComponent* EventManager) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Classes")
	TSoftClassPtr<AAGASSMadCopsOfficerCharacter> OfficerCharacterClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Classes")
	TSoftClassPtr<AAGASSMadCopsCarActor> PoliceCarActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Gameplay", meta = (ClampMin = "0"))
	int32 BaseBribeCost = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Gameplay", meta = (ClampMin = "0"))
	int32 BribeCostIncreasePerActivation = 50;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Gameplay", meta = (ClampMin = "0.0", ToolTip = "Radius around the authored scan point used to search for support-linked towers."))
	float ScanRadius = 2500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Gameplay", meta = (ClampMin = "0.0", ToolTip = "How far the officer stops from the targeted tower before triggering the destruction action."))
	float OfficerTowerStandOffDistance = 160.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Gameplay", meta = (ClampMin = "0.0"))
	float OfficerNavigationAcceptanceRadius = 125.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Gameplay", meta = (ClampMin = "0.0"))
	float OfficerMoveTimeoutSeconds = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Gameplay", meta = (ClampMin = "0.0"))
	float CollapseImpulseStrength = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Timing", meta = (ClampMin = "0.0"))
	float SirenLeadInSeconds = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Timing", meta = (ClampMin = "0.0"))
	float CarArrivalDurationSeconds = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Timing", meta = (ClampMin = "0.0"))
	float ScanDurationSeconds = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Timing", meta = (ClampMin = "0.0"))
	float DestroyPauseSeconds = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Timing", meta = (ClampMin = "0.0"))
	float ReturnToCarDelaySeconds = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Timing", meta = (ClampMin = "0.0"))
	float CarDepartureDurationSeconds = 3.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Gameplay")
	FVector OfficerExitLocalOffset = FVector(150.f, 90.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Tracing", meta = (ClampMin = "0.0"))
	float GroundProjectionTraceHeight = 2500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Cops|Debug")
	bool bDrawDebugTowerDetection = false;
};
