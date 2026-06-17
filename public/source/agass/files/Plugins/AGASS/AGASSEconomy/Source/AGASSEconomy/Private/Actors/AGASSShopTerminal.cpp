#include "Actors/AGASSShopTerminal.h"

#include "Actors/AGASSCarryableItemActor.h"
#include "Actors/AGASSPlaceableItemActor.h"
#include "Components/AGASSInteractionComponent.h"
#include "Components/AGASSTeamWalletComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Data/AGASSItemDefinition.h"
#include "Data/AGASSShopCatalog.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Font.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/AGASSEconomyDeveloperSettings.h"
#include "Settings/AGASSTowerDeveloperSettings.h"
#include "Subsystems/AGASSGameplayEventSubsystem.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSShopTerminal, Log, All);

AAGASSShopTerminal::AAGASSShopTerminal()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TerminalMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalMesh"));
	TerminalMeshComponent->SetupAttachment(SceneRoot);
	TerminalMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	TerminalMeshComponent->SetGenerateOverlapEvents(false);
	TerminalMeshComponent->SetCanEverAffectNavigation(false);
	TerminalMeshComponent->SetSimulatePhysics(false);

	PurchaseSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PurchaseSpawnPoint"));
	PurchaseSpawnPoint->SetupAttachment(SceneRoot);
	PurchaseSpawnPoint->SetRelativeLocation(FVector(125.f, 0.f, 40.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		FallbackCubeMesh = CubeMesh.Object;
		TerminalMeshComponent->SetStaticMesh(CubeMesh.Object);
		TerminalMeshComponent->SetRelativeScale3D(FVector(0.9f, 0.9f, 1.6f));
	}

	ApplyLegacyTerminalPresentationState();
}

void AAGASSShopTerminal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyLegacyTerminalPresentationState();
	RebuildGeneratedShopSlots();
}

void AAGASSShopTerminal::BeginPlay()
{
	Super::BeginPlay();

	ApplyLegacyTerminalPresentationState();
	RebuildGeneratedShopSlots();
}

void AAGASSShopTerminal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearGeneratedShopSlots();

	Super::EndPlay(EndPlayReason);
}

