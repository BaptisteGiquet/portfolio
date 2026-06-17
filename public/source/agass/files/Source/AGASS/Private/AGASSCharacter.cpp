#include "AGASSCharacter.h"

#include "AGASSGameModeBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/AGASSRespawnTuningData.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "Net/UnrealNetwork.h"
#include "Settings/AGASSSettingsLocal.h"
#include "Settings/AGASSTowerDeveloperSettings.h"

AAGASSCharacter::AAGASSCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->JumpZVelocity = 420.f;
	GetCharacterMovement()->AirControl = 0.2f;

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 64.f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = FirstPersonPresentationFieldOfView;
	FirstPersonCameraComponent->FirstPersonScale = FirstPersonPresentationScale;

	FirstPersonPresentationMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonPresentationMesh"));
	FirstPersonPresentationMeshComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonPresentationMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	FirstPersonPresentationMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonPresentationMeshComponent->SetGenerateOverlapEvents(false);
	FirstPersonPresentationMeshComponent->SetCanEverAffectNavigation(false);
	FirstPersonPresentationMeshComponent->SetOnlyOwnerSee(true);
	FirstPersonPresentationMeshComponent->SetCastShadow(false);
	FirstPersonPresentationMeshComponent->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonPresentationMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	FirstPersonHiddenBones =
	{
		TEXT("head"),
		TEXT("thigh_l"),
		TEXT("calf_l"),
		TEXT("foot_l"),
		TEXT("ball_l"),
		TEXT("thigh_r"),
		TEXT("calf_r"),
		TEXT("foot_r"),
		TEXT("ball_r")
	};
}

void AAGASSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bDeathRagdollActive);
	DOREPLIFETIME(ThisClass, bHasCarriedUsableItem);
	DOREPLIFETIME(ThisClass, CarriedUsableAnimationId);
}

void AAGASSCharacter::BeginPlay()
{
	Super::BeginPlay();

	RefreshFirstPersonPresentationAssets();
	RefreshFirstPersonPresentation();
	RefreshLocalViewSettings();
	RefreshDangerousFallSettings();
	ResetDangerousFallState();
}

void AAGASSCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateFirstPersonPresentationAlignment();
	RefreshLocalViewSettings();
	UpdateDangerousFallState(DeltaSeconds);
}

void AAGASSCharacter::ApplyMoveInput(const FVector2D& MovementInput)
{
	if (bDeathRagdollActive)
	{
		return;
	}

	MoveForward(MovementInput.Y);
	MoveRight(MovementInput.X);
}

void AAGASSCharacter::ApplyLookInput(const FVector2D& LookInput)
{
	if (bDeathRagdollActive)
	{
		return;
	}

	Turn(LookInput.X);
	LookUp(LookInput.Y);
}

void AAGASSCharacter::StartJumpInput()
{
	if (bDeathRagdollActive)
	{
		return;
	}

	Jump();
}

void AAGASSCharacter::StopJumpInput()
{
	if (bDeathRagdollActive)
	{
		return;
	}

	StopJumping();
}

void AAGASSCharacter::ActivateDeathRagdoll()
{
	if (bDeathRagdollActive)
	{
		return;
	}

	if (HasAuthority())
	{
		MulticastPlayFallDeathSound(GetActorLocation());
	}

	bDeathRagdollActive = true;
	ApplyDeathRagdollState();

	if (HasAuthority())
	{
		ForceNetUpdate();
	}
}

bool AAGASSCharacter::IsDeathRagdollActive() const
{
	return bDeathRagdollActive;
}

bool AAGASSCharacter::HasCarriedUsableItem() const
{
	return bHasCarriedUsableItem;
}

FName AAGASSCharacter::GetCarriedUsableAnimationId() const
{
	return CarriedUsableAnimationId;
}

USkeletalMeshComponent* AAGASSCharacter::GetAGASSCarriedUsableAttachMesh() const
{
	return GetMesh();
}

USkeletalMeshComponent* AAGASSCharacter::GetAGASSLocalViewCarriedUsableAttachMesh() const
{
	return IsLocallyControlled() ? FirstPersonPresentationMeshComponent : nullptr;
}

void AAGASSCharacter::SetAGASSCarriedUsablePresentation(AActor* UsableItemActor, const FName CarryAnimationId)
{
	if (!HasAuthority())
	{
		return;
	}

	PresentedUsableItemActor = UsableItemActor;
	bHasCarriedUsableItem = UsableItemActor != nullptr;
	CarriedUsableAnimationId = bHasCarriedUsableItem ? CarryAnimationId : NAME_None;
	ForceNetUpdate();
}

