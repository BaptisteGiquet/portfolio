
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MobaMatchStatsWidget.generated.h"


enum class EMobaTeam : uint8;
class AMobaDestroyer;
class UTextBlock;
class UImage;

UCLASS()
class LOD_API UMobaMatchStatsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

private:
	void UpdateTeamDominance(const int32 TeamOneCount, const int32 TeamTwoCount);
	void MatchFinished(AActor* ViewTarget, EMobaTeam WinningTeam);
	void UpdateDestroyerProgress();

	UPROPERTY(EditDefaultsOnly, Category = "MatchStats")
	float DestroyerProgressUpdateFrequency = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "MatchStats")
	FName DestroyerProgressDynamicMaterialParamName = FName(TEXT("DestroyerProgress"));
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_DestroyerProgress = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_TeamOneCount = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_TeamTwoCount = nullptr;

	UPROPERTY()
	TObjectPtr<AMobaDestroyer> Destroyer = nullptr;

	FTimerHandle UpdateDestroyerProgressTimerHandle;
};
