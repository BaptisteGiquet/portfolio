#include "UI/Reference/EMRPathologyReferencePaperWidget.h"

#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Data/EMRExamRequirementsForPathologyMapping.h"
#include "Data/EMRTreatmentForPathologyMapping.h"
#include "Engine/DataTable.h"
#include "Framework/EMRAssetManager.h"
#include "GAS/EMRTags.h"
#include "Subsystems/EMRLocalizationSubsystem.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "UI/Reference/EMRPathologyReferencePaperRowItem.h"

namespace
{
    FText BuildMissingLocalizationText(const FString& MissingIdentifier)
    {
        return FText::Format(
            NSLOCTEXT("EMRPathologyReferencePaper", "MissingLocalizationTag", "[MISSING_LOCALIZATION: {0}]"),
            FText::FromString(MissingIdentifier)
        );
    }

    void SortTagsByName(TArray<FGameplayTag>& Tags)
    {
        Tags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
        {
            return A.ToString() < B.ToString();
        });
    }

    void SortNamesByLexicalOrder(TArray<FName>& Names)
    {
        Names.Sort([](const FName& A, const FName& B)
        {
            return A.ToString() < B.ToString();
        });
    }

    void AddUniqueLocalizedLabels(
        const TArray<FGameplayTag>& RequirementTags,
        TArray<FText>& OutLabels,
        const TFunctionRef<FText(const FGameplayTag&)>& LocalizeTagFn)
    {
        TSet<FString> SeenLabelStrings;
        for (const FGameplayTag& RequirementTag : RequirementTags)
        {
            const FText LocalizedLabel = LocalizeTagFn(RequirementTag);
            const FString LabelString = LocalizedLabel.ToString();
            if (LabelString.IsEmpty() || SeenLabelStrings.Contains(LabelString))
            {
                continue;
            }

            SeenLabelStrings.Add(LabelString);
            OutLabels.Add(LocalizedLabel);
        }
    }

    FName ResolveReferencePathologyName(const FGameplayTag& PathologyTag)
    {
        if (!PathologyTag.IsValid())
        {
            return NAME_None;
        }

        const FString PathologyTagString = PathologyTag.ToString();
        if (PathologyTagString.EndsWith(TEXT("AnkleSprain")))
        {
            return FName(TEXT("EMR.Patient.Pathology.AnkleSprain"));
        }

        if (PathologyTagString.EndsWith(TEXT("Dislocation")))
        {
            return FName(TEXT("EMR.Patient.Pathology.Dislocation"));
        }

        if (PathologyTagString.EndsWith(TEXT("Fracture")))
        {
            return FName(TEXT("EMR.Patient.Pathology.SimpleFracture"));
        }

        return FName(*PathologyTagString);
    }

    FText LocalizeCanonicalPathologyName(
        const FName CanonicalPathologyName,
        const FGameplayTag& SourcePathologyTag,
        const TFunctionRef<FText(const FGameplayTag&)>& LocalizeTagFn)
    {
        const FGameplayTag CanonicalPathologyTag = FGameplayTag::RequestGameplayTag(CanonicalPathologyName, false);
        if (CanonicalPathologyTag.IsValid())
        {
            return LocalizeTagFn(CanonicalPathologyTag);
        }

        if (SourcePathologyTag.IsValid())
        {
            return BuildMissingLocalizationText(CanonicalPathologyName.ToString());
        }

        return BuildMissingLocalizationText(TEXT("<INVALID_PATHOLOGY_TAG>"));
    }
}

void UEMRPathologyReferencePaperWidget::InitializePaper(EEMRPathologyReferencePaperType InPaperType)
{
    if (PaperType != InPaperType)
    {
        PaperType = InPaperType;
        bHasBuiltStaticContent = false;
    }

    BuildRowsAndRefresh();
}

void UEMRPathologyReferencePaperWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildRowsAndRefresh();
}

