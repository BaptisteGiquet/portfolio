#include "Environment/EMRHospitalExit.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "GAS/EMRTags.h"
#include "Subsystems/EMRPatientPoolSubsystem.h"
#include "Subsystems/EMRNightShiftSpawnSubsystem.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

AEMRHospitalExit::AEMRHospitalExit()
{
    PrimaryActorTick.bCanEverTick = false;

    ExitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("ExitPoint"));
    SetRootComponent(ExitPoint);

    ExitTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("ExitTrigger"));
    ExitTrigger->SetupAttachment(ExitPoint);
    ExitTrigger->InitSphereRadius(150.f);
    ExitTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ExitTrigger->SetCollisionObjectType(ECC_WorldDynamic);
    ExitTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    ExitTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    ExitTrigger->SetGenerateOverlapEvents(true);

    ExitTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleExitOverlap);
}

FTransform AEMRHospitalExit::GetExitTransform() const
{
    if (ExitPoint)
    {
        return ExitPoint->GetComponentTransform();
    }

    return GetActorTransform();
}

void AEMRHospitalExit::HandleExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority())
    {
        return;
    }

    if (AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(OtherActor))
    {
        PlayerExitOverlapDelegate.Broadcast(PlayerCharacter);
    }

    AEMRPatient* Patient = Cast<AEMRPatient>(OtherActor);
    if (!Patient)
    {
        return;
    }

    UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent();
    if (!PatientASC || !PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving))
    {
        return;
    }

    // Reset the leaving phase tag before returning the patient to the pool
    FGameplayTagContainer TagsToRemove;
    TagsToRemove.AddTag(EMRTags::Patient::Phase::Leaving);
	PatientASC->RemoveActiveEffectsWithGrantedTags(TagsToRemove);
	

    if (UWorld* World = GetWorld())
    {
        if (UEMRPatientPoolSubsystem* PoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>())
        {
            PoolSubsystem->ReleasePatient(Patient);
        }

        if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
        {
            SpawnSubsystem->NotifyActivePatientReleased();
        }
    }
}
