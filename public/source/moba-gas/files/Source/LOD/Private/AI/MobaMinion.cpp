
#include "AI/MobaMinion.h"

#include "MobaAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GAS/MobaAbilitySystemComponent.h"
#include "GAS/MobaGameplayTags.h"


void AMobaMinion::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	Super::SetGenericTeamId(NewTeamID);
	// Not necessary to do it on server, but we prefer to keep client/server visuals as close as possible
	PickSkinBasedOnTeamID();
}

void AMobaMinion::OnRep_TeamID()
{
	// Only on Client
	Super::OnRep_TeamID();
	PickSkinBasedOnTeamID();
}

void AMobaMinion::OnDead()
{
	Super::OnDead();
	if (!HasAuthority()) return;

	GetWorldTimerManager().ClearTimer(RespawnCooldownTimer);
	GetWorldTimerManager().SetTimer(RespawnCooldownTimer, this, &AMobaMinion::OnRespawnCooldownEnded, 10.f, false);
}

void AMobaMinion::OnRespawn()
{
	Super::OnRespawn();
	if (!HasAuthority()) return;
	
	bIsReadyToRespawn = false;
}

bool AMobaMinion::IsActive()
{
	// true if minion not dead
	return !GetAbilitySystemComponent()->HasMatchingGameplayTag(MobaStatusTags::Status_Dead);
}

void AMobaMinion::Activate() const
{
	GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(MobaStatusTags::Status_Dead));
}

void AMobaMinion::SetGoal(AActor* Goal)
{
	AMobaAIController* MinionAIController = Cast<AMobaAIController>(GetController());
	if (!IsValid(MinionAIController)) return;

	UBlackboardComponent* MinionBlackboardComponent = MinionAIController->GetBlackboardComponent();
	if (!MinionBlackboardComponent) return;

	MinionBlackboardComponent->SetValueAsObject(GoalBlackboardKeyName, Goal);
}

void AMobaMinion::PickSkinBasedOnTeamID()
{
	USkeletalMesh* MeshSkin = SkinMapping.FindRef(GetGenericTeamId());
	if (MeshSkin)
	{
		GetMesh()->SetSkeletalMesh(MeshSkin);
	}
}

bool AMobaMinion::IsReadyToRespawn()
{
	return bIsReadyToRespawn;
}

void AMobaMinion::OnRespawnCooldownEnded()
{
	bIsReadyToRespawn = true;
}
