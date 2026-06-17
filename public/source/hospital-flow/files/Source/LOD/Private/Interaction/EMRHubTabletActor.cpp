#include "Interaction/EMRHubTabletActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/EMRInteractableComponent.h"

namespace
{
    constexpr TCHAR HubTabletUpgradesFlowLogPrefix[] = TEXT("[UpgradesFlow]");
}

AEMRHubTabletActor::AEMRHubTabletActor()
{
    PrimaryActorTick.bCanEverTick = false;

    TabletRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TabletRoot"));
    SetRootComponent(TabletRoot);

    TabletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TabletMesh"));
    TabletMesh->SetupAttachment(TabletRoot);
    TabletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));
    if (InteractableComponent)
    {
        InteractableComponent->SetDisplayName(FText::FromString(TEXT("Hub Tablet")));
    }

    TabletWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("TabletWidget"));
    TabletWidgetComponent->SetupAttachment(TabletMesh);
    TabletWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    TabletWidgetComponent->SetDrawAtDesiredSize(true);
    TabletWidgetComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TabletWidgetComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    TabletWidgetComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    TabletWidgetComponent->SetGenerateOverlapEvents(false);
}

void AEMRHubTabletActor::BeginPlay()
{
    Super::BeginPlay();

    InitializeWidgetComponent();
    InitializeOwnerPlayer();
}

void AEMRHubTabletActor::OnRep_Owner()
{
    Super::OnRep_Owner();

    InitializeOwnerPlayer();
}

void AEMRHubTabletActor::SetTabletInteractionEnabled(bool bEnabled)
{
    if (!TabletWidgetComponent)
    {
        return;
    }

    TabletWidgetComponent->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

    if (InteractableComponent)
    {
        InteractableComponent->SetEnabled(bEnabled);
    }
}

void AEMRHubTabletActor::RefreshOwnerPlayer()
{
    InitializeOwnerPlayer();
}

void AEMRHubTabletActor::SetTabletWidgetMode(const EEMRHubTabletWidgetMode NewWidgetMode)
{
    if (!TabletWidgetComponent || CurrentWidgetMode == NewWidgetMode)
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s HubTablet switching widget mode %d -> %d"),
        HubTabletUpgradesFlowLogPrefix,
        static_cast<int32>(CurrentWidgetMode),
        static_cast<int32>(NewWidgetMode));
    CurrentWidgetMode = NewWidgetMode;

    const TSubclassOf<UUserWidget> DesiredWidgetClass = ResolveWidgetClassForMode(CurrentWidgetMode);
    if (DesiredWidgetClass && TabletWidgetComponent->GetWidgetClass() != DesiredWidgetClass)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s HubTablet applying widget class %s"),
            HubTabletUpgradesFlowLogPrefix,
            *GetNameSafe(DesiredWidgetClass));
        TabletWidgetComponent->SetWidgetClass(DesiredWidgetClass);
        TabletWidgetComponent->InitWidget();
        InitializeOwnerPlayer();
    }
}

void AEMRHubTabletActor::InitializeOwnerPlayer()
{
    if (!TabletWidgetComponent)
    {
        return;
    }

    const APawn* PawnOwner = Cast<APawn>(GetOwner());
    if (!PawnOwner)
    {
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(PawnOwner->GetController());
    if (!PlayerController || !PlayerController->IsLocalController())
    {
        return;
    }

    TabletWidgetComponent->SetOwnerPlayer(PlayerController->GetLocalPlayer());
    TabletWidgetComponent->InitWidget();
}

void AEMRHubTabletActor::InitializeWidgetComponent()
{
    if (!TabletWidgetComponent)
    {
        return;
    }

    if (!NightShiftSelectionWidgetClass && TabletWidgetClass)
    {
        NightShiftSelectionWidgetClass = TabletWidgetClass;
    }

    if (const TSubclassOf<UUserWidget> InitialWidgetClass = ResolveWidgetClassForMode(CurrentWidgetMode))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s HubTablet initialize widget component with mode=%d class=%s"),
            HubTabletUpgradesFlowLogPrefix,
            static_cast<int32>(CurrentWidgetMode),
            *GetNameSafe(InitialWidgetClass));
        TabletWidgetComponent->SetWidgetClass(InitialWidgetClass);
    }

    if (bOverrideTabletWidgetTransform)
    {
        TabletWidgetComponent->SetRelativeTransform(TabletWidgetRelativeTransform);
    }
    TabletWidgetComponent->InitWidget();
}

TSubclassOf<UUserWidget> AEMRHubTabletActor::ResolveWidgetClassForMode(const EEMRHubTabletWidgetMode WidgetMode) const
{
    switch (WidgetMode)
    {
    case EEMRHubTabletWidgetMode::UpgradeVoting:
        if (UpgradeVoteWidgetClass)
        {
            return UpgradeVoteWidgetClass;
        }
        break;
    case EEMRHubTabletWidgetMode::NightShiftSelection:
    default:
        break;
    }

    if (NightShiftSelectionWidgetClass)
    {
        return NightShiftSelectionWidgetClass;
    }

    return TabletWidgetClass;
}

FGameplayEventData AEMRHubTabletActor::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    if (InteractableComponent)
    {
        return InteractableComponent->BuildInteractionEventData(InterActor);
    }

    return FGameplayEventData();
}

FText AEMRHubTabletActor::GetInteractableDisplayName_Implementation() const
{
    return InteractableComponent ? InteractableComponent->GetDisplayName() : FText::FromString(GetName());
}

bool AEMRHubTabletActor::IsInteractableEnabled_Implementation() const
{
    return InteractableComponent ? InteractableComponent->IsEnabled() : true;
}
