#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AGASSTowerBootstrapSubsystem.generated.h"

UCLASS()
class AGASSTOWER_API UAGASSTowerBootstrapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	static bool IsGameplaySessionWorld(const UWorld& World);

private:
	static void EnsureDefaultObjective(UWorld& World);
	static void InitializeSessionEvents(UWorld& World);
};
