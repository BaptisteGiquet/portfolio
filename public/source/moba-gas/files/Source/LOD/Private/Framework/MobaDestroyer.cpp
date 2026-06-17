

#include "MobaDestroyer.h"

#include "AIController.h"
#include "GenericTeamAgentInterface.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/Abilities/MobaGameplayTypes.h"
#include "Net/UnrealNetwork.h"


AMobaDestroyer::AMobaDestroyer()
{
	PrimaryActorTick.bCanEverTick = true;

	UnitDetectionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("UnitDetectionComponent"));
	UnitDetectionComponent->SetupAttachment(GetRootComponent());
	
	UnitDetectionComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnUnitEnteredDetectionRange);
	UnitDetectionComponent->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnUnitLeftDetectionRange);

	GroundDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("GroundDecalComponent"));
	GroundDecalComponent->SetupAttachment(GetRootComponent());
	
	ViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCameraComponent"));
	ViewCameraComponent->SetupAttachment(GetRootComponent());
}


void AMobaDestroyer::BeginPlay()
{
	Super::BeginPlay();

	if (!TeamOneGoal || !TeamTwoGoal) { return; }
	
	const FVector TeamOneGoalLocation = TeamOneGoal->GetActorLocation();
	const FVector TeamTwoGoalLocation = TeamTwoGoal->GetActorLocation();

	FVector TeamGoalOffset = TeamOneGoalLocation - TeamTwoGoalLocation;
	TeamGoalOffset.Z = 0.f;

	DistanceBetweenTeams = TeamGoalOffset.Length();
}


void AMobaDestroyer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	OwnerAIController = Cast<AAIController>(NewController);
}


void AMobaDestroyer::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, UnitDetectionRadius))
	{
		UE_LOG(LogTemp, Warning, TEXT("PostEditChangeProperty"))
		UnitDetectionComponent->SetSphereRadius(UnitDetectionRadius);
		GroundDecalComponent->DecalSize = FVector(UnitDetectionRadius);
	}
}


void AMobaDestroyer::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, CoreToCapture, COND_None, REPNOTIFY_Always);
}


void AMobaDestroyer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CoreToCapture)
	{
		const FVector CoreMoveDirection = (GetMesh()->GetComponentLocation() - CoreToCapture->GetActorLocation()).GetSafeNormal();
		CoreToCapture->AddActorWorldOffset(CoreMoveDirection * CoreCaptureSpeed * DeltaSeconds);
	}
}


float AMobaDestroyer::GetDestroyerProgress() const
{
	// TeamTwoGoal is the base of TeamOne
	const FVector TeamOneBaseLocation = TeamTwoGoal->GetActorLocation();
	FVector VectorFromTeamOne = GetActorLocation() - TeamOneBaseLocation;
	VectorFromTeamOne.Z = 0.f;

	return VectorFromTeamOne.Length() / DistanceBetweenTeams;
}


void AMobaDestroyer::OnUnitEnteredDetectionRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OverlappedComponent || !OtherActor || !OtherComp) { return; }

	if (OtherActor == TeamOneGoal) // Destroyer arrived at TeamOneGoal
	{
		GoalReached(EMobaTeam::TeamOne);
		return;
	}
	if (OtherActor == TeamTwoGoal)
	{
		GoalReached(EMobaTeam::TeamTwo);
		return;
	}
	
	IGenericTeamAgentInterface* UnitTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
	if (UnitTeamInterface)
	{
		const uint8 UnitTeamID = UnitTeamInterface->GetGenericTeamId().GetId();
		
		if (UnitTeamID == static_cast<uint8>(EMobaTeam::TeamOne))
		{
			TeamOneUnitCount++;
		}
		else if (UnitTeamID == static_cast<uint8>(EMobaTeam::TeamTwo))
		{
			TeamTwoUnitCount++;
		}

		UpdateTeamDominance();
	}
}


