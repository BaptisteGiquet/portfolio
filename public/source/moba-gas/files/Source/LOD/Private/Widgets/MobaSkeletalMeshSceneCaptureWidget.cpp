

#include "MobaSkeletalMeshSceneCaptureWidget.h"

#include "Components/SceneCaptureComponent2D.h"
#include "SceneCapture/MobaSkeletalMeshSceneCaptureActor.h"
#include "GameFramework/Character.h"
#include "Interfaces/MobaRenderActorTargetInterface.h"


void UMobaSkeletalMeshSceneCaptureWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ACharacter* PlayerCharacter = Cast<ACharacter>(GetOwningPlayerPawn());
	IMobaRenderActorTargetInterface* PlayerCharacterRenderTargetInterface = Cast<IMobaRenderActorTargetInterface>(PlayerCharacter);
	
	if (!PlayerCharacter || !SkeletalMeshSceneCaptureActor) { return; }

	SkeletalMeshSceneCaptureActor->ConfigureSkeletalMesh(PlayerCharacter->GetMesh()->GetSkeletalMeshAsset(), PlayerCharacter->GetMesh()->GetAnimClass());

	USceneCaptureComponent2D* SceneCapture = SkeletalMeshSceneCaptureActor->GetCaptureComponent();
	if (PlayerCharacterRenderTargetInterface && SceneCapture)
	{
		SceneCapture->SetRelativeLocation(PlayerCharacterRenderTargetInterface->GetCaptureLocalPosition());
		SceneCapture->SetRelativeRotation(PlayerCharacterRenderTargetInterface->GetCaptureLocalRotation());
	}
}


void UMobaSkeletalMeshSceneCaptureWidget::SpawnSceneCaptureActor()
{
	UWorld* World = GetWorld();
	if (!SkeletalMeshSceneCaptureClass || !World) { return; }

	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SkeletalMeshSceneCaptureActor = World->SpawnActor<AMobaSkeletalMeshSceneCaptureActor>(SkeletalMeshSceneCaptureClass, ActorSpawnParams);
}


AMobaSceneCaptureActor* UMobaSkeletalMeshSceneCaptureWidget::GetSceneCaptureActor() const
{
	return SkeletalMeshSceneCaptureActor;
}
