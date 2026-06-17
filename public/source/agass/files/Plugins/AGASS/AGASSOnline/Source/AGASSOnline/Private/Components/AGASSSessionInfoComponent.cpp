#include "Components/AGASSSessionInfoComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UAGASSSessionInfoComponent::UAGASSSessionInfoComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAGASSSessionInfoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InviteCode);
	DOREPLIFETIME(ThisClass, bInviteCodeRegenerationInProgress);
}

void UAGASSSessionInfoComponent::SetInviteCode(const FString& NewInviteCode)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || InviteCode.Equals(NewInviteCode, ESearchCase::CaseSensitive))
	{
		return;
	}

	InviteCode = NewInviteCode;
	SessionInfoChangedEvent.Broadcast();
}

void UAGASSSessionInfoComponent::SetInviteCodeRegenerationInProgress(const bool bNewInviteCodeRegenerationInProgress)
{
	if (GetOwner() == nullptr
		|| !GetOwner()->HasAuthority()
		|| bInviteCodeRegenerationInProgress == bNewInviteCodeRegenerationInProgress)
	{
		return;
	}

	bInviteCodeRegenerationInProgress = bNewInviteCodeRegenerationInProgress;
	SessionInfoChangedEvent.Broadcast();
}

void UAGASSSessionInfoComponent::OnRep_InviteCode()
{
	SessionInfoChangedEvent.Broadcast();
}

void UAGASSSessionInfoComponent::OnRep_InviteCodeRegenerationInProgress()
{
	SessionInfoChangedEvent.Broadcast();
}
