

#include "MobaMatchStatsWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Framework/MobaDestroyer.h"
#include "GAS/Abilities/MobaGameplayTypes.h"
#include "Kismet/GameplayStatics.h"


void UMobaMatchStatsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Destroyer = Cast<AMobaDestroyer>(UGameplayStatics::GetActorOfClass(this, AMobaDestroyer::StaticClass()));

	if (Destroyer)
	{
		Destroyer->OnTeamUnitCountUpdated.AddUObject(this, &ThisClass::UpdateTeamDominance);
		Destroyer->OnGoalReached.AddUObject(this, &ThisClass::MatchFinished);
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(UpdateDestroyerProgressTimerHandle, this, &ThisClass::UpdateDestroyerProgress, DestroyerProgressUpdateFrequency, true);
		}
	}
}


void UMobaMatchStatsWidget::UpdateTeamDominance(const int32 TeamOneCount, const int32 TeamTwoCount)
{
	if (!Text_TeamOneCount || !Text_TeamTwoCount) { return; }

	Text_TeamOneCount->SetText(FText::AsNumber(TeamOneCount));
	Text_TeamTwoCount->SetText(FText::AsNumber(TeamTwoCount));
}


void UMobaMatchStatsWidget::MatchFinished(AActor* ViewTarget, EMobaTeam WinningTeam)
{
	if (!ViewTarget || !GetWorld() || !Image_DestroyerProgress) { return; }

	const float DestroyerProgress = (WinningTeam == EMobaTeam::TeamOne) ?  0.f : 1.f;
	GetWorld()->GetTimerManager().ClearTimer(UpdateDestroyerProgressTimerHandle);
	Image_DestroyerProgress->GetDynamicMaterial()->SetScalarParameterValue(DestroyerProgressDynamicMaterialParamName, DestroyerProgress);
}


void UMobaMatchStatsWidget::UpdateDestroyerProgress()
{
	if (Destroyer && Image_DestroyerProgress)
	{
		const float DestroyerProgress = Destroyer->GetDestroyerProgress();
		Image_DestroyerProgress->GetDynamicMaterial()->SetScalarParameterValue(DestroyerProgressDynamicMaterialParamName, DestroyerProgress);
	}
}