void AMobaDestroyer::OnUnitLeftDetectionRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OverlappedComponent || !OtherActor || !OtherComp) { return; }

	IGenericTeamAgentInterface* OtherTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
	if (OtherTeamInterface)
	{
		if (OtherTeamInterface->GetGenericTeamId().GetId() == static_cast<uint8>(EMobaTeam::TeamOne))
		{
			TeamOneUnitCount = FMath::Max(0, TeamOneUnitCount - 1);
		}
		else
		{
			TeamTwoUnitCount = FMath::Max(0, TeamTwoUnitCount - 1);
		}

		UpdateTeamDominance();
	}
}


void AMobaDestroyer::UpdateTeamDominance()
{
	OnTeamUnitCountUpdated.Broadcast(TeamOneUnitCount, TeamTwoUnitCount);
	
	if (TeamOneUnitCount == TeamTwoUnitCount)
	{
		TeamDominance = 0.f;
	}
	else
	{
		const float TeamOffset = TeamOneUnitCount - TeamTwoUnitCount;
		const float TeamTotal = TeamOneUnitCount + TeamTwoUnitCount;

		if (TeamTotal > 0)
		{
			TeamDominance = TeamOffset / TeamTotal;	
		}
	}

	
	UpdateDestroyerGoal();
}


void AMobaDestroyer::UpdateDestroyerGoal()
{
	if (!HasAuthority() || !OwnerAIController || !GetCharacterMovement()) { return; }

	// Destroyer movementspeed is based on the Team dominance
	const float Speed = MaxMovementSpeed * FMath::Abs(TeamDominance);
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	
	if (TeamDominance > 0)
	{
		OwnerAIController->MoveToActor(TeamOneGoal);
	}
	else
	{
		OwnerAIController->MoveToActor(TeamTwoGoal);
	}
}


void AMobaDestroyer::GoalReached(const EMobaTeam WinningTeam)
{
	OnGoalReached.Broadcast(this, WinningTeam);

	if (!HasAuthority()) { return; }

	UE_LOG(LogTemp, Warning, TEXT("GoalReached"))
	
	MaxMovementSpeed = 0.f;
	if (WinningTeam == EMobaTeam::TeamOne)
	{
		CoreToCapture = TeamTwoCore;
	}
	else if (WinningTeam == EMobaTeam::TeamTwo)
	{
		CoreToCapture = TeamOneCore;
	}

	CaptureCore();	
}


void AMobaDestroyer::CaptureCore()
{
	if (!GetMesh() || !GetMesh()->GetAnimInstance() || !ExpandMontage || !CoreToCapture) { return; }

	UE_LOG(LogTemp, Warning, TEXT("CaptureCore"))
	const float ExpandDuration = GetMesh()->GetAnimInstance()->Montage_Play(ExpandMontage);

	CoreCaptureSpeed = FVector::Distance(GetMesh()->GetComponentLocation(), CoreToCapture->GetActorLocation()) / ExpandDuration;

	CoreToCapture->SetActorEnableCollision(false);
	GetCharacterMovement()->MaxWalkSpeed = 0.f;

	FTimerHandle ExpandTimerHandle;
	GetWorldTimerManager().SetTimer(ExpandTimerHandle, this, &ThisClass::OnExpandFinished, ExpandDuration);
}


void AMobaDestroyer::OnExpandFinished()
{
	if (!CaptureMontage) { return; }

	UE_LOG(LogTemp, Warning, TEXT("OnExpandFinished"))

	CoreToCapture->SetActorLocation(GetMesh()->GetComponentLocation());
	CoreToCapture->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform, "root");
	CoreToCapture->SetActorScale3D(FVector::ZeroVector);
	const float CaptureDuration = GetMesh()->GetAnimInstance()->Montage_Play(CaptureMontage);
}


void AMobaDestroyer::OnRep_CoreToCapture()
{
	if (!CoreToCapture) { return; }

	UE_LOG(LogTemp, Warning, TEXT("OnRep_CoreToCapture"))
	CaptureCore();
}

