
#include "GAS/EMRTags.h"

namespace EMRTags
{

    // ------------------------------------------------------------------------
    // Components
    // ------------------------------------------------------------------------
    namespace Component
    {
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(TreatmentAttachment, "EMR.Component.TreatmentAttachment", "Tag added to dynamic components attached to patients so they can be cleaned up when pooled");
    }

    // ------------------------------------------------------------------------
    // Seat Animation
    // ------------------------------------------------------------------------
    namespace SeatAnimation
    {
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hub, "EMR.SeatAnimation.Hub", "Seat animation for hub seating");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reception, "EMR.SeatAnimation.Reception", "Seat animation for reception seating");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(XRay, "EMR.SeatAnimation.XRay", "Seat animation for XRay seating");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(PatientWaitingRoom, "EMR.SeatAnimation.PatientWaitingRoom", "Patient seated loop for waiting room seats");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(PatientExamWaiting, "EMR.SeatAnimation.PatientExamWaiting", "Patient seated loop for exam waiting seats");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(PatientTreatmentWaiting, "EMR.SeatAnimation.PatientTreatmentWaiting", "Patient seated loop for treatment waiting seats");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(PatientToilet, "EMR.SeatAnimation.PatientToilet", "Patient seated loop while using a toilet");
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
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "EMR.GMS.AI.Navigation", "GMS tag to inform AI to move");

				UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToReception, "EMR.GMS.AI.Navigation.MoveToReception", "GMS tag to inform AI to move to Reception");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToBathroom, "EMR.GMS.AI.Navigation.MoveToBathroom", "GMS tag to inform AI to move to Bathroom");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToSnackMachine, "EMR.GMS.AI.Navigation.MoveToSnackMachine", "GMS tag to inform AI to move to SnackMachine");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToMagicBox, "EMR.GMS.AI.Navigation.MoveToMagicBox", "GMS tag to inform AI to move to MagicBox");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToXRay, "EMR.GMS.AI.Navigation.MoveToXRay", "GMS tag to inform AI to move to XRay");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToElectrocardiogram, "EMR.GMS.AI.Navigation.MoveToElectrocardiogram", "GMS tag to inform AI to move to Electrocardiogram");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToCTScan, "EMR.GMS.AI.Navigation.MoveToCTScan", "GMS tag to inform AI to move to CTScan");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToUltrasound, "EMR.GMS.AI.Navigation.MoveToUltrasound", "GMS tag to inform AI to move to Ultrasound");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToLabAnalyzer, "EMR.GMS.AI.Navigation.MoveToLabAnalyzer", "GMS tag to inform AI to move to LabAnalyzer");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToBed, "EMR.GMS.AI.Navigation.MoveToBed", "GMS tag to inform AI to move to Bed");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToWaitingSeat, "EMR.GMS.AI.Navigation.MoveToWaitingSeat", "GMS tag to inform AI to move to a waiting seat");
                UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToExit, "EMR.GMS.AI.Navigation.MoveToExit", "GMS tag to inform AI to move to Exit");
            }
        }
    }

	// ------------------------------------------------------------------------
	// SetByCaller
	// ------------------------------------------------------------------------
	namespace SetByCaller
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(SpendMoney, "EMR.SetByCaller.SpendMoney", "SetByCallerTag for GE_Spend_Money");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(GrantMoney, "EMR.SetByCaller.GrantMoney", "SetByCallerTag for GE_Grant_Money");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(RemoveReputation, "EMR.SetByCaller.RemoveReputation", "SetByCallerTag for GE_Remove_Reputation");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReputationDrain, "EMR.SetByCaller.ReputationDrain", "SetByCallerTag for GE_ReputationDrain");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(GrantReputation, "EMR.SetByCaller.GrantReputation", "SetByCallerTag for GE_Grant_Reputation");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(PatienceDrain, "EMR.SetByCaller.PatienceDrain", "SetByCallerTag for GE_PatienceDrain");
	}

	// ------------------------------------------------------------------------
	// Events
	// ------------------------------------------------------------------------
	namespace Event
	{
		namespace Item
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pick, "EMR.Event.Item.Pick", "Request to Pick a carried clinic item");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Place, "EMR.Event.Item.Place", "Request to place a carried clinic item");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Use, "EMR.Event.Item.Use", "Request to Use a carried clinic item");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(UseToiletCleaner, "EMR.Event.Item.UseToiletCleaner", "Request to use a toilet cleaner item");
		}
		namespace LabAnalyzer
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(UseTube, "EMR.Event.LabAnalyzer.UseTube", "Request to use a LabAnalyzer tube");
		}
		namespace Intravenous
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(UseBag, "EMR.Event.Intravenous.UseBag", "Request to use an IV bag on the IV stand");
		}
	}

	// ------------------------------------------------------------------------
	// Attributes
	// ------------------------------------------------------------------------
	namespace Attributes
	{
		namespace Patient
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HeartRate, "EMR.Attributes.Patient.HeartRate", "Patient heart rate (bpm)");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(BloodPressureSystolic, "EMR.Attributes.Patient.BloodPressureSystolic", "Patient systolic blood pressure (mmHg)");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(BloodPressureDiastolic, "EMR.Attributes.Patient.BloodPressureDiastolic", "Patient diastolic blood pressure (mmHg)");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(OxygenSaturation, "EMR.Attributes.Patient.OxygenSaturation", "Patient oxygen saturation (%)");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(RespiratoryRate, "EMR.Attributes.Patient.RespiratoryRate", "Patient respiratory rate (breaths/min)");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Temperature, "EMR.Attributes.Patient.Temperature", "Patient body temperature (°C)");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(GlasgowScore, "EMR.Attributes.Patient.GlasgowScore", "Patient Glasgow coma score (3-15)");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Patience, "EMR.Attributes.Patient.Patience", "Current patience level");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(MaxPatience, "EMR.Attributes.Patient.MaxPatience", "Maximum patience level");
		}

		namespace Team
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Money, "EMR.Attributes.Team.Money", "Team Money");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reputation, "EMR.Attributes.Team.Reputation", "Team Reputation");
		}
	}

	// ------------------------------------------------------------------------
	// Abilities
	// ------------------------------------------------------------------------
	namespace Abilities
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven, "EMR.Abilities.ActivateOnGiven", "For abilities that should activate on given");

		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interact, "EMR.Abilities.Interact", "Interact with interactable actor");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CallWaitingPatient, "EMR.Abilities.CallWaitingPatient", "Server gameplay event used to call a seated patient toward reception");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(SelectNightShift, "EMR.Abilities.SelectNightShift", "Client gameplay event to pick a night shift from the hub");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(StopNightShift, "EMR.Abilities.StopNightShift", "Request to end the current NightShift once overtime is active");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(UseCoffeePitcher, "EMR.Abilities.UseCoffeePitcher", "Use coffee pitcher on coffee machine or patient");

		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveToSeat, "EMR.Abilities.MoveToSeat", "Trigger to move a patient toward an assigned waiting seat");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(TriageSeat, "EMR.Abilities.TriageSeat", "Sit on a triage seat and focus the triage UI");

		namespace Exam
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Triage, "EMR.Abilities.Exam.Triage", "Perform Triage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(None, "EMR.Abilities.Exam.None", "No exam required / direct treatment");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(CTScan, "EMR.Abilities.Exam.CTScan", "Perform CT scan");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(XRay, "EMR.Abilities.Exam.XRay", "Perform X-ray");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ultrasound, "EMR.Abilities.Exam.Ultrasound", "Perform ultrasound");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Electrocardiogram, "EMR.Abilities.Exam.Electrocardiogram", "Perform electrocardiogram");

			namespace LabAnalyzer
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "EMR.Abilities.Exam.LabAnalyzer", "Perform blood and urine test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(CBC, "EMR.Abilities.Exam.LabAnalyzer.CBC", "Perform CBC test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Electrolytes, "EMR.Abilities.Exam.LabAnalyzer.Electrolytes", "Perform Electrolytes test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Lactate, "EMR.Abilities.Exam.LabAnalyzer.Lactate", "Perform Lactate test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(CRP, "EMR.Abilities.Exam.LabAnalyzer.CRP", "Perform CRP test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ddimer, "EMR.Abilities.Exam.LabAnalyzer.Ddimer", "Perform Ddimer test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Toxicology, "EMR.Abilities.Exam.LabAnalyzer.Toxicology", "Perform Toxicology test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Troponin, "EMR.Abilities.Exam.LabAnalyzer.Troponin", "Test troponin levels");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Glucose, "EMR.Abilities.Exam.LabAnalyzer.Glucose", "Test glucose levels");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(UrineDipstick, "EMR.Abilities.Exam.LabAnalyzer.UrineDipstick", "Perform urine dipstick test");
			}
		}

		namespace Treatment
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "EMR.Abilities.Treatment", "Apply Treatment");

			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Observation, "EMR.Abilities.Treatment.Observation", "Place under observation");

			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Bandage, "EMR.Abilities.Treatment.Bandage", "Apply bandage");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Suture, "EMR.Abilities.Treatment.Suture", "Perform suture");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cast, "EMR.Abilities.Treatment.Cast", "Apply cast");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Splint, "EMR.Abilities.Treatment.Splint", "Apply splint");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hydration, "EMR.Abilities.Treatment.Hydration", "Administer hydration");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Oxygen, "EMR.Abilities.Treatment.Oxygen", "Administer oxygen");

			UE_DEFINE_GAMEPLAY_TAG_COMMENT(IntravenousMedication, "EMR.Abilities.Treatment.IntravenousMedication", "Place under IntravenousMedication");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(OralMedication, "EMR.Abilities.Treatment.OralMedication", "Place under OralMedication");
		}
	}

    // ------------------------------------------------------------------------
    // State
    // ------------------------------------------------------------------------
    namespace GameMode
    {
        namespace Hub
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(CanSelectNightShift, "EMR.GameMode.Hub.CanSelectNightShift", "Player is allowed to trigger NightShift selection from hub");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(PendingNightShift, "EMR.GameMode.Hub.PendingNightShift", "A NightShift has been validated and is pending travel");
        }

        namespace NightShift
        {
        	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Basic, "EMR.GameMode.NightShift.Basic", "The Basic NightShift.");
        	UE_DEFINE_GAMEPLAY_TAG_COMMENT(FootballMatch, "EMR.GameMode.NightShift.FootballMatch", "The FootballMatch NightShift.");
        	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Riots, "EMR.GameMode.NightShift.Riots", "The Riots NightShift.");
        	
	        namespace Modifier
	        {
	        	UE_DEFINE_GAMEPLAY_TAG_COMMENT(BusAccident, "EMR.GameMode.NightShift.Modifier.BusAccident", "The Basic NightShift.");
	        }
        }
    }

    namespace RunUpgrade
    {
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "EMR.RunUpgrade", "Run upgrade root tag");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(CoffeeMachine, "EMR.RunUpgrade.CoffeeMachine", "Coffee machine upgrade unlocked for current run");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(OxygenMachine, "EMR.RunUpgrade.OxygenMachine", "Oxygen machine upgrade unlocked for current run");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(SnackMachine, "EMR.RunUpgrade.SnackMachine", "Snack machine upgrade unlocked for current run");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(MagicBox, "EMR.RunUpgrade.MagicBox", "Magic box upgrade unlocked for current run");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemDispenserSales, "EMR.RunUpgrade.ItemDispenserSales", "Item dispenser sales upgrade unlocked for current run");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(TreatmentBed, "EMR.RunUpgrade.TreatmentBed", "Treatment-bed upgrade unlocked for current run");
    }

    // ------------------------------------------------------------------------
    // Items
    // ------------------------------------------------------------------------
    namespace Items
    {
        namespace Type
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(PhysicalCare, "EMR.Items.Type.PhysicalCare", "Supplies such as bandages and splints");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Intravenous, "EMR.Items.Type.Intravenous", "IV medications and fluids");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Oral, "EMR.Items.Type.Oral", "Oral medications");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(LifeSupport, "EMR.Items.Type.LifeSupport", "Oxygen or monitoring equipment");
        }
    }
	
    // ------------------------------------------------------------------------
    // Patient
    // ------------------------------------------------------------------------
	namespace Patient
	{
		namespace Type
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Normal, "EMR.Patient.Type.Normal", "Patient is normal");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Impatient, "EMR.Patient.Type.Impatient", "Patient is Impatient");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Drunk, "EMR.Patient.Type.Drunk", "Patient is Drunk");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Prisoner, "EMR.Patient.Type.Prisoner", "Patient is Prisoner");
		}
		
		namespace BloodType
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(APositive, "EMR.Patient.BloodType.APositive", "APositive");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ANegative, "EMR.Patient.BloodType.ANegative", "ANegative");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(BPositive, "EMR.Patient.BloodType.BPositive", "BPositive");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(BNegative, "EMR.Patient.BloodType.BNegative", "BNegative");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ABPositive, "EMR.Patient.BloodType.ABPositive", "ABPositive");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ABNegative, "EMR.Patient.BloodType.ABNegative", "ABNegative");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(OPositive, "EMR.Patient.BloodType.OPositive", "OPositive");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ONegative, "EMR.Patient.BloodType.ONegative", "ONegative");
		}
		
        namespace Phase
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "EMR.Patient.Phase", "Patient root phase");
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction, "EMR.Patient.Phase.Interaction", "Patient interaction phase");
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitingInReception, "EMR.Patient.Phase.WaitingInReception", "Patient waiting in reception");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(UnderTriage, "EMR.Patient.Phase.UnderTriage", "Patient in triage");
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitingExam, "EMR.Patient.Phase.WaitingExam", "Patient waiting for exam");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(UnderExam, "EMR.Patient.Phase.UnderExam", "Patient under examination");
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitingTreatment, "EMR.Patient.Phase.WaitingTreatment", "Patient waiting for treatment");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(UnderTreatment, "EMR.Patient.Phase.UnderTreatment", "Patient under treatment");

			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitingToPay, "EMR.Patient.Phase.WaitingToPay", "Patient is waiting to pay");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Paying, "EMR.Patient.Phase.Paying", "Patient is paying");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Paid, "EMR.Patient.Phase.Paid", "Patient paid");

            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Leaving, "EMR.Patient.Phase.Leaving", "Patient leaving");
        }
		

        namespace Pathology
        {
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "EMR.Patient.Pathology", "Root");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(AnkleSprain, "EMR.Patient.Pathology.AnkleSprain", "Ankle sprain root");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dislocation, "EMR.Patient.Pathology.Dislocation", "Dislocation root");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(SimpleFracture, "EMR.Patient.Pathology.SimpleFracture", "Simple fracture root");

		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(IschemicStroke, "EMR.Patient.Pathology.IschemicStroke", "Ischemic stroke");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Concussion, "EMR.Patient.Pathology.Concussion", "Concussion");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(PulmonaryEmbolism, "EMR.Patient.Pathology.PulmonaryEmbolism", "Pulmonary embolism");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(RenalColic, "EMR.Patient.Pathology.RenalColic", "Renal colic");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pneumonia, "EMR.Patient.Pathology.Pneumonia", "Pneumonia");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pneumothorax, "EMR.Patient.Pathology.Pneumothorax", "Pneumothorax");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(LeftArmFracture, "EMR.Patient.Pathology.LeftArmFracture", "Simple fracture");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(RightArmFracture, "EMR.Patient.Pathology.RightArmFracture", "Simple fracture");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(LeftLegFracture, "EMR.Patient.Pathology.LeftLegFracture", "Simple fracture");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(RightLegFracture, "EMR.Patient.Pathology.RightLegFracture", "Simple fracture");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(LeftKneeDislocation, "EMR.Patient.Pathology.LeftKneeDislocation", "LeftKneeDislocation");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(RightKneeDislocation, "EMR.Patient.Pathology.RightKneeDislocation", "RightKneeDislocation");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(LeftShoulderDislocation, "EMR.Patient.Pathology.LeftShoulderDislocation", "LeftShoulderDislocation");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(RightShoulderDislocation, "EMR.Patient.Pathology.RightShoulderDislocation", "RightShoulderDislocation");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(LeftAnkleSprain, "EMR.Patient.Pathology.LeftAnkleSprain", "Ankle sprain");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(RightAnkleSprain, "EMR.Patient.Pathology.RightAnkleSprain", "Ankle sprain");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Appendicitis, "EMR.Patient.Pathology.Appendicitis", "Appendicitis");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cholecystitis, "EMR.Patient.Pathology.Cholecystitis", "Cholecystitis");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(CoronarySyndrome, "EMR.Patient.Pathology.CoronarySyndrome", "Coronary syndrome");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(AcuteAsthma, "EMR.Patient.Pathology.AcuteAsthma", "Acute asthma");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Sepsis, "EMR.Patient.Pathology.Sepsis", "Sepsis");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pyelonephritis, "EMR.Patient.Pathology.Pyelonephritis", "Pyelonephritis");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cystitis, "EMR.Patient.Pathology.Cystitis", "Cystitis");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(DiabeticKetoacidosis, "EMR.Patient.Pathology.DiabeticKetoacidosis", "Diabetic ketoacidosis");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SkinLaceration, "EMR.Patient.Pathology.SkinLaceration", "Skin laceration");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hernia, "EMR.Patient.Pathology.Hernia", "Hernia");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Migraine, "EMR.Patient.Pathology.Migraine", "Migraine");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Anaphylaxis, "EMR.Patient.Pathology.Anaphylaxis", "Anaphylaxis");
		}

		
        namespace Symptom
        {
		    // Ischemic stroke
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(FaceDroop, "EMR.Patient.Symptom.FaceDroop", "Face droop");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(LimbWeakness, "EMR.Patient.Symptom.LimbWeakness", "Limb weakness");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SpeechDifficulty, "EMR.Patient.Symptom.SpeechDifficulty", "Speech difficulty");

		    // Concussion
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Headache, "EMR.Patient.Symptom.Headache", "Headache");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dizziness, "EMR.Patient.Symptom.Dizziness", "Dizziness");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Nausea, "EMR.Patient.Symptom.Nausea", "Nausea");

		    // Pulmonary embolism
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ShortnessOfBreath, "EMR.Patient.Symptom.ShortnessOfBreath", "Shortness of breath");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(PleuriticChestPain, "EMR.Patient.Symptom.PleuriticChestPain", "Pleuritic chest pain");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tachycardia, "EMR.Patient.Symptom.Tachycardia", "Fast heart rate");

		    // Renal colic
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(FlankPain, "EMR.Patient.Symptom.FlankPain", "Flank pain");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(GroinRadiation, "EMR.Patient.Symptom.GroinRadiation", "Pain radiating to the groin");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hematuria, "EMR.Patient.Symptom.Hematuria", "Blood in urine");

		    // Pneumonia
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fever, "EMR.Patient.Symptom.Fever", "Fever");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ProductiveCough, "EMR.Patient.Symptom.ProductiveCough", "Productive cough");
		    // ++ ShortnessOfBreath

		    // Pneumothorax
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(UnilateralChestPain, "EMR.Patient.Symptom.UnilateralChestPain", "One-sided chest pain");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(DryCough, "EMR.Patient.Symptom.DryCough", "Dry cough");
		    // ++ ShortnessOfBreath

		    // Simple fracture
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Pain, "EMR.Patient.Symptom.Pain", "Pain");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Swelling, "EMR.Patient.Symptom.Swelling", "Swelling");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Deformity, "EMR.Patient.Symptom.Deformity", "Deformity");

		    // LeftKneeDislocation
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(JointDeformity, "EMR.Patient.Symptom.JointDeformity", "Joint deformity");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SeverePain, "EMR.Patient.Symptom.SeverePain", "Severe pain");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(LimitedMotion, "EMR.Patient.Symptom.LimitedMotion", "Limited range of motion");

		    // Ankle sprain
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(PainOnWeightBearing, "EMR.Patient.Symptom.PainOnWeightBearing", "Pain on weight-bearing");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Instability, "EMR.Patient.Symptom.Instability", "Instability");
			// ++ Swelling

		    // Appendicitis
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(RLQPain, "EMR.Patient.Symptom.RLQPain", "Right lower-quadrant pain");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(PainMigration, "EMR.Patient.Symptom.PainMigration", "Pain migration to RLQ");
		    // ++ Nausea

		    // Cholecystitis
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(RUQPain, "EMR.Patient.Symptom.RUQPain", "Right upper-quadrant pain");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(MurphySign, "EMR.Patient.Symptom.MurphySign", "Murphy sign");
		    // ++ Fever

		    // Acute coronary syndrome
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ChestPressure, "EMR.Patient.Symptom.ChestPressure", "Chest pressure");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(RadiationArmJawBack, "EMR.Patient.Symptom.RadiationArmJawBack", "Pain radiating to arm/jaw/back");
		    // ++ ShortnessOfBreath

		    // Acute asthma
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Wheeze, "EMR.Patient.Symptom.Wheeze", "Wheeze");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ChestTightness, "EMR.Patient.Symptom.ChestTightness", "Chest tightness");
			// ++ ShortnessOfBreath

		    // Sepsis
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tachypnea, "EMR.Patient.Symptom.Tachypnea", "Fast breathing");
			// ++ Fever
			// ++ Tachycardia

		    // Pyelonephritis
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dysuria, "EMR.Patient.Symptom.Dysuria", "Painful urination");
			// ++ FlankPain
			// ++ Fever

		    // Cystitis
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Frequency, "EMR.Patient.Symptom.Frequency", "Urinary frequency");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Urgency, "EMR.Patient.Symptom.Urgency", "Urinary urgency");
			// ++ Dysuria

		    // Diabetic ketoacidosis
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Polyuria, "EMR.Patient.Symptom.Polyuria", "Frequent urination");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Polydipsia, "EMR.Patient.Symptom.Polydipsia", "Excessive thirst");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(KussmaulBreathing, "EMR.Patient.Symptom.KussmaulBreathing", "Kussmaul breathing");

		    // Skin laceration
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Bleeding, "EMR.Patient.Symptom.Bleeding", "Bleeding");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(OpenWound, "EMR.Patient.Symptom.OpenWound", "Open wound");
			// ++ Pain

		    // Hernia
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(GroinBulge, "EMR.Patient.Symptom.GroinBulge", "Groin bulge");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(DiscomfortWithLifting, "EMR.Patient.Symptom.DiscomfortWithLifting", "Discomfort with lifting/coughing");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReducibleMass, "EMR.Patient.Symptom.ReducibleMass", "Reducible lump");

		    // Migraine
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(UnilateralThrobbingHeadache, "EMR.Patient.Symptom.UnilateralThrobbingHeadache", "Unilateral throbbing headache");
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Photophobia, "EMR.Patient.Symptom.Photophobia", "Photophobia");
			// ++ Nausea

		    // Anaphylaxis
		    UE_DEFINE_GAMEPLAY_TAG_COMMENT(LipTongueThroatSwelling, "EMR.Patient.Symptom.LipTongueThroatSwelling", "Swelling of lips/tongue/throat");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hives, "EMR.Patient.Symptom.Hives", "Hives");
            // ++ ShortnessOfBreath
        }
		
        namespace ExamCompleted
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "EMR.Patient.ExamCompleted", "Root tag for patient exam completion state");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(CTScan, "EMR.Patient.ExamCompleted.CTScan", "Patient has completed a CT scan");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(XRay, "EMR.Patient.ExamCompleted.XRay", "Patient has completed an X-ray");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ultrasound, "EMR.Patient.ExamCompleted.Ultrasound", "Patient has completed an ultrasound");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Electrocardiogram, "EMR.Patient.ExamCompleted.Electrocardiogram", "Patient has completed an ECG");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(LabAnalyzer, "EMR.Patient.ExamCompleted.LabAnalyzer", "Patient has completed a laboratory analysis");

			namespace LabAnalyzerTests
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(CBC, "EMR.Patient.ExamCompleted.LabAnalyzer.CBC", "Patient has completed CBC lab test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Electrolytes, "EMR.Patient.ExamCompleted.LabAnalyzer.Electrolytes", "Patient has completed Electrolytes lab test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Lactate, "EMR.Patient.ExamCompleted.LabAnalyzer.Lactate", "Patient has completed Lactate lab test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(CRP, "EMR.Patient.ExamCompleted.LabAnalyzer.CRP", "Patient has completed CRP lab test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ddimer, "EMR.Patient.ExamCompleted.LabAnalyzer.Ddimer", "Patient has completed Ddimer lab test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Toxicology, "EMR.Patient.ExamCompleted.LabAnalyzer.Toxicology", "Patient has completed Toxicology lab test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Troponin, "EMR.Patient.ExamCompleted.LabAnalyzer.Troponin", "Patient has completed Troponin lab test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Glucose, "EMR.Patient.ExamCompleted.LabAnalyzer.Glucose", "Patient has completed Glucose lab test");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(UrineDipstick, "EMR.Patient.ExamCompleted.LabAnalyzer.UrineDipstick", "Patient has completed Urine dipstick lab test");
			}
        }

        namespace Severity
        {
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Green, "EMR.Patient.Severity.Green", "Minor severity - green");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Yellow, "EMR.Patient.Severity.Yellow", "Moderate severity - yellow");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Red, "EMR.Patient.Severity.Red", "Critical severity - red");
            UE_DEFINE_GAMEPLAY_TAG_COMMENT(Black, "EMR.Patient.Severity.Black", "Unsavable severity - black");
        }

		namespace ExamQueue
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root , "EMR.Patient.ExamQueue", "Wait in queue root");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitingForCTScan , "EMR.Patient.ExamQueue.WaitingForCTScan", "Wait in queue for CTScan");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitingForXRay , "EMR.Patient.ExamQueue.WaitingForXRay", "Wait in queue for XRay");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitingForUltrasound , "EMR.Patient.ExamQueue.WaitingForUltrasound", "Wait in queue for Ultrasound");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitingForElectrocardiogram , "EMR.Patient.ExamQueue.WaitingForElectrocardiogram", "Wait in queue for Electrocardiogram");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitingForLabAnalyzer , "EMR.Patient.ExamQueue.WaitingForLabAnalyzer", "Wait in queue for LabAnalyzer");
		}

		namespace TreatmentQueue
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Root, "EMR.Patient.TreatmentQueue", "Patient TreatmentQueue");
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitForBandageTreatment, "EMR.Patient.TreatmentQueue.WaitForBandageTreatment", "Patient requires a bandage treatment");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitForSutureTreatment, "EMR.Patient.TreatmentQueue.WaitForSutureTreatment", "Patient requires a suture treatment");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitForCastTreatment, "EMR.Patient.TreatmentQueue.WaitForCastTreatment", "Patient requires a cast treatment");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitForSplintTreatment, "EMR.Patient.TreatmentQueue.WaitForSplintTreatment", "Patient requires a splint treatment");
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitForOxygenTreatment, "EMR.Patient.TreatmentQueue.WaitForOxygenTreatment", "Patient requires oxygen therapy");
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitForIntravenousMedication, "EMR.Patient.TreatmentQueue.WaitForIntravenousMedication", "Patient requires Intravenous medication treatment");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(WaitForOralMedication, "EMR.Patient.TreatmentQueue.WaitForOralMedication", "Patient requires oral medication treatment");
		}

	}

	// ------------------------------------------------------------------------
	// Stations / Rooms
	// ------------------------------------------------------------------------
	namespace Station
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reception, "EMR.Station.Reception", "Reception and triage area");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Exam, "EMR.Station.Exam", "Exams room");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Treatment, "EMR.Station.Treatment", "Treatment room");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Bathroom, "EMR.Station.Bathroom", "Bathroom");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Storage, "EMR.Station.Storage", "Storage room");
	}

	// ------------------------------------------------------------------------
	// Machines
	// ------------------------------------------------------------------------
	namespace Machine
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Any, "EMR.Machine", "Any machine event");

		UE_DEFINE_GAMEPLAY_TAG_COMMENT(PatientCaller, "EMR.Machine.PatientCaller", "Any machine event");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Reception, "EMR.Machine.Reception", "Any machine event");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(CTScan, "EMR.Machine.CTScan", "Any machine event");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(XRay, "EMR.Machine.XRay", "Any machine event");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ultrasound, "EMR.Machine.Ultrasound", "Any machine event");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Electrocardiogram, "EMR.Machine.Electrocardiogram", "Any machine event");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(LabAnalyzer, "EMR.Machine.LabAnalyzer", "Any machine event");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(IntravenousMedication, "EMR.Machine.IntravenousMedication", "Intravenous medication stand");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Oxygen, "EMR.Machine.Oxygen", "Oxygen machine");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemDispenser, "EMR.Machine.ItemDispenser", "Item dispenser");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(CoffeeMachine, "EMR.Machine.CoffeeMachine", "Coffee machine");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(SnackMachine, "EMR.Machine.SnackMachine", "Snack machine");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(MagicBox, "EMR.Machine.MagicBox", "Magic box");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(OvertimeStopTerminal, "EMR.Machine.OvertimeStopTerminal", "Overtime stop terminal");
	}
	
	// ------------------------------------------------------------------------
	// Treatment Results
	// ------------------------------------------------------------------------
    namespace Treatment
    {
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Any, "EMR.Treatment", "Any Treatment event");
	
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Bandage, "EMR.Treatment.Bandage", "Any Treatment event");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Suture, "EMR.Treatment.Suture", "Any Treatment event");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cast, "EMR.Treatment.Cast", "Any Treatment event");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Splint, "EMR.Treatment.Splint", "Any Treatment event");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Oxygen, "EMR.Treatment.Oxygen", "Any Treatment event");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(IntravenousMedication, "EMR.Treatment.IntravenousMedication", "Any Treatment event");
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(OralMedication, "EMR.Treatment.OralMedication", "Any Treatment event");
    }



    namespace UI
    {
        UE_DEFINE_GAMEPLAY_TAG_COMMENT(Validate, "EMR.UI.Validate", "Validate");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Close, "EMR.UI.Close", "Close");

		namespace WidgetStack
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Modal, "EMR.UI.WidgetStack.Modal", "To switch to Modal widget stack");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameMenu, "EMR.UI.WidgetStack.GameMenu", "To switch to GameMenu widget stack");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameHud, "EMR.UI.WidgetStack.GameHud", "To switch to GameHud widget stack");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Frontend, "EMR.UI.WidgetStack.Frontend", "To switch to Frontend widget stack");
		}

		namespace Widgets
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(PressAnyKeyScreen, "EMR.UI.Widgets.PressAnyKeyScreen", "PressAnyKeyScreen widget");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(MainMenu, "EMR.UI.Widgets.MainMenu", "MainMenu widget");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(PlayScreen, "EMR.UI.Widgets.PlayScreen", "PlayScreen widget");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(OptionsScreen, "EMR.UI.Widgets.OptionsScreen", "OptionsScreen widget");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(CreditsScreen, "EMR.UI.Widgets.CreditsScreen", "CreditsScreen widget");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(LobbyScreen, "EMR.UI.Widgets.LobbyScreen", "LobbyScreen widget");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ConfirmScreen, "EMR.UI.Widgets.ConfirmScreen", "ConfirmScreen widget");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(KeyRemapScreen, "EMR.UI.Widgets.KeyRemapScreen", "KeyRemapScreen widget");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(TriageScreen, "EMR.UI.Widgets.TriageScreen", "TriageScreen widget");
		}

		namespace OptionsImages
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(TestImage, "EMR.UI.OptionsImages.TestImage", "TestImage");
		}
	}
}
