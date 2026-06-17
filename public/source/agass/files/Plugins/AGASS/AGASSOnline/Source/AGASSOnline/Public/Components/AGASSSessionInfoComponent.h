#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AGASSSessionInfoComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FAGASSSessionInfoChangedEvent);

UCLASS(ClassGroup = (AGASS), meta = (BlueprintSpawnableComponent))
class AGASSONLINE_API UAGASSSessionInfoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAGASSSessionInfoComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	const FString& GetInviteCode() const
	{
		return InviteCode;
	}

	bool IsInviteCodeRegenerationInProgress() const
	{
		return bInviteCodeRegenerationInProgress;
	}

	void SetInviteCode(const FString& NewInviteCode);
	void SetInviteCodeRegenerationInProgress(bool bNewInviteCodeRegenerationInProgress);

	FAGASSSessionInfoChangedEvent& OnSessionInfoChanged()
	{
		return SessionInfoChangedEvent;
	}

private:
	UFUNCTION()
	void OnRep_InviteCode();

	UFUNCTION()
	void OnRep_InviteCodeRegenerationInProgress();

	UPROPERTY(ReplicatedUsing = OnRep_InviteCode)
	FString InviteCode;

	UPROPERTY(ReplicatedUsing = OnRep_InviteCodeRegenerationInProgress)
	bool bInviteCodeRegenerationInProgress = false;

	FAGASSSessionInfoChangedEvent SessionInfoChangedEvent;
};
