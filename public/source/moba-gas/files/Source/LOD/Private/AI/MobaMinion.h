
#pragma once

#include "CoreMinimal.h"
#include "Character/MobaCharacter.h"
#include "MobaMinion.generated.h"




UCLASS()
class AMobaMinion : public AMobaCharacter
{
	GENERATED_BODY()

public:
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	bool IsActive();
	void Activate() const;
	void SetGoal(AActor* Goal);
	bool IsReadyToRespawn();
	
protected:
	virtual void OnRep_TeamID() override;
	virtual void OnDead() override;
	virtual void OnRespawn() override;

	
private:
	void PickSkinBasedOnTeamID();
	void OnRespawnCooldownEnded();

	bool bIsReadyToRespawn = false;
	
	FTimerHandle RespawnCooldownTimer;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	FName GoalBlackboardKeyName = TEXT("Goal");
	
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TMap<FGenericTeamId, TObjectPtr<USkeletalMesh>> SkinMapping;
};
