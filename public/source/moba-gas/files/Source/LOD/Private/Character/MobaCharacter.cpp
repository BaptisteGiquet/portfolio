
#include "Character/MobaCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Widgets/MobaOverheadResourceBarWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/MobaAbilitySystemComponent.h"
#include "GAS/Attributes/MobaAttributeSet.h"
#include "GAS/MobaGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "LOD/MobaCustomCollisionChannels.h"

AMobaCharacter::AMobaCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_SpringArm, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Target, ECR_Ignore);
	
	AbilitySystemComponent = CreateDefaultSubobject<UMobaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UMobaAttributeSet>(TEXT("AttributeSet"));

	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverHeardWidgetComponent"));
	OverheadWidgetComponent->SetupAttachment(GetRootComponent());

	PerceptionStimuliSourceComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSourceComponent"));

	BindToGASChangeDelegate();
}




void AMobaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}




void AMobaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}




void AMobaCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	SetOwner(NewController);
	
	if (NewController && !NewController->IsPlayerController())
	{
		InitializeAbilitySystemComponent_Server();
	}
}




void AMobaCharacter::InitializeAbilitySystemComponent_Server()
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->InitializeAbilitySystemComponent_Server();
}




void AMobaCharacter::InitializeAbilitySystemComponent_Client()
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}




bool AMobaCharacter::IsLocallyControlledByPlayer() const
{
	return GetController() && GetController()->IsLocalPlayerController();
}




void AMobaCharacter::BeginPlay()
{
	Super::BeginPlay();
	ConfigureOverheadWidget();
	
	RefreshAnimInstanceCache();
	
	if (GetMesh())
	{
		MeshRelativeTransform = GetMesh()->GetRelativeTransform();
	}

	if (PerceptionStimuliSourceComp)
	{
		PerceptionStimuliSourceComp->RegisterForSense(UAISense_Sight::StaticClass());	
	}
}




TObjectPtr<UAnimInstance> AMobaCharacter::GetAvatarAnimInstance()
{
	if (!GetMesh()->GetAnimInstance())
	{
		return nullptr;
	}
	UAnimInstance* AvatarAnimInstance = GetMesh()->GetAnimInstance();
	return AvatarAnimInstance;
}




void AMobaCharacter::RefreshAnimInstanceCache()
{
	if (GetMesh())
	{
		CachedAvatarAnimInstance = GetMesh()->GetAnimInstance();
	}
}



void AMobaCharacter::UpgradeAbilityWithInputID(const EAbilityInputID AbilityInputID)
{
	if (AbilitySystemComponent && IsLocallyControlledByPlayer())
	{
		const FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromInputID(static_cast<int32>(AbilityInputID));
		if (!AbilitySpec) { return; }
		
		const FGameplayAbilitySpecHandle AbilitySpecHandle = AbilitySpec->Handle;
		if (!AbilitySpecHandle.IsValid()) { return; }
		
		AbilitySystemComponent->Server_UpgradeAbilityWithHandle(AbilitySpecHandle);	
	}
}



UAbilitySystemComponent* AMobaCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}




// Bind callback functions for all GAS change that occurs
void AMobaCharacter::BindToGASChangeDelegate()
{
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(MobaStatusTags::Status_Dead, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::DeathTagUpdated);
		AbilitySystemComponent->RegisterGameplayTagEvent(MobaStatusTags::Status_Stun, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::StunTagUpdated);

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet::GetMoveSpeedAttribute()).AddUObject(this, &ThisClass::OnMoveSpeedUpdated);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &ThisClass::OnMaxHealthUpdated);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet::GetMaxManaAttribute()).AddUObject(this, &ThisClass::OnMaxManaUpdated);
	}
}




void AMobaCharacter::DeathTagUpdated(const FGameplayTag Tag, const int32 DeadTagCount)
{
	bIsDead = DeadTagCount != 0;
	if (IsDead())
	{
		StartDeathSequence();
	}
	else
	{
		StartRespawnSequence();
	}
}




void AMobaCharacter::StunTagUpdated(const FGameplayTag Tag, const int32 StunTagCount)
{
	if (IsDead() || !IsValid(StunMontage)) return;
	
	bIsStun = StunTagCount != 0;
	if (IsStun())
	{
		OnStun();
		PlayAnimMontage(StunMontage);
	}
	else
	{
		OnRecoverFromStun();
		StopAnimMontage(StunMontage);
	}
}


FVector AMobaCharacter::GetCaptureLocalPosition() const
{
	return HeroPortraitCaptureLocalPosition;
}

FRotator AMobaCharacter::GetCaptureLocalRotation() const
{
	return HeroPortraitCaptureLocalRotation;
}

void AMobaCharacter::ConfigureOverheadWidget()
{
	if (!OverheadWidgetComponent)
	 { 
	 	return;
	 }
	
	if (IsLocallyControlledByPlayer())
	{
		// For client, we don't show and configure the overheadwidget
		OverheadWidgetComponent->SetHiddenInGame(true);
		return;
	}
	
	UMobaOverheadResourceBarWidget* OverHeadWidget = Cast<UMobaOverheadResourceBarWidget>(OverheadWidgetComponent->GetUserWidgetObject());
	if (OverHeadWidget && AbilitySystemComponent)
	{
		OverHeadWidget->BindToAbilitySystemComponent(AbilitySystemComponent);
		GetWorldTimerManager().ClearTimer(OverheadVisibilityTimerHandle);
		GetWorldTimerManager().SetTimer(OverheadVisibilityTimerHandle, this, &AMobaCharacter::UpdateOverheadResourceBarVisibility, OverheadVisibilityUpdateFrequency, true, 0.f);
	}
}