void AAGASSShopTerminal::RebuildGeneratedShopSlots()
{
	ClearGeneratedShopSlots();
	ApplyLegacyTerminalPresentationState();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	const UAGASSShopCatalog* const ResolvedCatalog = ResolveCatalog();
	if (ResolvedCatalog == nullptr)
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' could not resolve a shop catalog."), *GetNameSafe(this));
		return;
	}

	if (ResolvedCatalog->Entries.IsEmpty())
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' resolved an empty shop catalog '%s'."), *GetNameSafe(this), *GetNameSafe(ResolvedCatalog));
		return;
	}

	struct FResolvedGeneratedSlotEntry
	{
		int32 CatalogIndex = INDEX_NONE;
		TObjectPtr<UStaticMesh> DisplayMesh = nullptr;
	};

	TArray<FResolvedGeneratedSlotEntry> ResolvedEntries;
	ResolvedEntries.Reserve(ResolvedCatalog->Entries.Num());

	for (int32 EntryIndex = 0; EntryIndex < ResolvedCatalog->Entries.Num(); ++EntryIndex)
	{
		const FAGASSShopCatalogEntry* const Entry = ResolveCatalogEntryByIndex(*ResolvedCatalog, EntryIndex);
		if (Entry == nullptr)
		{
			continue;
		}

		UStaticMesh* const DisplayMesh = ResolveEntryDisplayMesh(*Entry);
		if (DisplayMesh == nullptr)
		{
			UE_LOG(
				LogAGASSShopTerminal,
				Warning,
				TEXT("Shop '%s' skipped catalog entry '%s' because it could not resolve a display mesh."),
				*GetNameSafe(this),
				Entry->EntryId.IsNone() ? TEXT("<default>") : *Entry->EntryId.ToString());
			continue;
		}

		FResolvedGeneratedSlotEntry& ResolvedEntry = ResolvedEntries.AddDefaulted_GetRef();
		ResolvedEntry.CatalogIndex = EntryIndex;
		ResolvedEntry.DisplayMesh = DisplayMesh;
	}

	if (ResolvedEntries.IsEmpty())
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' could not build any visible shop slots."), *GetNameSafe(this));
		return;
	}

	auto RegisterGeneratedComponent = [this](UActorComponent* Component)
	{
		if (Component == nullptr)
		{
			return;
		}

		AddInstanceComponent(Component);
		Component->CreationMethod = EComponentCreationMethod::Instance;
		if (GetWorld() != nullptr)
		{
			Component->RegisterComponent();
		}
	};

	const float SafeEntrySpacing = FMath::Max(EntrySpacing, 0.f);
	const float SafePriceTextWorldSize = FMath::Max(PriceTextWorldSize, 1.f);
	const FVector SafePurchaseTriggerExtent = PurchaseTriggerExtent.GetAbs();
	UStaticMesh* const ResolvedPlatformMesh = PlatformMesh.Get() != nullptr ? PlatformMesh.Get() : FallbackCubeMesh.Get();

	if (ResolvedPlatformMesh == nullptr)
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' has no platform mesh and no fallback cube mesh."), *GetNameSafe(this));
	}

	const float CenterOffset = 0.5f * static_cast<float>(ResolvedEntries.Num() - 1);
	for (int32 SlotIndex = 0; SlotIndex < ResolvedEntries.Num(); ++SlotIndex)
	{
		const FResolvedGeneratedSlotEntry& ResolvedEntry = ResolvedEntries[SlotIndex];
		const FAGASSShopCatalogEntry* const Entry = ResolveCatalogEntryByIndex(*ResolvedCatalog, ResolvedEntry.CatalogIndex);
		if (Entry == nullptr)
		{
			continue;
		}

		FAGASSGeneratedShopSlot& GeneratedSlot = GeneratedSlots.AddDefaulted_GetRef();
		GeneratedSlot.CatalogIndex = ResolvedEntry.CatalogIndex;

		GeneratedSlot.SlotRoot = NewObject<USceneComponent>(this, MakeUniqueObjectName(this, USceneComponent::StaticClass(), TEXT("GeneratedShopSlotRoot")));
		GeneratedSlot.SlotRoot->SetupAttachment(SceneRoot);
		GeneratedSlot.SlotRoot->SetRelativeLocation(FVector(0.f, (static_cast<float>(SlotIndex) - CenterOffset) * SafeEntrySpacing, 0.f));
		RegisterGeneratedComponent(GeneratedSlot.SlotRoot);

		GeneratedSlot.PurchaseSpawnPoint = NewObject<USceneComponent>(this, MakeUniqueObjectName(this, USceneComponent::StaticClass(), TEXT("GeneratedShopPurchaseSpawnPoint")));
		GeneratedSlot.PurchaseSpawnPoint->SetupAttachment(GeneratedSlot.SlotRoot);
		if (PurchaseSpawnPoint != nullptr)
		{
			GeneratedSlot.PurchaseSpawnPoint->SetRelativeTransform(PurchaseSpawnPoint->GetRelativeTransform());
		}
		RegisterGeneratedComponent(GeneratedSlot.PurchaseSpawnPoint);

		GeneratedSlot.DisplayMeshComponent = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("GeneratedShopDisplayMesh")));
		GeneratedSlot.DisplayMeshComponent->SetupAttachment(GeneratedSlot.SlotRoot);
		GeneratedSlot.DisplayMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		GeneratedSlot.DisplayMeshComponent->SetGenerateOverlapEvents(false);
		GeneratedSlot.DisplayMeshComponent->SetCanEverAffectNavigation(false);
		GeneratedSlot.DisplayMeshComponent->SetSimulatePhysics(false);
		GeneratedSlot.DisplayMeshComponent->SetStaticMesh(ResolvedEntry.DisplayMesh);
		GeneratedSlot.DisplayMeshComponent->SetRelativeRotation(DisplayRotationOffset);

		const float DisplayScale = ResolveDisplayScale(*ResolvedEntry.DisplayMesh);
		const FBoxSphereBounds DisplayBounds = ResolvedEntry.DisplayMesh->GetBounds();
		const FVector DisplayScaleVector(DisplayScale);
		GeneratedSlot.DisplayMeshComponent->SetRelativeLocation(
			DisplayOffset - (DisplayBounds.Origin * DisplayScaleVector) + FVector(0.f, 0.f, DisplayBounds.BoxExtent.Z * DisplayScale));
		GeneratedSlot.DisplayMeshComponent->SetRelativeScale3D(DisplayScaleVector);
		RegisterGeneratedComponent(GeneratedSlot.DisplayMeshComponent);

		if (ResolvedPlatformMesh != nullptr)
		{
			GeneratedSlot.PlatformMeshComponent = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("GeneratedShopPlatformMesh")));
			GeneratedSlot.PlatformMeshComponent->SetupAttachment(GeneratedSlot.SlotRoot);
			GeneratedSlot.PlatformMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			GeneratedSlot.PlatformMeshComponent->SetGenerateOverlapEvents(false);
			GeneratedSlot.PlatformMeshComponent->SetCanEverAffectNavigation(false);
			GeneratedSlot.PlatformMeshComponent->SetSimulatePhysics(false);
			GeneratedSlot.PlatformMeshComponent->SetStaticMesh(ResolvedPlatformMesh);
			GeneratedSlot.PlatformMeshComponent->SetRelativeLocation(PlatformOffset);
			GeneratedSlot.PlatformMeshComponent->SetRelativeScale3D(PlatformScale);
			RegisterGeneratedComponent(GeneratedSlot.PlatformMeshComponent);
		}

		GeneratedSlot.PriceTextComponent = NewObject<UTextRenderComponent>(this, MakeUniqueObjectName(this, UTextRenderComponent::StaticClass(), TEXT("GeneratedShopPriceText")));
		GeneratedSlot.PriceTextComponent->SetupAttachment(GeneratedSlot.SlotRoot);
		GeneratedSlot.PriceTextComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		GeneratedSlot.PriceTextComponent->SetGenerateOverlapEvents(false);
		GeneratedSlot.PriceTextComponent->SetCanEverAffectNavigation(false);
		GeneratedSlot.PriceTextComponent->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		GeneratedSlot.PriceTextComponent->SetWorldSize(SafePriceTextWorldSize);
		GeneratedSlot.PriceTextComponent->SetRelativeLocation(PriceTextOffset);
		GeneratedSlot.PriceTextComponent->SetTextRenderColor(PriceTextColor);
		GeneratedSlot.PriceTextComponent->SetText(ResolveEntryPriceText(*Entry));
		if (PriceFont != nullptr)
		{
			GeneratedSlot.PriceTextComponent->SetFont(PriceFont);
		}
		RegisterGeneratedComponent(GeneratedSlot.PriceTextComponent);

		GeneratedSlot.PurchaseTriggerComponent = NewObject<UBoxComponent>(this, MakeUniqueObjectName(this, UBoxComponent::StaticClass(), TEXT("GeneratedShopPurchaseTrigger")));
		GeneratedSlot.PurchaseTriggerComponent->SetupAttachment(GeneratedSlot.SlotRoot);
		GeneratedSlot.PurchaseTriggerComponent->InitBoxExtent(SafePurchaseTriggerExtent);
		GeneratedSlot.PurchaseTriggerComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		GeneratedSlot.PurchaseTriggerComponent->SetCollisionObjectType(ECC_WorldDynamic);
		GeneratedSlot.PurchaseTriggerComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		GeneratedSlot.PurchaseTriggerComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		GeneratedSlot.PurchaseTriggerComponent->SetCanEverAffectNavigation(false);
		GeneratedSlot.PurchaseTriggerComponent->SetGenerateOverlapEvents(true);
		GeneratedSlot.PurchaseTriggerComponent->SetRelativeLocation(PlatformOffset + FVector(0.f, 0.f, SafePurchaseTriggerExtent.Z));
		GeneratedSlot.PurchaseTriggerComponent->SetHiddenInGame(true);
		GeneratedSlot.PurchaseTriggerComponent->SetVisibility(false, true);
		GeneratedSlot.PurchaseTriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandlePurchaseTriggerBeginOverlap);
		RegisterGeneratedComponent(GeneratedSlot.PurchaseTriggerComponent);

		PurchaseTriggerToSlotIndex.Add(GeneratedSlot.PurchaseTriggerComponent, GeneratedSlots.Num() - 1);
	}
}