void AAGASSCharacter::ClearAGASSCarriedUsablePresentation(AActor* UsableItemActor)
{
	if (!HasAuthority())
	{
		return;
	}

	if (UsableItemActor != nullptr && PresentedUsableItemActor.IsValid() && PresentedUsableItemActor.Get() != UsableItemActor)
	{
		return;
	}

	PresentedUsableItemActor.Reset();
	bHasCarriedUsableItem = false;
	CarriedUsableAnimationId = NAME_None;
	ForceNetUpdate();
}

void AAGASSCharacter::PlayAGASSCarriedUsableUseMontage(UAnimMontage* UseMontage, const float PlayRate)
{
	if (!HasAuthority() || UseMontage == nullptr)
	{
		return;
	}

	MulticastPlayAGASSCarriedUsableUseMontage(UseMontage, PlayRate);
}

void AAGASSCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	if (HasAuthority())
	{
		bDangerousFallDeathTriggered = true;

		if (AAGASSGameModeBase* const AGASSGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AAGASSGameModeBase>() : nullptr)
		{
			AGASSGameMode->HandlePlayerPawnFellOutOfWorld(this);
			return;
		}
	}

	Super::FellOutOfWorld(DamageType);
}

void AAGASSCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (!HasAuthority() || !bDangerousFallDeathPending || bDangerousFallDeathTriggered)
	{
		return;
	}

	bDangerousFallDeathTriggered = true;
	if (AAGASSGameModeBase* const AGASSGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AAGASSGameModeBase>() : nullptr)
	{
		AGASSGameMode->HandlePlayerPawnTimedFallDeath(this);
	}
}

void AAGASSCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	RefreshFirstPersonPresentation();
	RefreshLocalViewSettings();
}

void AAGASSCharacter::OnMovementModeChanged(const EMovementMode PrevMovementMode, const uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (bDeathRagdollActive)
	{
		return;
	}

	if (GetCharacterMovement() == nullptr || GetCharacterMovement()->MovementMode != MOVE_Falling)
	{
		ResetDangerousFallState();
	}
}

void AAGASSCharacter::OnRep_DeathRagdollActive()
{
	if (bDeathRagdollActive)
	{
		ApplyDeathRagdollState();
	}
}

void AAGASSCharacter::OnRep_CarriedUsablePresentation()
{
}

void AAGASSCharacter::MulticastPlayAGASSCarriedUsableUseMontage_Implementation(UAnimMontage* UseMontage, const float PlayRate)
{
	PlayAGASSCarriedUsableUseMontageLocal(UseMontage, PlayRate);
}

void AAGASSCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	RefreshFirstPersonPresentation();
	RefreshLocalViewSettings();
}

