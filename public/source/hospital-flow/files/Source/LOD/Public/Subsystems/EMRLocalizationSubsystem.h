#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "EMRLocalizationSubsystem.generated.h"

class UDataTable;

/**
 * Susbsystem to localize gameplaytags
 * - Cache Tag → Localization Key
 */

UCLASS(Config = Game)
class LOD_API UEMRLocalizationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	
	// Return the tag as a Localized text
	UFUNCTION(BlueprintPure, Category = "EMR|Localization")
	FText GetLocalizedTag(const FGameplayTag& Tag) const;

	
	UFUNCTION(BlueprintPure, Category = "EMR|Localization")
	FText GetLocalizedList(const TArray<FGameplayTag>& Tags, bool bUseCommaList = true) const;

	
	UFUNCTION(BlueprintCallable, Category = "EMR|Localization")
	void ReloadLocalizationCache();

private:
	void LoadLocalizationTable();
	FGameplayTag ResolveLocalizationTag(const FGameplayTag& Tag) const;

	/** Cache Tag → Localization Key */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FString> CachedTagLocalizations;


	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> GameplayTagLocalizationTable;
	
	UPROPERTY(Config)
	FName GameplayTagsStringTableId;
};