void UEMRPathologyReferencePaperWidget::BuildRowsAndRefresh()
{
    if (bHasBuiltStaticContent)
    {
        return;
    }

    if (Text_Title)
    {
        Text_Title->SetText(PaperType == EEMRPathologyReferencePaperType::Exams
            ? NSLOCTEXT("EMRPathologyReferencePaper", "TitleExams", "Pathologies and Required Exams")
            : NSLOCTEXT("EMRPathologyReferencePaper", "TitleTreatments", "Pathologies and Required Treatments"));
    }

    UDataTable* ExamTable = nullptr;
    UDataTable* TreatmentTable = nullptr;
    if (!ResolveMappingTables(ExamTable, TreatmentTable))
    {
        RequestSubsystemConfigIfNeeded();
        CachedRowItems.Reset();
        if (List_ReferenceRows)
        {
            List_ReferenceRows->ClearListItems();
        }
        return;
    }

    UDataTable* SelectedTable = PaperType == EEMRPathologyReferencePaperType::Exams ? ExamTable : TreatmentTable;
    if (!SelectedTable)
    {
        CachedRowItems.Reset();
        if (List_ReferenceRows)
        {
            List_ReferenceRows->ClearListItems();
        }
        bHasBuiltStaticContent = true;
        return;
    }

    TArray<FEMRPathologyReferenceRowView> RowViews;

    if (PaperType == EEMRPathologyReferencePaperType::Exams)
    {
        TArray<FEMRExamRequirementsForPathologyMapping*> Rows;
        SelectedTable->GetAllRows(TEXT("PathologyReferencePaper"), Rows);
        TMap<FName, TSet<FGameplayTag>> RequirementsByPathologyName;
        TMap<FName, FGameplayTag> RepresentativePathologyByName;

        for (const FEMRExamRequirementsForPathologyMapping* Row : Rows)
        {
            if (!Row || !Row->Pathology.IsValid())
            {
                continue;
            }

            const FName CanonicalPathologyName = ResolveReferencePathologyName(Row->Pathology);
            if (CanonicalPathologyName.IsNone())
            {
                continue;
            }

            RepresentativePathologyByName.FindOrAdd(CanonicalPathologyName, Row->Pathology);

            TSet<FGameplayTag>& RequirementSet = RequirementsByPathologyName.FindOrAdd(CanonicalPathologyName);
            TArray<FGameplayTag> RequirementTags;
            Row->RequiredExams.GetGameplayTagArray(RequirementTags);
            for (const FGameplayTag& RequirementTag : RequirementTags)
            {
                if (RequirementTag.IsValid())
                {
                    RequirementSet.Add(RequirementTag);
                }
            }
        }

        TArray<FName> PathologyNames;
        RequirementsByPathologyName.GetKeys(PathologyNames);
        SortNamesByLexicalOrder(PathologyNames);

        for (const FName& PathologyName : PathologyNames)
        {
            FEMRPathologyReferenceRowView RowView;
            RowView.PathologyLabel = LocalizeCanonicalPathologyName(
                PathologyName,
                RepresentativePathologyByName.FindRef(PathologyName),
                [this](const FGameplayTag& PathologyTag) { return LocalizeTag(PathologyTag); });

            if (const TSet<FGameplayTag>* RequirementSet = RequirementsByPathologyName.Find(PathologyName))
            {
                TArray<FGameplayTag> RequirementTags = RequirementSet->Array();
                SortTagsByName(RequirementTags);
                AddUniqueLocalizedLabels(
                    RequirementTags,
                    RowView.RequirementLabels,
                    [this](const FGameplayTag& RequirementTag) { return LocalizeTag(RequirementTag); });
            }

            RowViews.Add(MoveTemp(RowView));
        }
    }
    else
    {
        TArray<FEMRTreatmentForPathologyMapping*> Rows;
        SelectedTable->GetAllRows(TEXT("PathologyReferencePaper"), Rows);
        TMap<FName, TSet<FGameplayTag>> RequirementsByPathologyName;
        TMap<FName, FGameplayTag> RepresentativePathologyByName;

        for (const FEMRTreatmentForPathologyMapping* Row : Rows)
        {
            if (!Row || !Row->Pathology.IsValid())
            {
                continue;
            }

            const FName CanonicalPathologyName = ResolveReferencePathologyName(Row->Pathology);
            if (CanonicalPathologyName.IsNone())
            {
                continue;
            }

            RepresentativePathologyByName.FindOrAdd(CanonicalPathologyName, Row->Pathology);

            TSet<FGameplayTag>& RequirementSet = RequirementsByPathologyName.FindOrAdd(CanonicalPathologyName);
            TArray<FGameplayTag> RequirementTags;
            Row->Treatments.GetGameplayTagArray(RequirementTags);
            for (const FGameplayTag& RequirementTag : RequirementTags)
            {
                if (RequirementTag.IsValid())
                {
                    RequirementSet.Add(RequirementTag);
                }
            }
        }

        TArray<FName> PathologyNames;
        RequirementsByPathologyName.GetKeys(PathologyNames);
        SortNamesByLexicalOrder(PathologyNames);

        for (const FName& PathologyName : PathologyNames)
        {
            FEMRPathologyReferenceRowView RowView;
            RowView.PathologyLabel = LocalizeCanonicalPathologyName(
                PathologyName,
                RepresentativePathologyByName.FindRef(PathologyName),
                [this](const FGameplayTag& PathologyTag) { return LocalizeTag(PathologyTag); });

            if (const TSet<FGameplayTag>* RequirementSet = RequirementsByPathologyName.Find(PathologyName))
            {
                TArray<FGameplayTag> RequirementTags = RequirementSet->Array();
                SortTagsByName(RequirementTags);
                AddUniqueLocalizedLabels(
                    RequirementTags,
                    RowView.RequirementLabels,
                    [this](const FGameplayTag& RequirementTag) { return LocalizeTag(RequirementTag); });
            }

            RowViews.Add(MoveTemp(RowView));
        }
    }

    CachedRowItems.Reset();
    CachedRowItems.Reserve(RowViews.Num());

    for (const FEMRPathologyReferenceRowView& RowView : RowViews)
    {
        UEMRPathologyReferencePaperRowItem* RowItem = NewObject<UEMRPathologyReferencePaperRowItem>(this);
        if (!RowItem)
        {
            continue;
        }

        RowItem->Initialize(RowView.PathologyLabel, RowView.RequirementLabels);
        CachedRowItems.Add(RowItem);
    }

    if (List_ReferenceRows)
    {
        List_ReferenceRows->ClearListItems();
        for (UEMRPathologyReferencePaperRowItem* RowItem : CachedRowItems)
        {
            if (RowItem)
            {
                List_ReferenceRows->AddItem(RowItem);
            }
        }
    }

    BP_OnReferenceRowsBuilt(RowViews);

    bConfigLoadRequested = false;
    bHasBuiltStaticContent = true;
}

