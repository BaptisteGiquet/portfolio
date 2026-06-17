
#include "AI/MobaAIController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "BrainComponent.h"
#include "GAS/MobaAbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GAS/MobaGameplayTags.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"


AMobaAIController::AMobaAIController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetupSightPerception();
}




void AMobaAIController::SetupSightPerception()
{
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("AISightConfig"));

	// Config Sight sense to only detect Enemies (based on TeamID Interface)
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;

	SightConfig->SightRadius = 1000.f;
	SightConfig->LoseSightRadius = 1200.f;

	// AI will still remember target for 5sc after losing sight
	SightConfig->SetMaxAge(5.f);
	
	SightConfig->PeripheralVisionAngleDegrees = 180.f;

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AMobaAIController::TargetPerceptionUpdate);
	AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(this, &AMobaAIController::TargetPerceptionForgotten);
}




void AMobaAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	SetPawnGenericTeamID(InPawn);

	GetPawnBlackboardComponent();

	BindToGameplayTagChange();
}

void AMobaAIController::BeginPlay()
{
	Super::BeginPlay();
}


void AMobaAIController::SetPawnGenericTeamID(APawn* InPawn)
{
	IGenericTeamAgentInterface* PawnTeamInterface = Cast<IGenericTeamAgentInterface>(InPawn);
	if (PawnTeamInterface)
	{
		SetGenericTeamId(PawnTeamInterface->GetGenericTeamId());
		ClearAndDisableAllSenses();
		EnableAllSenses();
	}
}




void AMobaAIController::GetPawnBlackboardComponent()
{
	if (!BehaviorTree)
	{
		return;
	}
	RunBehaviorTree(BehaviorTree);
	
	if (!GetBlackboardComponent())
	{
		return;
	}
	BlackboardComponent = GetBlackboardComponent();	
}




void AMobaAIController::BindToGameplayTagChange()
{
	UAbilitySystemComponent* PawnASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (PawnASC)
	{
		PawnASC->RegisterGameplayTagEvent(MobaStatusTags::Status_Dead, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMobaAIController::OnDeathTagUpdated);
		PawnASC->RegisterGameplayTagEvent(MobaStatusTags::Status_Stun, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMobaAIController::OnStunTagUpdated);
	}
}




void AMobaAIController::SetCurrentTarget(AActor* NewTarget)
{
	if (BlackboardComponent)
	{
		BlackboardComponent->SetValueAsObject(TargetBlackboardKeyName, NewTarget);	
	}
}




AActor* AMobaAIController::GetCurrentTarget() const
{
	// Check if Target BBKey already has a value
	if (BlackboardComponent)
	{
		return Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetBlackboardKeyName));
	}
	return nullptr;
}




void AMobaAIController::EnableAllSenses()
{
	if (!AIPerceptionComponent)
	{
		return;
	}

	UAIPerceptionComponent::TAISenseConfigConstIterator SensesConfigIterator = AIPerceptionComponent->GetSensesConfigIterator();
	for (SensesConfigIterator; SensesConfigIterator; ++SensesConfigIterator)
	{
		const UAISenseConfig* AISenseConfig = *SensesConfigIterator;
		if (!AISenseConfig)
		{
			continue;
		}
		
		const TSubclassOf<UAISense> SenseClass = AISenseConfig->GetSenseImplementation();
		AIPerceptionComponent->SetSenseEnabled(SenseClass, true);
	}
}




void AMobaAIController::ClearAndDisableAllSenses()
{
	if (!AIPerceptionComponent)
	{
		return;
	}

	AIPerceptionComponent->ForgetAll();
	/*constexpr float StimuliMaxAge = TNumericLimits<float>::Max();
	AIPerceptionComponent->AgeStimuli(StimuliMaxAge);*/

	// We deactivate every AISenseConfig this AI has
	UAIPerceptionComponent::TAISenseConfigConstIterator SensesConfigIterator = AIPerceptionComponent->GetSensesConfigIterator();
	for (SensesConfigIterator; SensesConfigIterator; ++SensesConfigIterator)
	{
		const UAISenseConfig* AISenseConfig = *SensesConfigIterator;
		if (!AISenseConfig)
		{
			continue;
		}
		
		const TSubclassOf<UAISense> SenseClass = AISenseConfig->GetSenseImplementation();
		if (!SenseClass)
		{
			continue;
		}
		
		AIPerceptionComponent->SetSenseEnabled(SenseClass, false);
	}

	if (BlackboardComponent)
	{
		BlackboardComponent->ClearValue(TargetBlackboardKeyName);
	}
}

void AMobaAIController::OnStunTagUpdated(const FGameplayTag Tag, const int32 StunTagCount)
{
	if (PawnIsDead()) return;
	
	bPawnIsStun = StunTagCount != 0;
	if (PawnIsStun())
	{
		if (GetBrainComponent())
		{
			GetBrainComponent()->StopLogic("Stun");
		}
	}
	else
	{
		if (GetBrainComponent())
		{
			GetBrainComponent()->StartLogic();
		}
	}
	
}


void AMobaAIController::OnDeathTagUpdated(const FGameplayTag Tag, const int32 DeadTagCount)
{
	bPawnIsDead = DeadTagCount != 0;
	if (PawnIsDead())
	{
		ClearAndDisableAllSenses();
		if (GetBrainComponent())
		{
			GetBrainComponent()->StopLogic("Dead");	
		}
	}
	else
	{
		if (GetBrainComponent())
		{
			GetBrainComponent()->StartLogic();	
		}
		EnableAllSenses();
	}
}




void AMobaAIController::TargetPerceptionUpdate(AActor* ActorPerceived, FAIStimulus Stimulus)
{
	// this function is called everytime the perception change for THIS ActorPerceived
	const bool bIsActorPerceived = Stimulus.WasSuccessfullySensed();
	if (bIsActorPerceived)
	{
		if (!GetCurrentTarget())
		{
			SetCurrentTarget(ActorPerceived);
		}
	}
	else // Lose perception of this actor
	{
		//If Actor is not dead AI will wait for MaxAge before forgetting
		ForgetActorIfDead(ActorPerceived);
	}
}




void AMobaAIController::ForgetActorIfDead(AActor* ActorToForget)
{
	const UAbilitySystemComponent* ActorToForgetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ActorToForget);
	if (!ActorToForgetASC)
		return;
	
	if (ActorToForgetASC->HasMatchingGameplayTag(MobaStatusTags::Status_Dead))
	{
		if (!AIPerceptionComponent)
		{
			return;
		}
		
		AIPerceptionComponent->ForgetActor(ActorToForget);

		if (ActorToForget == GetCurrentTarget() && BlackboardComponent)
		{
			BlackboardComponent->ClearValue(TargetBlackboardKeyName);
		}
	}
}




void AMobaAIController::TargetPerceptionForgotten(AActor* ForgottenActor)
{
	// Function called when Target is forgotten (lose sight for more than MaxAge or dead)
	if (!ForgottenActor)
		return;

	if (GetCurrentTarget() == ForgottenActor)
	{
		SetCurrentTarget(GetNextPerceivedActor());
	}
}




AActor* AMobaAIController::GetNextPerceivedActor() const
{
	if (!AIPerceptionComponent)
	{
		return nullptr;
	}

	TArray<AActor*> PerceivedActors;
	AIPerceptionComponent->GetPerceivedHostileActors(PerceivedActors);
	
	if (PerceivedActors.Num() == 0)
	{
		return nullptr;
	}

	AActor* NextPerceivedActor = PerceivedActors[0];
	
	return NextPerceivedActor;
}
