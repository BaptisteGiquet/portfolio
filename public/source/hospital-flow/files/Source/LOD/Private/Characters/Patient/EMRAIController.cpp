#include "Characters/Patient/EMRAIController.h"

#include "Characters/Patient/EMRPatient.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "GAS/EMRTags.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "GameFramework/GameplayMessageSubsystem.h"


AEMRAIController::AEMRAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	
}


void AEMRAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    InitBehaviorTreeAndBlackboard();

    ControlledPatient = Cast<AEMRPatient>(InPawn);
	RegisterToGameplayMessageSubsystem();
}


void AEMRAIController::OnUnPossess()
{
    Super::OnUnPossess();

     if (UWorld* World = GetWorld())
    {
        UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);
        MessageSubsystem.UnregisterListener(NavigationIntentListenerHandle);
    }

    ControlledPatient.Reset();
    NavigationIntentListenerHandle = FGameplayMessageListenerHandle();
}


void AEMRAIController::BeginPlay()
{
    Super::BeginPlay();
}


void AEMRAIController::InitBehaviorTreeAndBlackboard()
{
    if (BehaviorTree)
    {
    	RunBehaviorTree(BehaviorTree);
    }
	

	if (GetBlackboardComponent())
    {
		BlackboardComponent = GetBlackboardComponent();
    }
}


void AEMRAIController::RestartBehaviorTree()
{
    InitBehaviorTreeAndBlackboard();
}


void AEMRAIController::RegisterToGameplayMessageSubsystem()
{
	UWorld* World = GetWorld();
	if (!World) return;
	
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);
	
	FGameplayMessageListenerParams<FEMRNavigationIntentMessage> Params;
	Params.MatchType = EGameplayMessageMatch::PartialMatch;
	Params.OnMessageReceivedCallback = [WeakThis = TWeakObjectPtr(this)] (FGameplayTag Channel, const FEMRNavigationIntentMessage& Message)
	{
		if (AEMRAIController* StrongThis = WeakThis.Get())
		{
			StrongThis->HandleNavigationIntentMessage(Channel, Message);
		}
	};

	NavigationIntentListenerHandle = MessageSubsystem.RegisterListener(EMRTags::GMS::AI::Navigation::Root, Params);
}


void AEMRAIController::HandleNavigationIntentMessage(FGameplayTag ChannelTag, const FEMRNavigationIntentMessage& Message)
{
	if (!ControlledPatient.IsValid() || Message.Patient.Get() != ControlledPatient.Get())
	{
		return;
	}

	if (!BlackboardComponent)
	{
		return;
	}

	const AActor* TargetActor = Message.TargetActor.Get();
	BlackboardComponent->ClearValue(PatientTargetKeyName);

	if (TargetActor)
	{
		BlackboardComponent->SetValueAsObject(PatientTargetKeyName, const_cast<AActor*>(TargetActor));
	}

	FVector TargetLocation = Message.TargetLocation;
	bool bHasTargetLocation = Message.bHasTargetLocation;
	if (!bHasTargetLocation && TargetActor)
	{
		TargetLocation = TargetActor->GetActorLocation();
		bHasTargetLocation = true;
	}

	if (bHasTargetLocation)
	{
		BlackboardComponent->SetValueAsVector(PatientWaitPointLocationKeyName, TargetLocation);
	}
	else
	{
		BlackboardComponent->ClearValue(PatientWaitPointLocationKeyName);
	}

	FRotator TargetRotation = Message.TargetRotation;
	bool bHasTargetRotation = Message.bHasTargetRotation;
	if (!bHasTargetRotation && TargetActor)
	{
		TargetRotation = TargetActor->GetActorRotation();
		bHasTargetRotation = true;
	}

	if (bHasTargetRotation)
	{
		BlackboardComponent->SetValueAsRotator(PatientWaitPointRotationKeyName, TargetRotation);
	}
	else
	{
		BlackboardComponent->ClearValue(PatientWaitPointRotationKeyName);
	}
}


void AEMRAIController::ResetBlackboardState()
{
    if (!BlackboardComponent)
    {
        return;
    }

    BlackboardComponent->ClearValue(PatientTargetKeyName);
    BlackboardComponent->ClearValue(PatientWaitPointLocationKeyName);
    BlackboardComponent->ClearValue(PatientWaitPointRotationKeyName);
}