void UEMRPathologyReferencePaperWidget::RequestSubsystemConfigIfNeeded()
{
    if (bConfigLoadRequested)
    {
        return;
    }

    bConfigLoadRequested = true;
    UEMRAssetManager::Get().LoadSubsystemConfig(FStreamableDelegate::CreateUObject(this, &ThisClass::HandleSubsystemConfigLoaded));
}

void UEMRPathologyReferencePaperWidget::HandleSubsystemConfigLoaded()
{
    bConfigLoadRequested = false;
    BuildRowsAndRefresh();
}

bool UEMRPathologyReferencePaperWidget::ResolveMappingTables(UDataTable*& OutExamTable, UDataTable*& OutTreatmentTable) const
{
    OutExamTable = nullptr;
    OutTreatmentTable = nullptr;

    TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
    if (!UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig) || LoadedSubsystemConfig.Num() <= 0)
    {
        return false;
    }

    const UEMRSubsystemConfig* SubsystemConfig = LoadedSubsystemConfig[0];
    if (!SubsystemConfig)
    {
        return false;
    }

    OutExamTable = SubsystemConfig->GetExamRequirementsFromPathologyTable();
    OutTreatmentTable = SubsystemConfig->GetTreatmentsFromPathologyMappingTable();
    return true;
}

FText UEMRPathologyReferencePaperWidget::LocalizeTag(const FGameplayTag& Tag) const
{
    if (!Tag.IsValid())
    {
        return FText::GetEmpty();
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UEMRLocalizationSubsystem* LocalizationSubsystem = GameInstance->GetSubsystem<UEMRLocalizationSubsystem>())
        {
            return LocalizationSubsystem->GetLocalizedTag(Tag);
        }
    }

    return FText::Format(
        NSLOCTEXT("EMRPathologyReferencePaper", "MissingLocalizationSubsystemTag", "[MISSING_LOCALIZATION_SUBSYSTEM: {0}]"),
        FText::FromString(Tag.ToString())
    );
}
