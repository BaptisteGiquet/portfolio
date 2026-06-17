
#include "Subsystems/EMRLocalizationSubsystem.h"

#include "Engine/DataTable.h"
#include "Data/EMRTagsLocalizationTable.h"
#include "GAS/EMRTags.h"
#include "Subsystems/EMRLocalizationDeveloperSettings.h"
#include "Internationalization/StringTableRegistry.h"

namespace
{
    FText BuildMissingLocalizationText(const FString& MissingIdentifier)
    {
        return FText::Format(
            NSLOCTEXT("EMRLocalization", "MissingLocalization", "[MISSING_LOCALIZATION: {0}]"),
            FText::FromString(MissingIdentifier)
        );
    }
}


void UEMRLocalizationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const UEMRLocalizationDeveloperSettings* LocalizationSettings = GetDefault<UEMRLocalizationDeveloperSettings>();
    if (LocalizationSettings)
    {
        GameplayTagLocalizationTable = LocalizationSettings->GetGameplayTagLocalizationTable();
        GameplayTagsStringTableId = LocalizationSettings->GetGameplayTagStringTableId();
    }

    if (GameplayTagLocalizationTable.IsNull())
    {
        UE_LOG(LogTemp, Error, TEXT("[LocalizationSubsystem] GameplayTagLocalizationTable is not configured in EMRLocalizationDeveloperSettings"));
    }

    if (GameplayTagsStringTableId.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("[LocalizationSubsystem] GameplayTagsStringTableId is not configured in EMRLocalizationDeveloperSettings"));
    }

    LoadLocalizationTable();

    UE_LOG(LogTemp, Log, TEXT("[LocalizationSubsystem] Initialized with %d cached tags"), CachedTagLocalizations.Num());
}


void UEMRLocalizationSubsystem::Deinitialize()
{
    CachedTagLocalizations.Empty();
    Super::Deinitialize();
}


void UEMRLocalizationSubsystem::LoadLocalizationTable()
{
    CachedTagLocalizations.Empty();

    UDataTable* DataTable = GameplayTagLocalizationTable.LoadSynchronous();
    if (!DataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("[LocalizationSubsystem] Failed to load GameplayTagLocalizationTable"));
        return;
    }

    TArray<FEMRGameplayTagLocalizationEntry*> AllRows;
    DataTable->GetAllRows<FEMRGameplayTagLocalizationEntry>(TEXT("LoadCache"), AllRows);

	UE_LOG(LogTemp, Log, TEXT("[LocalizationSubsystem] Loading %d rows from DT"), AllRows.Num());
	
    for (const FEMRGameplayTagLocalizationEntry* Row : AllRows)
    {
    	if (!Row) continue;
    	UE_LOG(LogTemp, Verbose, TEXT("[LocalizationSubsystem] Row Tag: %s  Key: %s"), *Row->Tag.ToString(), *Row->LocalizationKey);
    	
        if (Row->Tag.IsValid() && !Row->LocalizationKey.IsEmpty())
        {
            CachedTagLocalizations.Add(Row->Tag, Row->LocalizationKey);
        }
    }

	
    if (GameplayTagsStringTableId.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("[LocalizationSubsystem] GameplayTagsStringTableId is not set!"));
    }
    else if (!FStringTableRegistry::Get().FindStringTable(GameplayTagsStringTableId))
    {
        UE_LOG(LogTemp, Error, TEXT("[LocalizationSubsystem] String Table not found: %s"),
            *GameplayTagsStringTableId.ToString());
    }
}


FText UEMRLocalizationSubsystem::GetLocalizedTag(const FGameplayTag& Tag) const
{
    if (!Tag.IsValid())
    {
        return FText::GetEmpty();
    }

    const FGameplayTag ResolvedTag = ResolveLocalizationTag(Tag);
    const FString* LocalizationKey = CachedTagLocalizations.Find(ResolvedTag);

    if (LocalizationKey && !LocalizationKey->IsEmpty())
    {
        return FText::FromStringTable(GameplayTagsStringTableId, *LocalizationKey);
    }
	
    UE_LOG(LogTemp, Warning, TEXT("[LocalizationSubsystem] Tag not found in localization table: %s (resolved from %s)"),
        *ResolvedTag.ToString(),
        *Tag.ToString());

    return BuildMissingLocalizationText(ResolvedTag.ToString());
}


FText UEMRLocalizationSubsystem::GetLocalizedList(const TArray<FGameplayTag>& Tags, bool bUseCommaList) const
{
    if (Tags.Num() == 0)
    {
        return FText::FromStringTable(GameplayTagsStringTableId, TEXT("Fallback_None"));
    }

    TArray<FText> LocalizedTags;
    for (const FGameplayTag& Tag : Tags)
    {
        LocalizedTags.Add(GetLocalizedTag(Tag));
    }

    if (bUseCommaList)
    {
        FText Separator = FText::FromString(TEXT(", "));
        return FText::Join(Separator, LocalizedTags);
    }
    else
    {
        FText Newline = FText::FromString(TEXT("\n"));
        return FText::Join(Newline, LocalizedTags);
    }
}


void UEMRLocalizationSubsystem::ReloadLocalizationCache()
{
    UE_LOG(LogTemp, Log, TEXT("[LocalizationSubsystem] Reloading localization cache..."));
    LoadLocalizationTable();
}

FGameplayTag UEMRLocalizationSubsystem::ResolveLocalizationTag(const FGameplayTag& Tag) const
{
    if (!Tag.IsValid())
    {
        return Tag;
    }

    const FGameplayTag LabAnalyzerRootTag = EMRTags::Abilities::Exam::LabAnalyzer::Root;
    if (LabAnalyzerRootTag.IsValid() &&
        Tag.MatchesTag(LabAnalyzerRootTag))
    {
        return LabAnalyzerRootTag;
    }

    return Tag;
}