void AAGASSCharacter::ApplyDeathRagdollState()
{
	ResetDangerousFallState();

	if (UCharacterMovementComponent* const MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	if (UCapsuleComponent* const CharacterCapsuleComponent = GetCapsuleComponent())
	{
		CharacterCapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (USkeletalMeshComponent* const CharacterMeshComponent = GetMesh())
	{
		CharacterMeshComponent->SetCollisionProfileName(TEXT("Ragdoll"));
		CharacterMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CharacterMeshComponent->SetGenerateOverlapEvents(false);
		CharacterMeshComponent->SetSimulatePhysics(true);
		CharacterMeshComponent->SetAllBodiesSimulatePhysics(true);
		CharacterMeshComponent->SetEnableGravity(true);
		CharacterMeshComponent->WakeAllRigidBodies();
		CharacterMeshComponent->bBlendPhysics = true;
	}

	if (FirstPersonPresentationMeshComponent != nullptr)
	{
		FirstPersonPresentationMeshComponent->SetHiddenInGame(true, true);
	}
}

void AAGASSCharacter::PlayAGASSCarriedUsableUseMontageLocal(UAnimMontage* UseMontage, const float PlayRate) const
{
	if (bDeathRagdollActive || UseMontage == nullptr)
	{
		return;
	}

	if (USkeletalMeshComponent* const CharacterMeshComponent = GetMesh())
	{
		if (UAnimInstance* const AnimInstance = CharacterMeshComponent->GetAnimInstance())
		{
			AnimInstance->Montage_Play(UseMontage, PlayRate);
		}
	}
}

void AAGASSCharacter::MulticastPlayFallDeathSound_Implementation(const FVector_NetQuantize SoundLocation)
{
	PlayFallDeathSoundAtLocation(SoundLocation);
}

void AAGASSCharacter::PlayFallDeathSoundAtLocation(const FVector& SoundLocation) const
{
	if (FallDeathSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FallDeathSound, SoundLocation);
	}
}

void AAGASSCharacter::RefreshFirstPersonPresentationAssets()
{
	USkeletalMeshComponent* const CharacterMeshComponent = GetMesh();
	if (CharacterMeshComponent == nullptr || FirstPersonPresentationMeshComponent == nullptr)
	{
		return;
	}

	USkeletalMesh* FirstPersonMeshAsset = CharacterMeshComponent->GetSkeletalMeshAsset();
	if (!FirstPersonPresentationMeshOverride.IsNull())
	{
		if (USkeletalMesh* const OverrideMesh = FirstPersonPresentationMeshOverride.LoadSynchronous())
		{
			FirstPersonMeshAsset = OverrideMesh;
		}
	}

	FirstPersonPresentationMeshComponent->SetSkeletalMeshAsset(FirstPersonMeshAsset);
	for (int32 MaterialIndex = 0; MaterialIndex < CharacterMeshComponent->GetNumMaterials(); ++MaterialIndex)
	{
		FirstPersonPresentationMeshComponent->SetMaterial(MaterialIndex, CharacterMeshComponent->GetMaterial(MaterialIndex));
	}

	FirstPersonPresentationMeshComponent->SetLeaderPoseComponent(CharacterMeshComponent, true, false);
}

void AAGASSCharacter::RefreshFirstPersonPresentation()
{
	USkeletalMeshComponent* const CharacterMeshComponent = GetMesh();
	if (CharacterMeshComponent == nullptr || FirstPersonPresentationMeshComponent == nullptr)
	{
		return;
	}

	RefreshFirstPersonPresentationAssets();

	if (UCapsuleComponent* const CharacterCapsuleComponent = GetCapsuleComponent())
	{
		if (FirstPersonPresentationMeshComponent->GetAttachParent() != CharacterCapsuleComponent)
		{
			FirstPersonPresentationMeshComponent->AttachToComponent(
				CharacterCapsuleComponent,
				FAttachmentTransformRules::KeepRelativeTransform);
		}
	}

	const bool bUseDedicatedFirstPersonPresentation = IsLocallyControlled() && !bDeathRagdollActive;
	CharacterMeshComponent->SetOwnerNoSee(bUseDedicatedFirstPersonPresentation);
	FirstPersonPresentationMeshComponent->SetVisibility(bUseDedicatedFirstPersonPresentation, true);
	FirstPersonPresentationMeshComponent->SetHiddenInGame(!bUseDedicatedFirstPersonPresentation, true);

	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = FirstPersonPresentationFieldOfView;
	FirstPersonCameraComponent->FirstPersonScale = FirstPersonPresentationScale;

	for (const FName BoneName : FirstPersonHiddenBones)
	{
		if (BoneName.IsNone() || FirstPersonPresentationMeshComponent->GetBoneIndex(BoneName) == INDEX_NONE)
		{
			continue;
		}

		if (bUseDedicatedFirstPersonPresentation)
		{
			FirstPersonPresentationMeshComponent->HideBoneByName(BoneName, EPhysBodyOp::PBO_None);
		}
		else
		{
			FirstPersonPresentationMeshComponent->UnHideBoneByName(BoneName);
		}
	}

	UpdateFirstPersonPresentationAlignment();
	RefreshLocalViewSettings();
}

void AAGASSCharacter::RefreshLocalViewSettings()
{
	if (!IsLocallyControlled() || FirstPersonCameraComponent == nullptr)
	{
		return;
	}

	const UAGASSSettingsLocal* const LocalSettings = UAGASSSettingsLocal::Get();
	const float DesiredFieldOfView = LocalSettings != nullptr ? LocalSettings->GetGameplayFieldOfView() : 100.0f;
	if (!FMath::IsNearlyEqual(FirstPersonCameraComponent->FieldOfView, DesiredFieldOfView))
	{
		FirstPersonCameraComponent->SetFieldOfView(DesiredFieldOfView);
	}
}

void AAGASSCharacter::UpdateFirstPersonPresentationAlignment()
{
	USkeletalMeshComponent* const CharacterMeshComponent = GetMesh();
	if (!IsLocallyControlled()
		|| bDeathRagdollActive
		|| CharacterMeshComponent == nullptr
		|| FirstPersonPresentationMeshComponent == nullptr
		|| FirstPersonPresentationAlignmentBone.IsNone())
	{
		return;
	}

	const FQuat BaseMeshRotation = CharacterMeshComponent->GetRelativeRotation().Quaternion();
	const FQuat OffsetRotation = FirstPersonPresentationOffset.GetRotation();
	const FQuat DesiredRotation = OffsetRotation * BaseMeshRotation;
	const FVector DesiredScale = CharacterMeshComponent->GetRelativeScale3D() * FirstPersonPresentationOffset.GetScale3D();
	const FVector DesiredAlignmentLocation =
		(FirstPersonCameraComponent != nullptr ? FirstPersonCameraComponent->GetRelativeLocation() : FVector::ZeroVector)
		+ FirstPersonPresentationOffset.GetLocation();

	const bool bHasAlignmentBone =
		CharacterMeshComponent->DoesSocketExist(FirstPersonPresentationAlignmentBone)
		|| CharacterMeshComponent->GetBoneIndex(FirstPersonPresentationAlignmentBone) != INDEX_NONE;
	if (!bHasAlignmentBone)
	{
		FirstPersonPresentationMeshComponent->SetRelativeLocation(DesiredAlignmentLocation);
		FirstPersonPresentationMeshComponent->SetRelativeRotation(DesiredRotation);
		FirstPersonPresentationMeshComponent->SetRelativeScale3D(DesiredScale);
		return;
	}

	const FTransform AlignmentBoneTransform = CharacterMeshComponent->GetSocketTransform(
		FirstPersonPresentationAlignmentBone,
		ERelativeTransformSpace::RTS_Component);
	const FVector ScaledAlignmentLocation = AlignmentBoneTransform.GetLocation() * DesiredScale;
	const FVector DesiredLocation = DesiredAlignmentLocation - DesiredRotation.RotateVector(ScaledAlignmentLocation);

	FirstPersonPresentationMeshComponent->SetRelativeLocation(DesiredLocation);
	FirstPersonPresentationMeshComponent->SetRelativeRotation(DesiredRotation);
	FirstPersonPresentationMeshComponent->SetRelativeScale3D(DesiredScale);
}

void AAGASSCharacter::RefreshDangerousFallSettings()
{
	bDangerousFallDeathEnabled = true;
	DangerousFallSecondsThreshold = 2.f;
	DangerousFallMinSpeed = 200.f;

	if (const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get())
	{
		if (const UAGASSRespawnTuningData* const RespawnTuning = TowerSettings->DefaultRespawnTuning.LoadSynchronous())
		{
			bDangerousFallDeathEnabled = RespawnTuning->bEnableTimedFallDeath;
			DangerousFallSecondsThreshold = FMath::Max(RespawnTuning->MaxDangerousFallSeconds, 0.f);
			DangerousFallMinSpeed = FMath::Max(RespawnTuning->MinDangerousFallSpeed, 0.f);
		}
	}
}

void AAGASSCharacter::UpdateDangerousFallState(const float DeltaSeconds)
{
	if (!HasAuthority() || !bDangerousFallDeathEnabled || bDangerousFallDeathTriggered || bDeathRagdollActive)
	{
		return;
	}

	const UCharacterMovementComponent* const MovementComponent = GetCharacterMovement();
	if (MovementComponent == nullptr || MovementComponent->MovementMode != MOVE_Falling)
	{
		ResetDangerousFallState();
		return;
	}

	if (MovementComponent->Velocity.Z >= -DangerousFallMinSpeed)
	{
		if (!bDangerousFallDeathPending)
		{
			AccumulatedDangerousFallSeconds = 0.f;
		}
		return;
	}

	if (bDangerousFallDeathPending)
	{
		return;
	}

	AccumulatedDangerousFallSeconds += DeltaSeconds;
	if (AccumulatedDangerousFallSeconds < DangerousFallSecondsThreshold)
	{
		return;
	}

	bDangerousFallDeathPending = true;
}

void AAGASSCharacter::ResetDangerousFallState()
{
	AccumulatedDangerousFallSeconds = 0.f;
	bDangerousFallDeathPending = false;
	bDangerousFallDeathTriggered = false;
}

void AAGASSCharacter::MoveForward(const float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
}

void AAGASSCharacter::MoveRight(const float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
}

void AAGASSCharacter::Turn(const float Value)
{
	AddControllerYawInput(Value);
}

void AAGASSCharacter::LookUp(const float Value)
{
	AddControllerPitchInput(Value);
}
