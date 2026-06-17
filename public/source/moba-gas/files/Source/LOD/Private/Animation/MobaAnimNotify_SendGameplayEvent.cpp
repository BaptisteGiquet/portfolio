
#include "Animation/MobaAnimNotify_SendGameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagsManager.h"



void UMobaAnimNotify_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (MeshComp == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SendGameplayEvent] MeshComp=nullptr"));
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (OwnerActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SendGameplayEvent] OwnerActor=nullptr"));
		return;
	}
	
	if (!EventTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SendGameplayEvent] EventTag invalid"));
		return;
	}

	const UAbilitySystemComponent* OwnerActorAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	
	if (OwnerActorAbilitySystemComponent)
	{
		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = OwnerActor;
		Payload.Target = OwnerActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, Payload);	
	}
}


FString UMobaAnimNotify_SendGameplayEvent::GetNotifyName_Implementation() const
{
	if (EventTag.IsValid())
	{
		TArray<FName> GameplayTagNames;
		UGameplayTagsManager::Get().SplitGameplayTagFName(EventTag, GameplayTagNames);
		return GameplayTagNames.Last().ToString();
	}
	return FString("AN_SendGameplayEvent: EventTag is invalid");
}



