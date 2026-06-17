#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AGASSTeamWalletComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAGASSTeamWalletBalanceChangedSignature, int32, PreviousBalance, int32, NewBalance);

UCLASS(ClassGroup = (AGASS), meta = (BlueprintSpawnableComponent))
class AGASSECONOMY_API UAGASSTeamWalletComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAGASSTeamWalletComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "AGASS|Economy")
	int32 GetCurrentBalance() const;

	UFUNCTION(BlueprintPure, Category = "AGASS|Economy")
	bool CanAfford(int32 Amount) const;

	UFUNCTION(BlueprintCallable, Category = "AGASS|Economy")
	bool TrySpendMoney(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "AGASS|Economy")
	void AddMoney(int32 Amount);

	UPROPERTY(BlueprintAssignable, Category = "AGASS|Economy")
	FAGASSTeamWalletBalanceChangedSignature OnBalanceChanged;

private:
	UFUNCTION()
	void OnRep_CurrentBalance(int32 PreviousBalance);

	void SetCurrentBalance(int32 NewBalance);
	void EmitBalanceDeltaEvents(int32 PreviousBalance, int32 NewBalance);

	UPROPERTY(ReplicatedUsing = OnRep_CurrentBalance)
	int32 CurrentBalance = 0;

	bool bInitializedFromSettings = false;
	bool bHasProcessedInitialBalanceSnapshot = false;
};