void AAGASSShopTerminal::ClearGeneratedShopSlots()
{
	for (FAGASSGeneratedShopSlot& GeneratedSlot : GeneratedSlots)
	{
		if (GeneratedSlot.PurchaseTriggerComponent != nullptr)
		{
			GeneratedSlot.PurchaseTriggerComponent->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::HandlePurchaseTriggerBeginOverlap);
		}

		DestroyGeneratedComponent(GeneratedSlot.PriceTextComponent);
		DestroyGeneratedComponent(GeneratedSlot.PurchaseTriggerComponent);
		DestroyGeneratedComponent(GeneratedSlot.PlatformMeshComponent);
		DestroyGeneratedComponent(GeneratedSlot.DisplayMeshComponent);
		DestroyGeneratedComponent(GeneratedSlot.PurchaseSpawnPoint);
		DestroyGeneratedComponent(GeneratedSlot.SlotRoot);
	}

	GeneratedSlots.Reset();
	PurchaseTriggerToSlotIndex.Reset();
}

const UAGASSShopCatalog* AAGASSShopTerminal::ResolveCatalog() const
{
	if (UAGASSShopCatalog* const ExplicitCatalog = ShopCatalog.LoadSynchronous())
	{
		return ExplicitCatalog;
	}

	if (const UAGASSEconomyDeveloperSettings* const EconomySettings = UAGASSEconomyDeveloperSettings::Get())
	{
		return EconomySettings->DefaultShopCatalog.LoadSynchronous();
	}

	return nullptr;
}

