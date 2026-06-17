
#pragma once

#include "CoreMinimal.h"
#include "ItemsTypes.h"
#include "GameFramework/Actor.h"
#include "Item.generated.h"


class UNiagaraSystem;
class UNiagaraComponent;
class UBoxComponent;
class USphereComponent;
enum class EItemState : uint8;


UCLASS()
class SLASH_API AItem : public AActor
{
	GENERATED_BODY()
	
public:	
	AItem();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Floating")
	float GetRunningTime() const;

	UFUNCTION(BlueprintPure, Category = "Floating")
	bool GetEnableFloating() const;

	UFUNCTION(BlueprintCallable, Category = "Floating")
	void SetEnableFloating(bool InEnableFloat);
	
	UFUNCTION(BlueprintPure, Category = "Floating")
	float GetAmplitude() const;
	
	UFUNCTION(BlueprintCallable, Category = "Floating")
	void SetAmplitude(float InAmplitude);
	
	UFUNCTION(BlueprintPure, Category = "Floating")
	float GetPeriod() const;
	
	UFUNCTION(BlueprintCallable, Category = "Floating")
	void SetPeriod(float InPeriod);
	
	UFUNCTION(BlueprintPure, Category = "Floating")
	float TransformedSin();

	UFUNCTION(BlueprintPure, Category = "Floating")
	float TransformedCos();

	//Callback function used called by delegate
	UFUNCTION()
	virtual void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	virtual void OnHitBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	virtual void PlayPickupVisualEffect();

	UFUNCTION()
	virtual void PlayPickupSoundEffect();
	
	template<typename T>
	T Avg(T First, T Second);
	
	UPROPERTY(EditAnywhere, Category = "Visual Effects")
	TObjectPtr<UNiagaraComponent> HoveringVisualEffect;
	
	
private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> PickupVisualEffect;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> PickupSoundEffect;
	
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetItemMesh)
	UStaticMeshComponent* ItemMesh;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetOverlapSphereDetection)
	USphereComponent* OverlapSphereDetection;

	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetHitBox)
	UBoxComponent* HitBox;

	UPROPERTY(VisibleInstanceOnly)
	EItemState ItemState = EItemState::EIS_Hovering;
	
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetRunningTime)
	float RunningTime;

	UPROPERTY(EditAnywhere, Category = "Floating", BlueprintGetter = GetEnableFloating, BlueprintSetter = SetEnableFloating)
	bool bEnableFloating = true;
	
	UPROPERTY(EditAnywhere, Category = "Floating", BlueprintGetter = GetAmplitude, BlueprintSetter = SetAmplitude)
	float Amplitude = 0.25f;
	
	UPROPERTY(EditAnywhere, Category = "Floating", BlueprintGetter = GetPeriod, BlueprintSetter = SetPeriod)
	float Period = 5.f;


public:

	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE UStaticMeshComponent* GetItemMesh() const { return ItemMesh; }
	
	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE USphereComponent* GetOverlapSphereDetection() const { return OverlapSphereDetection; }
	
	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE UBoxComponent* GetHitBox() const { return HitBox; }
	
	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE EItemState GetItemState() const { return ItemState; }
	
	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE void SetItemState(EItemState InItemState) { ItemState = InItemState; }
};


template <typename T>
T AItem::Avg(T First, T Second)
{
	return (First + Second) / 2;
}
