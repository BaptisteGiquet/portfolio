
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "EMRPatientAttributeSet.generated.h"

UCLASS()
class UEMRPatientAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    ATTRIBUTE_ACCESSORS_BASIC(UEMRPatientAttributeSet, HeartRate)
    ATTRIBUTE_ACCESSORS_BASIC(UEMRPatientAttributeSet, BloodPressureSystolic)
    ATTRIBUTE_ACCESSORS_BASIC(UEMRPatientAttributeSet, BloodPressureDiastolic)

    ATTRIBUTE_ACCESSORS_BASIC(UEMRPatientAttributeSet, OxygenSaturation)
    ATTRIBUTE_ACCESSORS_BASIC(UEMRPatientAttributeSet, RespiratoryRate)
    ATTRIBUTE_ACCESSORS_BASIC(UEMRPatientAttributeSet, Temperature)
    ATTRIBUTE_ACCESSORS_BASIC(UEMRPatientAttributeSet, GlasgowScore)

    ATTRIBUTE_ACCESSORS_BASIC(UEMRPatientAttributeSet, Patience)
    ATTRIBUTE_ACCESSORS_BASIC(UEMRPatientAttributeSet, MaxPatience)
    
    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	

private:
    UPROPERTY(ReplicatedUsing = OnRep_HeartRate, BlueprintReadOnly, Category = "EMR|Attributes|VitalSigns", meta=(AllowPrivateAccess = "true"))
    FGameplayAttributeData HeartRate; // beat/min (normal: 60-100)

    UPROPERTY(ReplicatedUsing = OnRep_BloodPressureSystolic, BlueprintReadOnly, Category = "EMR|Attributes|VitalSigns", meta=(AllowPrivateAccess = "true"))
    FGameplayAttributeData BloodPressureSystolic; // mmHg (normal: 120)

    UPROPERTY(ReplicatedUsing = OnRep_BloodPressureDiastolic, BlueprintReadOnly, Category = "EMR|Attributes|VitalSigns", meta=(AllowPrivateAccess = "true"))
    FGameplayAttributeData BloodPressureDiastolic; // mmHg (normal: 80)

    UPROPERTY(ReplicatedUsing = OnRep_OxygenSaturation, BlueprintReadOnly, Category = "EMR|Attributes|VitalSigns", meta=(AllowPrivateAccess = "true"))
    FGameplayAttributeData OxygenSaturation; // % (normal: 95-100)

    UPROPERTY(ReplicatedUsing = OnRep_RespiratoryRate, BlueprintReadOnly, Category = "EMR|Attributes|VitalSigns", meta=(AllowPrivateAccess = "true"))
    FGameplayAttributeData RespiratoryRate; // Respirations/min (normal: 12-20)

    UPROPERTY(ReplicatedUsing = OnRep_Temperature, BlueprintReadOnly, Category = "EMR|Attributes|VitalSigns", meta=(AllowPrivateAccess = "true"))
    FGameplayAttributeData Temperature; // °C (normal: 36.5-37.5)

    UPROPERTY(ReplicatedUsing = OnRep_GlasgowScore, BlueprintReadOnly, Category = "EMR|Attributes|VitalSigns", meta=(AllowPrivateAccess = "true"))
    FGameplayAttributeData GlasgowScore; // (3-15)


    UPROPERTY(ReplicatedUsing = OnRep_Patience, BlueprintReadOnly, Category = "EMR|Attributes|Patience", meta=(AllowPrivateAccess = "true"))
    FGameplayAttributeData Patience;

    UPROPERTY(ReplicatedUsing = OnRep_MaxPatience, BlueprintReadOnly, Category = "EMR|Attributes|Patience", meta=(AllowPrivateAccess = "true"))
    FGameplayAttributeData MaxPatience;


	
    UFUNCTION()
    void OnRep_HeartRate(const FGameplayAttributeData& OldValue);
    
    UFUNCTION()
    void OnRep_BloodPressureSystolic(const FGameplayAttributeData& OldValue);
    
    UFUNCTION()
    void OnRep_BloodPressureDiastolic(const FGameplayAttributeData& OldValue);
    
    UFUNCTION()
    void OnRep_OxygenSaturation(const FGameplayAttributeData& OldValue);
    
    UFUNCTION()
    void OnRep_RespiratoryRate(const FGameplayAttributeData& OldValue);
    
    UFUNCTION()
    void OnRep_Temperature(const FGameplayAttributeData& OldValue);
    
    UFUNCTION()
    void OnRep_GlasgowScore(const FGameplayAttributeData& OldValue);
    
    UFUNCTION()
    void OnRep_Patience(const FGameplayAttributeData& OldValue);
    
    UFUNCTION()
    void OnRep_MaxPatience(const FGameplayAttributeData& OldValue);
};