const FAGASSShopCatalogEntry* AAGASSShopTerminal::ResolveCatalogEntryByIndex(const UAGASSShopCatalog& Catalog, const int32 EntryIndex) const
{
	return Catalog.Entries.IsValidIndex(EntryIndex) ? &Catalog.Entries[EntryIndex] : nullptr;
}

UAGASSItemDefinition* AAGASSShopTerminal::ResolveEntryItemDefinition(const FAGASSShopCatalogEntry& Entry) const
{
	if (UAGASSItemDefinition* const ExplicitDefinition = Entry.ItemDefinition.LoadSynchronous())
	{
		return ExplicitDefinition;
	}

	if (UClass* const ItemClass = ResolveEntryItemClass(Entry))
	{
		if (const AAGASSCarryableItemActor* const ItemDefaults = ItemClass->GetDefaultObject<AAGASSCarryableItemActor>())
		{
			return const_cast<UAGASSItemDefinition*>(ItemDefaults->GetItemDefinition());
		}
	}

	return nullptr;
}

UClass* AAGASSShopTerminal::ResolveEntryItemClass(const FAGASSShopCatalogEntry& Entry) const
{
	if (UAGASSItemDefinition* const ItemDefinition = Entry.ItemDefinition.LoadSynchronous())
	{
		if (UClass* const DefinitionActorClass = ItemDefinition->ResolveItemActorClass())
		{
			return DefinitionActorClass;
		}
	}

	if (UClass* const ExplicitItemClass = Entry.ItemClass.LoadSynchronous())
	{
		return ExplicitItemClass->IsChildOf(AAGASSCarryableItemActor::StaticClass()) ? ExplicitItemClass : nullptr;
	}

	if (const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get())
	{
		if (UClass* const DefaultItemClass = TowerSettings->DefaultBootstrapItemClass.LoadSynchronous())
		{
			return DefaultItemClass;
		}
	}

	return AAGASSPlaceableItemActor::StaticClass();
}

UStaticMesh* AAGASSShopTerminal::ResolveEntryDisplayMesh(const FAGASSShopCatalogEntry& Entry) const
{
	if (UAGASSItemDefinition* const ItemDefinition = ResolveEntryItemDefinition(Entry))
	{
		if (UStaticMesh* const ExplicitDisplayMesh = ItemDefinition->WorldStaticMesh.LoadSynchronous())
		{
			return ExplicitDisplayMesh;
		}
	}

	if (UClass* const ItemClass = ResolveEntryItemClass(Entry))
	{
		if (const AActor* const ItemDefaults = ItemClass->GetDefaultObject<AActor>())
		{
			if (const UStaticMeshComponent* const RootMeshComponent = Cast<UStaticMeshComponent>(ItemDefaults->GetRootComponent()))
			{
				if (UStaticMesh* const RootMesh = RootMeshComponent->GetStaticMesh())
				{
					return RootMesh;
				}
			}

			if (const UStaticMeshComponent* const FirstStaticMeshComponent = ItemDefaults->FindComponentByClass<UStaticMeshComponent>())
			{
				return FirstStaticMeshComponent->GetStaticMesh();
			}
		}
	}

	return nullptr;
}

