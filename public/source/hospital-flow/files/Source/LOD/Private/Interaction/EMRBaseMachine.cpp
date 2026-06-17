#include "Interaction/EMRBaseMachine.h"

#include "Characters/Patient/EMRAIController.h"
#include "Characters/Patient/EMRPatient.h"
#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Subsystems/EMRExamQueueSubsystem.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "GAS/EMRTags.h"
#include "LOD/EMRCollisionChannels.h"
#include "Net/UnrealNetwork.h"

AEMRBaseMachine::AEMRBaseMachine()
{
	PrimaryActorTick.bCanEverTick = false;

	MachineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MachineMesh"));
	SetRootComponent(MachineMesh);
	MachineMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MachineMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
     MachineMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
	DetectionRadius = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionRadius"));
	DetectionRadius->SetupAttachment(GetRootComponent());
	DetectionRadius->SetSphereRadius(1000.0f);
	DetectionRadius->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionRadius->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionRadius->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PatientWaitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("PatientWaitPoint"));
	PatientWaitPoint->SetupAttachment(GetRootComponent());
	PatientWaitPoint->SetArrowColor(FLinearColor::Green);
	PatientWaitPoint->bIsScreenSizeScaled = true;

	InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));
}


void AEMRBaseMachine::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEMRBaseMachine, bOccupied);
	DOREPLIFETIME(AEMRBaseMachine, CurrentPatient);
}


void AEMRBaseMachine::BeginPlay()
{
    Super::BeginPlay();

    if (!MachineTypeTag.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[Machine] %s has no MachineTypeTag set!"), *GetName());
        return;
    }

    if (InteractableComponent && InteractableComponent->GetDisplayName().IsEmpty())
    {
        const FText DefaultDisplayName = FText::FromString(MachineTypeTag.ToString());
        InteractableComponent->SetDisplayName(DefaultDisplayName);
    }

    UEMRExamQueueSubsystem* QueueSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEMRExamQueueSubsystem>();
    if (QueueSubsystem)
    {
        QueueSubsystem->OnPatientAddedToExamQueue.AddDynamic(this, &AEMRBaseMachine::OnPatientAddedToQueue);
		QueueSubsystem->OnExamCompleted.AddDynamic(this, &AEMRBaseMachine::OnExamCompleted);
	}
}


FGameplayEventData AEMRBaseMachine::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    FGameplayEventData EventData;
    EventData.EventTag = MachineTypeTag;
    EventData.Instigator = InterActor;
	EventData.Target = Cast<AActor>(FindNearestPatient());
	EventData.OptionalObject = this;
	
    return EventData;
}


FText AEMRBaseMachine::GetInteractableDisplayName_Implementation() const
{
    if (InteractableComponent)
    {
        return InteractableComponent->GetDisplayName();
    }

    if (MachineTypeTag.IsValid())
    {
        return FText::FromName(MachineTypeTag.GetTagName());
    }

    return FText::FromString(GetName());
}


bool AEMRBaseMachine::IsInteractableEnabled_Implementation() const
{
    return InteractableComponent ? InteractableComponent->IsEnabled() : true;
}


FVector AEMRBaseMachine::GetPatientWaitPointLocation() const
{
	return PatientWaitPoint->GetComponentLocation();
}


FRotator AEMRBaseMachine::GetPatientWaitPointRotation() const
{
	return PatientWaitPoint->GetComponentRotation();
}


void AEMRBaseMachine::OnPatientAddedToQueue(FGameplayTag ExamTag)
{
    const UEMRExamQueueSubsystem* QueueSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEMRExamQueueSubsystem>();
    const FGameplayTag MachineTagForExam = QueueSubsystem ? QueueSubsystem->GetMachineTagForExam(ExamTag) : FGameplayTag::EmptyTag;
    if (!MachineTagForExam.IsValid() || !MachineTagForExam.MatchesTag(MachineTypeTag))
    {
        UE_LOG(LogTemp, Log, TEXT("[Machine %s] Exam type mismatch (expected %s, got %s)"),
                        *GetName(), *MachineTypeTag.ToString(), *ExamTag.ToString());
        return;
    }

    if (!bOccupied)
    {
        TryAssignNextPatient();
    }
}


