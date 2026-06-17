#include "Actors/AGASSSellZone.h"

#include "Actors/AGASSPlaceableItemActor.h"
#include "CanvasItem.h"
#include "Components/AGASSTeamWalletComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "EngineFontServices.h"
#include "Engine/Font.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Fonts/SlateFontInfo.h"
#include "GameFramework/GameStateBase.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Subsystems/AGASSGameplayEventSubsystem.h"
#include "TimerManager.h"

AAGASSSellZone::AAGASSSellZone()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;

	SellVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("SellVolume"));
	SetRootComponent(SellVolume);

	SellVolume->InitBoxExtent(FVector(125.f, 125.f, 125.f));
	SellVolume->SetCollisionProfileName(TEXT("Trigger"));
	SellVolume->SetGenerateOverlapEvents(true);
	SellVolume->SetCanEverAffectNavigation(false);

	SellZoneVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SellZoneVisualMesh"));
	SellZoneVisualMesh->SetupAttachment(SellVolume);
	SellZoneVisualMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SellZoneVisualMesh->SetGenerateOverlapEvents(false);
	SellZoneVisualMesh->SetCanEverAffectNavigation(false);
	SellZoneVisualMesh->SetCastShadow(false);

	BannerText = FText::GetEmpty();
	BannerFont = nullptr;
	BannerTextureWidth = 1024;
	BannerTextureHeight = 256;
	BannerFontSize = 32.0f;
	BannerTextScale = 1.0f;
	BannerTextSpacing = 64.0f;
	BannerTextureParameterName = TEXT("BannerTextTexture");
	BannerTextScaleParameterName = TEXT("TextScale");
	BannerTextPeriodScaleParameterName = TEXT("TextPeriodScale");
	CachedBannerTextPeriodScale = 1.0f;
}

void AAGASSSellZone::BeginPlay()
{
	Super::BeginPlay();

	SellVolume->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleSellVolumeBeginOverlap);

	if (HasAuthority() && GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().SetTimer(SellVolumeScanTimerHandle, this, &ThisClass::ScanSellVolume, 0.15f, true);
	}

	if (ShouldRenderLocalizedBannerText())
	{
		RefreshBannerTextTexture();

		if (!TextRevisionChangedHandle.IsValid())
		{
			TextRevisionChangedHandle = FTextLocalizationManager::Get().OnTextRevisionChangedEvent.AddUObject(this, &ThisClass::HandleLocalizedBannerTextChanged);
		}
	}
}

void AAGASSSellZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TextRevisionChangedHandle.IsValid())
	{
		FTextLocalizationManager::Get().OnTextRevisionChangedEvent.Remove(TextRevisionChangedHandle);
		TextRevisionChangedHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void AAGASSSellZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshBannerTextTexture();
}

void AAGASSSellZone::HandleLocalizedBannerTextChanged()
{
	RefreshBannerTextTexture();
}

void AAGASSSellZone::RefreshBannerTextTexture()
{
	if (!ShouldRenderLocalizedBannerText())
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	int32 SafeWidth = FMath::Max(128, BannerTextureWidth);
	int32 SafeHeight = FMath::Max(32, BannerTextureHeight);
	CachedBannerTextPeriodScale = 1.0f;

	UFont* Font = BannerFont.Get();
	if (Font == nullptr && GEngine != nullptr)
	{
		Font = GEngine->GetMediumFont();
	}

	const FString RenderString = BannerText.ToString().TrimStartAndEnd();
	if (Font != nullptr && !RenderString.IsEmpty() && FEngineFontServices::IsInitialized())
	{
		const float SafeFontSize = FMath::Max(1.0f, BannerFontSize);
		const float SafeSpacing = FMath::Max(0.0f, BannerTextSpacing);
		const FSlateFontInfo BannerFontInfo(Font, SafeFontSize);
		const TSharedPtr<FSlateFontMeasure> FontMeasure = FEngineFontServices::Get().GetFontMeasure();
		if (FontMeasure.IsValid())
		{
			const FVector2D MeasuredTextSize = FontMeasure->Measure(FText::FromString(RenderString), BannerFontInfo, 1.0f);
			if (MeasuredTextSize.X > KINDA_SMALL_NUMBER && MeasuredTextSize.Y > KINDA_SMALL_NUMBER)
			{
				SafeWidth = FMath::Max(1, FMath::CeilToInt(MeasuredTextSize.X + SafeSpacing));
				SafeHeight = FMath::Max(SafeHeight, FMath::CeilToInt(MeasuredTextSize.Y));
				CachedBannerTextPeriodScale = FMath::Max(1.0f, static_cast<float>(SafeWidth) / MeasuredTextSize.X);
			}
		}
	}

	const bool bNeedsNewRenderTarget = BannerRenderTarget == nullptr
		|| BannerRenderTarget->SizeX != SafeWidth
		|| BannerRenderTarget->SizeY != SafeHeight;

	if (bNeedsNewRenderTarget)
	{
		BannerRenderTarget = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(World, UCanvasRenderTarget2D::StaticClass(), SafeWidth, SafeHeight);
		if (BannerRenderTarget == nullptr)
		{
			return;
		}

		BannerRenderTarget->ClearColor = FLinearColor::Transparent;
		BannerRenderTarget->SetShouldClearRenderTargetOnReceiveUpdate(true);
		BannerRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &ThisClass::HandleBannerRenderTargetUpdate);
	}

	UpdateBannerMaterialInstances();

	if (BannerRenderTarget != nullptr)
	{
		BannerRenderTarget->UpdateResource();
	}
}