FText AAGASSShopTerminal::ResolveEntryPriceText(const FAGASSShopCatalogEntry& Entry) const
{
	return FText::AsNumber(Entry.ResolvePurchaseCost(ResolveEntryItemDefinition(Entry)));
}

FTransform AAGASSShopTerminal::ResolveSlotPurchaseSpawnTransform(const FAGASSShopCatalogEntry& Entry, const USceneComponent& SlotPurchaseSpawnPoint) const
{
	const FTransform BaseTransform = SlotPurchaseSpawnPoint.GetComponentTransform();
	return FTransform(
		Entry.SpawnOffset.GetRotation() * BaseTransform.GetRotation(),
		BaseTransform.TransformPosition(Entry.SpawnOffset.GetLocation()),
		BaseTransform.GetScale3D() * Entry.SpawnOffset.GetScale3D());
}

float AAGASSShopTerminal::ResolveDisplayScale(const UStaticMesh& DisplayMesh) const
{
	const float SafeDisplayMaxDimension = FMath::Max(DisplayMaxDimension, 1.f);
	const FVector DisplayMeshSize = DisplayMesh.GetBounds().BoxExtent * 2.f;
	const float LargestDimension = DisplayMeshSize.GetMax();
	return LargestDimension > KINDA_SMALL_NUMBER ? SafeDisplayMaxDimension / LargestDimension : 1.f;
}

