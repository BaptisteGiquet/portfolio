
#include "Interaction/EMRInteractableComponent.h"

#include "Interfaces/EMRInteractableInterface.h"
#include "GAS/EMRTags.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/Interaction/EMRInteractableNameWidget.h"

UEMRInteractableComponent::UEMRInteractableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
    InteractionEventTag = EMRTags::Abilities::Interact;
}


void UEMRInteractableComponent::BeginPlay()
{
    Super::BeginPlay();

    if (DisplayName.IsEmpty() && GetOwner())
    {
        DisplayName = FText::FromString(GetOwner()->GetName());
    }

    CacheNameWidget();
    RefreshNameWidgetText();
    HideNameWidget();
}


void UEMRInteractableComponent::SetInteractionEventTag(const FGameplayTag& InTag)
{
    InteractionEventTag = InTag;
}


void UEMRInteractableComponent::SetDisplayName(const FText& InDisplayName)
{
    DisplayName = InDisplayName;
    RefreshNameWidgetText();
}


int32 UEMRInteractableComponent::GetCustomDepthStencilValue() const
{
    return ResolveOutlineStencilValue();
}


void UEMRInteractableComponent::ApplyHighlight()
{
    const int32 StencilValue = ResolveOutlineStencilValue();
    if (StencilValue <= 0)
    {
        ClearHighlight();
        return;
    }

    if (AActor* Owner = GetOwner())
    {
        TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Owner);

        for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
        {
            if (!PrimitiveComponent)
            {
                continue;
            }

            PrimitiveComponent->SetRenderCustomDepth(true);
            PrimitiveComponent->SetCustomDepthStencilValue(StencilValue);
        }
    }

    OnInteractableHoverChanged.Broadcast(true);
}


void UEMRInteractableComponent::ClearHighlight()
{
    if (AActor* Owner = GetOwner())
    {
        TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Owner);

        for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
        {
            if (!PrimitiveComponent)
            {
                continue;
            }

            PrimitiveComponent->SetRenderCustomDepth(false);
        }
    }

    OnInteractableHoverChanged.Broadcast(false);
}


void UEMRInteractableComponent::ShowNameWidget()
{
    CacheNameWidget();
    RefreshNameWidgetText();

    if (CachedNameWidgetComponent.IsValid())
    {
        CachedNameWidgetComponent->SetHiddenInGame(false);
        CachedNameWidgetComponent->SetVisibility(true);
    }
}


void UEMRInteractableComponent::HideNameWidget()
{
    if (CachedNameWidgetInstance.IsValid())
    {
        CachedNameWidgetInstance->ClearInteractableInfo();
    }

    if (CachedNameWidgetComponent.IsValid())
    {
        CachedNameWidgetComponent->SetHiddenInGame(true);
    }
}


FGameplayEventData UEMRInteractableComponent::BuildInteractionEventData(AActor* InteractingActor, const FGameplayTag& OverrideEventTag) const
{
    FGameplayEventData Data;
    Data.EventTag = InteractionEventTag;
    Data.Instigator = InteractingActor;
    Data.Target = GetOwner();
    return Data;
}


void UEMRInteractableComponent::CacheNameWidget()
{
    if (CachedNameWidgetComponent.IsValid())
    {
        if (!CachedNameWidgetInstance.IsValid())
        {
            CachedNameWidgetInstance = Cast<UEMRInteractableNameWidget>(CachedNameWidgetComponent->GetUserWidgetObject());
        }
        return;
    }

    if (AActor* Owner = GetOwner())
    {
        TInlineComponentArray<UWidgetComponent*> WidgetComponents(Owner);
        for (UWidgetComponent* WidgetComponent : WidgetComponents)
        {
            if (!WidgetComponent)
            {
                continue;
            }

            if (WidgetComponent->GetWidgetClass() && WidgetComponent->GetWidgetClass()->IsChildOf(UEMRInteractableNameWidget::StaticClass()))
            {
                CachedNameWidgetComponent = WidgetComponent;
                CachedNameWidgetInstance = Cast<UEMRInteractableNameWidget>(WidgetComponent->GetUserWidgetObject());
                CachedNameWidgetComponent->SetDrawAtDesiredSize(true);
                CachedNameWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                CachedNameWidgetComponent->SetVisibility(true);
                break;
            }
        }
    }
}


void UEMRInteractableComponent::RefreshNameWidgetText()
{
    if (CachedNameWidgetInstance.IsValid())
    {
        CachedNameWidgetInstance->UpdateInteractableInfo(DisplayName);
    }
}


int32 UEMRInteractableComponent::ResolveOutlineStencilValue() const
{
    if (bOverrideOutlineStencil)
    {
        return FMath::Clamp(CustomDepthStencilValue, 0, 255);
    }

    int32 ResolvedValue = 0;
    switch (OutlineProfile)
    {
    case EEMRInteractableOutlineProfile::ConfirmInput:
        ResolvedValue = 2;
        break;
    case EEMRInteractableOutlineProfile::GAInteract:
        ResolvedValue = 1;
        break;
    case EEMRInteractableOutlineProfile::None:
    default:
        ResolvedValue = 0;
        break;
    }

    return ResolvedValue;
}
