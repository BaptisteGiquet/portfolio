#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "EMRPathologyReferencePaperWidget.generated.h"

class UDataTable;
class UListView;
class UTextBlock;
class UEMRPathologyReferencePaperRowItem;

UENUM(BlueprintType)
enum class EEMRPathologyReferencePaperType : uint8
{
    Exams,
    Treatments
};

USTRUCT(BlueprintType)
struct LOD_API FEMRPathologyReferenceRowView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Reference")
    FText PathologyLabel;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Reference")
    TArray<FText> RequirementLabels;
};

UCLASS(Abstract, BlueprintType)
class LOD_API UEMRPathologyReferencePaperWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EMR|Reference")
    void InitializePaper(EEMRPathologyReferencePaperType InPaperType);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Reference")
    EEMRPathologyReferencePaperType PaperType = EEMRPathologyReferencePaperType::Exams;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Text_Title = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UListView> List_ReferenceRows = nullptr;

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|Reference", meta = (DisplayName = "On Reference Rows Built"))
    void BP_OnReferenceRowsBuilt(const TArray<FEMRPathologyReferenceRowView>& Rows);

private:
    void BuildRowsAndRefresh();
    void RequestSubsystemConfigIfNeeded();
    void HandleSubsystemConfigLoaded();
    bool ResolveMappingTables(UDataTable*& OutExamTable, UDataTable*& OutTreatmentTable) const;
    FText LocalizeTag(const FGameplayTag& Tag) const;

    bool bConfigLoadRequested = false;
    bool bHasBuiltStaticContent = false;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UEMRPathologyReferencePaperRowItem>> CachedRowItems;
};