bool AAGASSShopTerminal::TryPurchaseSlot(const int32 GeneratedSlotIndex, APawn* PurchasingPawn, EAGASSShopPurchaseFailureReason& OutFailureReason)
{
	OutFailureReason = EAGASSShopPurchaseFailureReason::None;

	if (!HasAuthority() || PurchasingPawn == nullptr || !GeneratedSlots.IsValidIndex(GeneratedSlotIndex))
	{
		return false;
	}

	AController* const PurchasingController = PurchasingPawn->GetController();
	if (PurchasingController == nullptr)
	{
		return false;
	}

	const UAGASSShopCatalog* const ResolvedCatalog = ResolveCatalog();
	if (ResolvedCatalog == nullptr)
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' rejected a purchase because it could not resolve a catalog at runtime."), *GetNameSafe(this));
		OutFailureReason = EAGASSShopPurchaseFailureReason::ContentError;
		return false;
	}

	const FAGASSGeneratedShopSlot& GeneratedSlot = GeneratedSlots[GeneratedSlotIndex];
	const FAGASSShopCatalogEntry* const Entry = ResolveCatalogEntryByIndex(*ResolvedCatalog, GeneratedSlot.CatalogIndex);
	if (Entry == nullptr)
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' rejected a purchase because slot %d referenced an invalid catalog entry."), *GetNameSafe(this), GeneratedSlot.CatalogIndex);
		OutFailureReason = EAGASSShopPurchaseFailureReason::ContentError;
		return false;
	}

	UAGASSInteractionComponent* const InteractionComponent = FindInteractionComponent(PurchasingController);
	if (InteractionComponent == nullptr)
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' rejected a purchase for pawn '%s' because its controller had no interaction component."), *GetNameSafe(this), *GetNameSafe(PurchasingPawn));
		OutFailureReason = EAGASSShopPurchaseFailureReason::ContentError;
		return false;
	}

	if (InteractionComponent->GetHeldItem() != nullptr)
	{
		OutFailureReason = EAGASSShopPurchaseFailureReason::HoldingItem;
		return false;
	}

	UAGASSTeamWalletComponent* const WalletComponent = GetWalletComponent();
	if (WalletComponent == nullptr)
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' rejected a purchase because no team wallet component was available."), *GetNameSafe(this));
		OutFailureReason = EAGASSShopPurchaseFailureReason::ContentError;
		return false;
	}

	UAGASSItemDefinition* const ItemDefinition = ResolveEntryItemDefinition(*Entry);
	const int32 PurchaseCost = Entry->ResolvePurchaseCost(ItemDefinition);
	if (!WalletComponent->TrySpendMoney(PurchaseCost))
	{
		OutFailureReason = EAGASSShopPurchaseFailureReason::InsufficientFunds;
		return false;
	}

	UClass* const SpawnClass = ResolveEntryItemClass(*Entry);
	if (SpawnClass == nullptr)
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' could not resolve a spawn class for catalog entry '%s'."), *GetNameSafe(this), Entry->EntryId.IsNone() ? TEXT("<default>") : *Entry->EntryId.ToString());
		WalletComponent->AddMoney(PurchaseCost);
		OutFailureReason = EAGASSShopPurchaseFailureReason::ContentError;
		return false;
	}

	const USceneComponent* const SlotPurchaseSpawnPoint = GeneratedSlot.PurchaseSpawnPoint != nullptr ? GeneratedSlot.PurchaseSpawnPoint : PurchaseSpawnPoint;
	if (SlotPurchaseSpawnPoint == nullptr)
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' could not resolve a purchase spawn point for catalog entry '%s'."), *GetNameSafe(this), Entry->EntryId.IsNone() ? TEXT("<default>") : *Entry->EntryId.ToString());
		WalletComponent->AddMoney(PurchaseCost);
		OutFailureReason = EAGASSShopPurchaseFailureReason::ContentError;
		return false;
	}

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		WalletComponent->AddMoney(PurchaseCost);
		OutFailureReason = EAGASSShopPurchaseFailureReason::ContentError;
		return false;
	}

	const FTransform SpawnTransform = ResolveSlotPurchaseSpawnTransform(*Entry, *SlotPurchaseSpawnPoint);
	AAGASSCarryableItemActor* const PurchasedItem = World->SpawnActorDeferred<AAGASSCarryableItemActor>(
		SpawnClass,
		SpawnTransform,
		PurchasingController,
		PurchasingPawn,
		Entry->bAutoClaimIntoCarry
			? ESpawnActorCollisionHandlingMethod::AlwaysSpawn
			: ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (PurchasedItem == nullptr)
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' failed to spawn purchased item '%s'."), *GetNameSafe(this), *GetNameSafe(SpawnClass));
		WalletComponent->AddMoney(PurchaseCost);
		OutFailureReason = EAGASSShopPurchaseFailureReason::ContentError;
		return false;
	}

	if (ItemDefinition != nullptr)
	{
		PurchasedItem->SetItemDefinition(ItemDefinition);
	}

	PurchasedItem->FinishSpawning(SpawnTransform);

	if (Entry->bAutoClaimIntoCarry && !InteractionComponent->TryBeginServerHoldFromSpawnedItem(PurchasedItem))
	{
		UE_LOG(LogAGASSShopTerminal, Warning, TEXT("Shop '%s' failed to auto-claim purchased item '%s' for pawn '%s'."), *GetNameSafe(this), *GetNameSafe(PurchasedItem), *GetNameSafe(PurchasingPawn));
		PurchasedItem->Destroy();
		WalletComponent->AddMoney(PurchaseCost);
		OutFailureReason = EAGASSShopPurchaseFailureReason::ContentError;
		return false;
	}

	UAGASSGameplayEventSubsystem::BroadcastGameplayEvent(this, AGASSGameplayEventNames::ItemPurchased(), 1, static_cast<float>(PurchaseCost), true);
	const FVector SoundLocation = GeneratedSlot.PlatformMeshComponent != nullptr
		? GeneratedSlot.PlatformMeshComponent->GetComponentLocation()
		: GeneratedSlot.PurchaseTriggerComponent != nullptr
			? GeneratedSlot.PurchaseTriggerComponent->GetComponentLocation()
			: GeneratedSlot.SlotRoot != nullptr
				? GeneratedSlot.SlotRoot->GetComponentLocation()
				: GetActorLocation();
	MulticastPlayPurchaseSuccessSound(SoundLocation);
	return true;
}