void AMobaCharacter::UpdateOverheadResourceBarVisibility()
{
	// Hard to understand: Server will replicate every character in the game to each client, so this function will be called by every character on client side
	// But, GetPlayerPawn always give the Pawn of the PlayerController of index 0, so when NON-CLIENT Characters use this function, they will compare
	// their position to the Client Character position
	APawn* LocalPlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (LocalPlayerPawn)
	{
		const float MaxRangeSquared = FMath::Square(OverheadVisibilityMaxRange);
		// We compare the position of the Client Pawn and the Character that used this function
		const float DistSquared = FVector::DistSquared(GetActorLocation(), LocalPlayerPawn->GetActorLocation());
		OverheadWidgetComponent->SetHiddenInGame(DistSquared > MaxRangeSquared);
	}
}




void AMobaCharacter::SetOverheadResourceBarWidgetEnabled(const bool bIsEnabled)
{
	GetWorldTimerManager().ClearTimer(OverheadVisibilityTimerHandle);
	if (bIsEnabled)
	{
		ConfigureOverheadWidget();
	}
	else
	{
		OverheadWidgetComponent->SetHiddenInGame(true);
	}
}




void AMobaCharacter::OnStun()
{
	
}




void AMobaCharacter::OnRecoverFromStun()
{
	
}




void AMobaCharacter::OnDead()
{
	// Parent function that will be overridden on player character
}




void AMobaCharacter::OnRespawn()
{
	// Parent function that will be overridden on player character
}




void AMobaCharacter::StartDeathSequence()
{
	// OnDead and OnRespawn will be override in PlayerCharacter to add things only for them
	OnDead();
	
	CancelAllAbilities();
	PlayDeathAnimation();
	SetOverheadResourceBarWidgetEnabled(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	SetAIPerceptionStimuliSourceEnabled(false);
}




void AMobaCharacter::StartRespawnSequence()
{
	OnRespawn();
	
	SetRagdollEnabled(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SetOverheadResourceBarWidgetEnabled(true);
	GetAvatarAnimInstance()->StopAllMontages(0.f);

	SetAIPerceptionStimuliSourceEnabled(true);

	if (HasAuthority() && GetController())
	{
		TWeakObjectPtr<AActor> StartSpot = GetController()->StartSpot;

		if (StartSpot.IsValid())
		{
			SetActorTransform(StartSpot->GetTransform());
		}
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ApplyRespawnEffects();
	}
}




void AMobaCharacter::PlayDeathAnimation()
{
	if (DeathMontage && CachedAvatarAnimInstance.IsValid())
	{
		PlayAnimMontage(DeathMontage);
		// Bind function for when the death montage blends out
		FOnMontageBlendingOutStarted BlendOutStarted;
		BlendOutStarted.BindUObject(this, &AMobaCharacter::OnDeathMontageBlendingOut);
		
		CachedAvatarAnimInstance->Montage_SetBlendingOutDelegate(BlendOutStarted, DeathMontage);
	}
}




void AMobaCharacter::OnDeathMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	SetRagdollEnabled(true);
}




void AMobaCharacter::SetRagdollEnabled(const bool bIsEnabled)
{
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (bIsEnabled)
	{
		if (!SkeletalMesh)
		{
			return;
		}
		
		SkeletalMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		SkeletalMesh->SetSimulatePhysics(true);
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	}
	else
	{
		if (!SkeletalMesh)
		{
			return;
		}
		
		SkeletalMesh->SetSimulatePhysics(false);
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkeletalMesh->AttachToComponent(GetRootComponent(),FAttachmentTransformRules::KeepRelativeTransform);
		SkeletalMesh->SetRelativeTransform(MeshRelativeTransform);
	}
}




void AMobaCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	IGenericTeamAgentInterface::SetGenericTeamId(NewTeamID);
	TeamID = NewTeamID;
}




FGenericTeamId AMobaCharacter::GetGenericTeamId() const
{
	return TeamID;
}




void AMobaCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(AMobaCharacter, TeamID, COND_None, REPNOTIFY_Always)
}

void AMobaCharacter::OnRep_TeamID()
{
}


void AMobaCharacter::SetAIPerceptionStimuliSourceEnabled(const bool bIsEnabled)
{
	if (!PerceptionStimuliSourceComp)
	{
		return;
	}

	if (bIsEnabled)
	{
		PerceptionStimuliSourceComp->RegisterWithPerceptionSystem();
	}
	else
	{
		PerceptionStimuliSourceComp->UnregisterFromPerceptionSystem();
	}
}




void AMobaCharacter::CancelAllAbilities()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();	
	}
}


void AMobaCharacter::OnMaxHealthUpdated(const FOnAttributeChangeData& HealthData)
{
	if (IsValid(AttributeSet))
	{
		AttributeSet->RescaleHealth();
	}	
}


void AMobaCharacter::OnMaxManaUpdated(const FOnAttributeChangeData& ManaData)
{
	if (IsValid(AttributeSet))
	{
		AttributeSet->RescaleMana();
	}	
}


void AMobaCharacter::OnMoveSpeedUpdated(const FOnAttributeChangeData& MoveSpeedData)
{
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeedData.NewValue;
}



