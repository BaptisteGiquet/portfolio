#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace EMRTags
{
    // ------------------------------------------------------------------------
    // Components
    // ------------------------------------------------------------------------
    namespace Component
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(TreatmentAttachment);
    }

    // ------------------------------------------------------------------------
    // Seat Animation
    // ------------------------------------------------------------------------
    namespace SeatAnimation
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hub);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reception);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(XRay);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(PatientWaitingRoom);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(PatientExamWaiting);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(PatientTreatmentWaiting);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(PatientToilet);
    }

	// ------------------------------------------------------------------------
	// GameplayMessageSubsystem
	// ------------------------------------------------------------------------
	namespace GMS
	{
		namespace AI
		{
			namespace Navigation
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Root);
				
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToReception);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToBathroom);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToSnackMachine);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToMagicBox);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToXRay);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToElectrocardiogram);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToCTScan);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToUltrasound);
                UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToOximeter);
                UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToLabAnalyzer);
                UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToBed);

                UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToWaitingSeat);

                UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToExit);
			}
        }
    }

	// ------------------------------------------------------------------------
	// SetByCaller
	// ------------------------------------------------------------------------
	namespace SetByCaller
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpendMoney);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GrantMoney);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(RemoveReputation);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ReputationDrain);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GrantReputation);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PatienceDrain);
	}

	
	// ------------------------------------------------------------------------
	// Events
	// ------------------------------------------------------------------------
	namespace Event
	{
		namespace Item
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pick);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Place);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Use);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseToiletCleaner);
		}
		namespace LabAnalyzer
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseTube);
		}
		namespace Intravenous
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseBag);
		}
	}

	// ------------------------------------------------------------------------
	// Attributes
	// ------------------------------------------------------------------------
	namespace Attributes
	{
		namespace Patient
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HeartRate);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(BloodPressureSystolic);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(BloodPressureDiastolic);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(OxygenSaturation);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(RespiratoryRate);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Temperature);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(GlasgowScore);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Patience);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxPatience);
		}

		namespace Team
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Money);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reputation);
		}
	}
	
	// ------------------------------------------------------------------------
	// Abilities
	// ------------------------------------------------------------------------
    namespace Abilities
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven)

        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interact);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(CallWaitingPatient);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(SelectNightShift);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(StopNightShift);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(UseCoffeePitcher);

        UE_DECLARE_GAMEPLAY_TAG_EXTERN(MoveToSeat);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(TriageSeat);
	

		namespace Exam
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Triage);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(None);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(CTScan);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(XRay);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ultrasound);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Electrocardiogram);

			namespace LabAnalyzer
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Root);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(CBC);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Glucose);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Electrolytes);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lactate);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(CRP);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ddimer);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Troponin);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(UrineDipstick);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Toxicology);
			}
		}

        namespace Treatment
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Root);
		
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bandage);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Suture);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cast);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Splint);
		
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Oxygen);

            UE_DECLARE_GAMEPLAY_TAG_EXTERN(IntravenousMedication);
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(OralMedication);
        }
    }
	

    // ------------------------------------------------------------------------
    // GameMode
    // ------------------------------------------------------------------------
    namespace GameMode
    {
        namespace Hub
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(CanSelectNightShift);
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(PendingNightShift);
        }

        namespace NightShift
        {
        	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Basic);
        	UE_DECLARE_GAMEPLAY_TAG_EXTERN(FootballMatch);
        	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Riots);
        	
	        namespace Modifier
	        {
	        	UE_DECLARE_GAMEPLAY_TAG_EXTERN(BusAccident);
	        }
        }
    }

    namespace RunUpgrade
    {
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Root);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(CoffeeMachine);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(OxygenMachine);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(SnackMachine);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicBox);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemDispenserSales);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(TreatmentBed);
    }

	// ------------------------------------------------------------------------
	// Items
	// ------------------------------------------------------------------------
	namespace Items
	{
		namespace Type
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PhysicalCare);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Intravenous);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Oral);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LifeSupport);
		}
	}
	
	// ------------------------------------------------------------------------
	// Patient
	// ------------------------------------------------------------------------
    namespace Patient
    {
		namespace Type
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Normal);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Impatient);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Drunk);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Prisoner);
		}

		namespace BloodType
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(APositive);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ANegative);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(BPositive);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(BNegative);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ABPositive);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ABNegative);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(OPositive);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ONegative);
		}
	
		namespace Phase
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Root);
		
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitingInReception);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnderTriage);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitingExam);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnderExam);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitingTreatment);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnderTreatment);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitingToPay);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Paying);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Paid);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Leaving);
		}
		
        namespace Pathology
        {
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Root);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(AnkleSprain);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dislocation);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SimpleFracture);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(IschemicStroke);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Concussion);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PulmonaryEmbolism);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(RenalColic);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pneumonia);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pneumothorax);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeftArmFracture);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(RightArmFracture);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeftLegFracture);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(RightLegFracture);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeftKneeDislocation);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(RightKneeDislocation);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeftShoulderDislocation);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(RightShoulderDislocation);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeftAnkleSprain);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(RightAnkleSprain);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Appendicitis);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cholecystitis);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(CoronarySyndrome);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(AcuteAsthma);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sepsis);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pyelonephritis);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cystitis);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(DiabeticKetoacidosis);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkinLaceration);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hernia);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Migraine);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Anaphylaxis);
		}

		namespace Symptom
		{
	        UE_DECLARE_GAMEPLAY_TAG_EXTERN(FaceDroop);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(LimbWeakness);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(SpeechDifficulty);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Headache);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dizziness);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Nausea);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(ShortnessOfBreath);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(PleuriticChestPain);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tachycardia);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(FlankPain);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(GroinRadiation);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hematuria);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fever);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(ProductiveCough);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnilateralChestPain);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(DryCough);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pain);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Swelling);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Deformity);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(JointDeformity);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(SeverePain);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(LimitedMotion);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(PainOnWeightBearing);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Instability);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(RLQPain);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(PainMigration);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(RUQPain);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(MurphySign);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(ChestPressure);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(RadiationArmJawBack);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Wheeze);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(ChestTightness);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tachypnea);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dysuria);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frequency);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Urgency);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Polyuria);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Polydipsia);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(KussmaulBreathing);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bleeding);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(OpenWound);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(GroinBulge);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(DiscomfortWithLifting);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(ReducibleMass);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnilateralThrobbingHeadache);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Photophobia);
		    UE_DECLARE_GAMEPLAY_TAG_EXTERN(LipTongueThroatSwelling);
	        UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hives);
		}
		
        namespace ExamCompleted
        {
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Root);
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(CTScan);
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(XRay);
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ultrasound);
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(Electrocardiogram);
            UE_DECLARE_GAMEPLAY_TAG_EXTERN(LabAnalyzer);

			namespace LabAnalyzerTests
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(CBC);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Electrolytes);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lactate);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(CRP);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ddimer);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Toxicology);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Troponin);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Glucose);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(UrineDipstick);
			}
        }

		namespace Severity
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Green); 
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Yellow);  
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Red); 
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Black);   
		}

		namespace ExamQueue
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Root);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitingForCTScan);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitingForXRay);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitingForUltrasound);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitingForElectrocardiogram);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitingForLabAnalyzer);
		}

		namespace TreatmentQueue
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Root);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitForBandageTreatment);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitForSutureTreatment);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitForCastTreatment);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitForSplintTreatment);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitForOxygenTreatment);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitForIntravenousMedicationTreatment);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WaitForOralMedicationTreatment);
		}
	}
	
	// ------------------------------------------------------------------------
	// Stations / Rooms
	// ------------------------------------------------------------------------
	namespace Station
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reception);   
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Exam);     
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Treatment);   
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bathroom);     
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Storage);      
	}
	
	// ------------------------------------------------------------------------
	// Machines
	// ------------------------------------------------------------------------
	namespace Machine
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Any);

		UE_DECLARE_GAMEPLAY_TAG_EXTERN(PatientCaller);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reception);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CTScan);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(XRay);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ultrasound);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Electrocardiogram);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(LabAnalyzer);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IntravenousMedication);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Oxygen);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ItemDispenser);
        UE_DECLARE_GAMEPLAY_TAG_EXTERN(CoffeeMachine);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SnackMachine);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicBox);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(OvertimeStopTerminal);
	}
	
	// ------------------------------------------------------------------------
	// Treatment Results
	// ------------------------------------------------------------------------
	namespace Treatment
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Any);
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bandage);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Suture);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cast);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Splint);
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Oxygen);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(IntravenousMedication);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(OralMedication);
	}


	namespace UI
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Validate);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Close);

		namespace WidgetStack
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Modal);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameMenu);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameHud);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend);
		}

		namespace Widgets
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PressAnyKeyScreen);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MainMenu);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PlayScreen);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(OptionsScreen);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(CreditsScreen);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LobbyScreen);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ConfirmScreen);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(KeyRemapScreen);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(TriageScreen);
		}

		namespace OptionsImages
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(TestImage);
		}
	}
}