void AAGASSShopTerminal::PlayFailureSoundAtSlot(const int32 GeneratedSlotIndex, const EAGASSShopPurchaseFailureReason FailureReason)
{
	if (!HasAuthority() || !GeneratedSlots.IsValidIndex(GeneratedSlotIndex) || ResolveFailureSound(FailureReason) == nullptr)
	{
		return;
	}

	const FAGASSGeneratedShopSlot& GeneratedSlot = GeneratedSlots[GeneratedSlotIndex];
	const FVector SoundLocation = GeneratedSlot.PlatformMeshComponent != nullptr
		? GeneratedSlot.PlatformMeshComponent->GetComponentLocation()
		: GeneratedSlot.PurchaseTriggerComponent != nullptr
			? GeneratedSlot.PurchaseTriggerComponent->GetComponentLocation()
			: GeneratedSlot.SlotRoot != nullptr
				? GeneratedSlot.SlotRoot->GetComponentLocation()
				: GetActorLocation();

	MulticastPlayFailureSound(SoundLocation, FailureReason);
}

USoundBase* AAGASSShopTerminal::ResolveFailureSound(const EAGASSShopPurchaseFailureReason FailureReason) const
{
	switch (FailureReason)
	{
	case EAGASSShopPurchaseFailureReason::InsufficientFunds:
		return InsufficientFundsFailureSound;

	case EAGASSShopPurchaseFailureReason::HoldingItem:
		return HandsFullFailureSound;

	default:
		return nullptr;
	}
}

void AAGASSShopTerminal::ApplyLegacyTerminalPresentationState()
{
	if (TerminalMeshComponent == nullptr)
	{
		return;
	}

	TerminalMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	TerminalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TerminalMeshComponent->SetGenerateOverlapEvents(false);
	TerminalMeshComponent->SetCanEverAffectNavigation(false);
	TerminalMeshComponent->SetVisibility(false, true);
	TerminalMeshComponent->SetHiddenInGame(true, true);
}

void AAGASSShopTerminal::DestroyGeneratedComponent(UActorComponent* Component)
{
	if (Component == nullptr)
	{
		return;
	}

	if (Component->GetOwner() == this)
	{
		RemoveInstanceComponent(Component);
	}

	Component->DestroyComponent();
}

UAGASSTeamWalletComponent* AAGASSShopTerminal::GetWalletComponent() const
{
	const AGameStateBase* const GameState = GetWorld() != nullptr ? GetWorld()->GetGameState() : nullptr;
	return GameState != nullptr ? GameState->FindComponentByClass<UAGASSTeamWalletComponent>() : nullptr;
}

UAGASSInteractionComponent* AAGASSShopTerminal::FindInteractionComponent(const AController* InteractingController) const
{
	return InteractingController != nullptr ? InteractingController->FindComponentByClass<UAGASSInteractionComponent>() : nullptr;
}

void AAGASSShopTerminal::MulticastPlayFailureSound_Implementation(const FVector_NetQuantize SoundLocation, const EAGASSShopPurchaseFailureReason FailureReason)
{
	if (USoundBase* const FailureSound = ResolveFailureSound(FailureReason))
	{
		UGameplayStatics::PlaySoundAtLocation(this, FailureSound, SoundLocation);
	}
}

void AAGASSShopTerminal::MulticastPlayPurchaseSuccessSound_Implementation(const FVector_NetQuantize SoundLocation)
{
	if (PurchaseSuccessSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PurchaseSuccessSound, SoundLocation);
	}
}

void AAGASSShopTerminal::HandlePurchaseTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	const int32 OtherBodyIndex,
	const bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || OverlappedComponent == nullptr || OtherActor == nullptr || OtherComp == nullptr)
	{
		return;
	}

	const int32* const GeneratedSlotIndex = PurchaseTriggerToSlotIndex.Find(OverlappedComponent);
	if (GeneratedSlotIndex == nullptr)
	{
		return;
	}

	APawn* const PurchasingPawn = Cast<APawn>(OtherActor);
	if (PurchasingPawn == nullptr || OtherComp != PurchasingPawn->GetRootComponent())
	{
		return;
	}

	EAGASSShopPurchaseFailureReason FailureReason = EAGASSShopPurchaseFailureReason::None;
	if (!TryPurchaseSlot(*GeneratedSlotIndex, PurchasingPawn, FailureReason)
		&& FailureReason != EAGASSShopPurchaseFailureReason::None
		&& FailureReason != EAGASSShopPurchaseFailureReason::ContentError)
	{
		PlayFailureSoundAtSlot(*GeneratedSlotIndex, FailureReason);
	}
}