void AAGASSSellZone::UpdateBannerMaterialInstances()
{
	if (BannerTextureParameterName.IsNone() || SellZoneVisualMesh == nullptr)
	{
		return;
	}

	BannerDynamicMaterials.Reset();

	const int32 MaterialCount = SellZoneVisualMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(SellZoneVisualMesh->GetMaterial(MaterialIndex));
		if (DynamicMaterial == nullptr)
		{
			DynamicMaterial = SellZoneVisualMesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
		}

		if (DynamicMaterial == nullptr)
		{
			continue;
		}

		DynamicMaterial->SetTextureParameterValue(BannerTextureParameterName, BannerRenderTarget);
		if (!BannerTextScaleParameterName.IsNone())
		{
			DynamicMaterial->SetScalarParameterValue(BannerTextScaleParameterName, FMath::Max(0.1f, BannerTextScale));
		}
		if (!BannerTextPeriodScaleParameterName.IsNone())
		{
			DynamicMaterial->SetScalarParameterValue(BannerTextPeriodScaleParameterName, FMath::Max(1.0f, CachedBannerTextPeriodScale));
		}
		BannerDynamicMaterials.Add(DynamicMaterial);
	}
}

bool AAGASSSellZone::ShouldRenderLocalizedBannerText() const
{
	if (HasAnyFlags(RF_ClassDefaultObject) || BannerTextureParameterName.IsNone())
	{
		return false;
	}

	const UWorld* const World = GetWorld();
	if (World == nullptr || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	return true;
}

void AAGASSSellZone::HandleBannerRenderTargetUpdate(UCanvas* Canvas, int32 Width, int32 Height)
{
	if (Canvas == nullptr)
	{
		return;
	}

	UFont* Font = BannerFont.Get();
	if (Font == nullptr && GEngine != nullptr)
	{
		Font = GEngine->GetMediumFont();
	}
	if (Font == nullptr)
	{
		return;
	}

	if (BannerText.IsEmptyOrWhitespace())
	{
		return;
	}

	const float SafeFontSize = FMath::Max(1.0f, BannerFontSize);
	const float SafeSpacing = FMath::Max(0.0f, BannerTextSpacing);
	const FString RenderString = BannerText.ToString().TrimStartAndEnd();
	if (RenderString.IsEmpty())
	{
		return;
	}

	if (!FEngineFontServices::IsInitialized())
	{
		return;
	}

	const FText RenderText = FText::FromString(RenderString);
	const FSlateFontInfo BannerFontInfo(Font, SafeFontSize);
	const TSharedPtr<FSlateFontMeasure> FontMeasure = FEngineFontServices::Get().GetFontMeasure();
	if (!FontMeasure.IsValid())
	{
		return;
	}

	const FVector2D MeasuredTextSize = FontMeasure->Measure(RenderText, BannerFontInfo, 1.0f);
	const float TextWidth = MeasuredTextSize.X;
	const float TextHeight = MeasuredTextSize.Y;
	if (TextWidth <= KINDA_SMALL_NUMBER || TextHeight <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float DrawY = FMath::Max(0.0f, (static_cast<float>(Height) - TextHeight) * 0.5f);
	FCanvasTextItem TextItem(FVector2D::ZeroVector, RenderText, BannerFontInfo, FLinearColor::White);
	TextItem.Position = FVector2D(0.0f, DrawY);
	Canvas->DrawItem(TextItem);
}

void AAGASSSellZone::ScanSellVolume()
{
	if (!HasAuthority() || SellVolume == nullptr)
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSSellZoneScan), false);
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	if (!World->OverlapMultiByObjectType(
		OverlapResults,
		SellVolume->GetComponentLocation(),
		SellVolume->GetComponentQuat(),
		ObjectQueryParams,
		FCollisionShape::MakeBox(SellVolume->GetScaledBoxExtent()),
		QueryParams))
	{
		return;
	}

	TSet<const AActor*> ProcessedActors;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		const AActor* const OverlappedActor = OverlapResult.GetActor();
		if (OverlappedActor == nullptr || ProcessedActors.Contains(OverlappedActor))
		{
			continue;
		}

		ProcessedActors.Add(OverlappedActor);
		if (TrySellPlaceable(Cast<AAGASSPlaceableItemActor>(OverlapResult.GetActor())))
		{
			return;
		}
	}
}

void AAGASSSellZone::HandleSellVolumeBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	const int32 OtherBodyIndex,
	const bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	TrySellPlaceable(Cast<AAGASSPlaceableItemActor>(OtherActor));
}

bool AAGASSSellZone::TrySellPlaceable(AAGASSPlaceableItemActor* PlaceableItem)
{
	UAGASSTeamWalletComponent* const WalletComponent = GetWalletComponent();
	if (PlaceableItem == nullptr || WalletComponent == nullptr || PlaceableItem->IsHeldHidden())
	{
		return false;
	}

	const int32 SellValue = PlaceableItem->GetSellValue();
	if (SellValue <= 0)
	{
		return false;
	}

	WalletComponent->AddMoney(SellValue);
	UAGASSGameplayEventSubsystem::BroadcastGameplayEvent(const_cast<AAGASSSellZone*>(this), AGASSGameplayEventNames::ItemSold(), 1, static_cast<float>(SellValue), true);
	MulticastPlaySellSuccessSound(GetActorLocation());
	PlaceableItem->Destroy();
	return true;
}

UAGASSTeamWalletComponent* AAGASSSellZone::GetWalletComponent() const
{
	const AGameStateBase* const GameState = GetWorld() != nullptr ? GetWorld()->GetGameState() : nullptr;
	return GameState != nullptr ? GameState->FindComponentByClass<UAGASSTeamWalletComponent>() : nullptr;
}

void AAGASSSellZone::MulticastPlaySellSuccessSound_Implementation(const FVector_NetQuantize SoundLocation)
{
	if (SellSuccessSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SellSuccessSound, SoundLocation);
	}
}