void AEMRBaseMachine::OnExamCompleted(FGameplayTag ExamTag)
{
    const UEMRExamQueueSubsystem* QueueSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEMRExamQueueSubsystem>();
    const FGameplayTag MachineTagForExam = QueueSubsystem ? QueueSubsystem->GetMachineTagForExam(ExamTag) : FGameplayTag::EmptyTag;
    if (!MachineTagForExam.IsValid() || !MachineTagForExam.MatchesTag(MachineTypeTag))
    {
        UE_LOG(LogTemp, Log, TEXT("[Machine %s] Exam type mismatch (expected %s, got %s)"),
                        *GetName(), *MachineTypeTag.ToString(), *ExamTag.ToString());
        return;
    }

    if (ExamTag.MatchesTagExact(EMRTags::Abilities::Exam::XRay))
    {
        UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Machine OnExamCompleted machine=%s patient=%s"),
                        *GetNameSafe(this), *GetNameSafe(CurrentPatient));
    }

    if (!ShouldReleasePatientOnExamCompleted(ExamTag))
    {
        return;
    }

    ReleasePatient();
}


bool AEMRBaseMachine::ShouldReleasePatientOnExamCompleted(FGameplayTag ExamTag) const
{
    return true;
}


void AEMRBaseMachine::TryAssignNextPatient()
{
	if (bOccupied)
	{
		return;
	}

    UEMRExamQueueSubsystem* QueueSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEMRExamQueueSubsystem>();
    if (!QueueSubsystem)
    {
        return;
    }

    AEMRPatient* NextPatientInQueue = QueueSubsystem->GetNextPatientForMachine(MachineTypeTag);
    if (NextPatientInQueue)
    {
        AssignPatient(NextPatientInQueue);
    }
}


void AEMRBaseMachine::AssignPatient(AEMRPatient* NextPatientInQueue)
{
    if (!NextPatientInQueue)
    {
        return;
    }

    if (CurrentPatient && CurrentPatient != NextPatientInQueue)
    {
        CurrentPatient->SetCurrentAssignedMachine(nullptr);
    }

    CurrentPatient = NextPatientInQueue;
    bOccupied = true;
    NextPatientInQueue->SetCurrentAssignedMachine(this);
    NextPatientInQueue->SetNextMachineOnList(nullptr);

    if (UEMRNavigationIntentSubsystem* NavigationSubsystem = GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
    {
        const FVector WaitLocation = GetPatientWaitPointLocation();
        const FRotator WaitRotation = GetPatientWaitPointRotation();
        const FGameplayTag NavigationTag = NavigationSubsystem->GetNavigationMessageTagForMachine(MachineTypeTag);

        NavigationSubsystem->BroadcastNavigationIntent(NextPatientInQueue, NavigationTag, this, WaitLocation, WaitRotation, MachineTypeTag);
    }
}


void AEMRBaseMachine::ReleasePatient()
{
    if (CurrentPatient)
    {
        CurrentPatient->SetCurrentAssignedMachine(nullptr);
    }

    CurrentPatient = nullptr;
    bOccupied = false;

    TryAssignNextPatient();
}


void AEMRBaseMachine::SetOccupiedByPatient(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
    }

    if (CurrentPatient && CurrentPatient != Patient)
    {
        CurrentPatient->SetCurrentAssignedMachine(nullptr);
    }

    CurrentPatient = Patient;
    bOccupied = true;
    Patient->SetCurrentAssignedMachine(this);
    Patient->SetNextMachineOnList(nullptr);
}


void AEMRBaseMachine::ClearOccupiedPatient(AEMRPatient* Patient)
{
    if (Patient && CurrentPatient && CurrentPatient != Patient)
    {
        return;
    }

    if (CurrentPatient)
    {
        CurrentPatient->SetCurrentAssignedMachine(nullptr);
    }

    CurrentPatient = nullptr;
    bOccupied = false;

    TryAssignNextPatient();
}


AEMRPatient* AEMRBaseMachine::FindNearestPatient() const
{
    TArray<AActor*> OverlappingActors;
    DetectionRadius->GetOverlappingActors(OverlappingActors, AEMRPatient::StaticClass());

	if (OverlappingActors.IsEmpty())
	{
		return nullptr;
	}


	return Cast<AEMRPatient>(OverlappingActors[0]);
}

