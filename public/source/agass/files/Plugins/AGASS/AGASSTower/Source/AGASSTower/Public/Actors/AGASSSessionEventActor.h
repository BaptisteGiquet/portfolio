#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGASSSessionEventActor.generated.h"

class UAGASSSessionEventManagerComponent;

UCLASS(Abstract, Blueprintable)
class AGASSTOWER_API AAGASSSessionEventActor : public AActor
{
	GENERATED_BODY()

public:
	AAGASSSessionEventActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeEventInstance(const FGuid& InInstanceId, const FString& InEventId, UAGASSSessionEventManagerComponent* InOwningManager);

	UFUNCTION(BlueprintPure, Category = "AGASS|Events")
	const FGuid& GetEventInstanceId() const
	{
		return EventInstanceId;
	}

	UFUNCTION(BlueprintPure, Category = "AGASS|Events")
	const FString& GetEventId() const
	{
		return EventId;
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AGASS|Events")
	void RequestFinishEvent();

	UFUNCTION(BlueprintNativeEvent, Category = "AGASS|Events")
	void HandleEventActivated();
	virtual void HandleEventActivated_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "AGASS|Events")
	void HandleEventEnded();
	virtual void HandleEventEnded_Implementation();

protected:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Events")
	FGuid EventInstanceId;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Events")
	FString EventId;

	TWeakObjectPtr<UAGASSSessionEventManagerComponent> OwningEventManager;
};
