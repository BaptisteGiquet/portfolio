#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Framework/EMRNightShiftGameState.h"
#include "Subsystems/EMRRunRulesSubsystem.h"
#include "Tests/AutomationCommon.h"
#include "Engine/GameInstance.h"
#include "UObject/UnrealType.h"

namespace EMRCompleteGameFlowTests
{
    static AEMRNightShiftGameState* SpawnRunState(UWorld* World, FAutomationTestBase& Test)
    {
        if (!World)
        {
            Test.AddError(TEXT("Test world is null."));
            return nullptr;
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.ObjectFlags = RF_Transient;

        AEMRNightShiftGameState* RunState = World->SpawnActor<AEMRNightShiftGameState>(AEMRNightShiftGameState::StaticClass(), SpawnParams);
        if (!RunState)
        {
            Test.AddError(TEXT("Failed to spawn AEMRNightShiftGameState."));
            return nullptr;
        }

        if (!RunState->HasAuthority())
        {
            Test.AddError(TEXT("Spawned run state is not authoritative."));
            return nullptr;
        }

        return RunState;
    }

    static UEMRRunRulesSubsystem* CreateRunRulesForTest(UWorld* World, FAutomationTestBase& Test)
    {
        if (!World)
        {
            Test.AddError(TEXT("Test world is null."));
            return nullptr;
        }

        UGameInstance* GameInstance = World->GetGameInstance();
        if (!GameInstance)
        {
            Test.AddError(TEXT("Test world has no game instance."));
            return nullptr;
        }

        UEMRRunRulesSubsystem* RunRules = NewObject<UEMRRunRulesSubsystem>(GameInstance);
        if (!RunRules)
        {
            Test.AddError(TEXT("Failed to allocate UEMRRunRulesSubsystem."));
            return nullptr;
        }

        return RunRules;
    }

    static bool SetRunRulesIntProperty(
        UEMRRunRulesSubsystem* RunRules,
        const TCHAR* PropertyName,
        const int32 Value,
        FAutomationTestBase& Test)
    {
        if (!RunRules)
        {
            Test.AddError(TEXT("RunRules instance is null."));
            return false;
        }

        FIntProperty* Property = FindFProperty<FIntProperty>(UEMRRunRulesSubsystem::StaticClass(), PropertyName);
        if (!Property)
        {
            Test.AddError(FString::Printf(TEXT("Failed to find int property '%s' on UEMRRunRulesSubsystem."), PropertyName));
            return false;
        }

        Property->SetPropertyValue_InContainer(RunRules, Value);
        return true;
    }

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEMRRunRulesEarlyQuotaAdvanceAutomationTest,
    "Project.RunFlow.CompleteGameFlow.RunRulesEarlyQuotaAdvance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRRunRulesEarlyQuotaAdvanceAutomationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    AddExpectedError(TEXT("String Table not found"), EAutomationExpectedErrorFlags::Contains, 1);

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        AddError(TEXT("Failed to create test world."));
        return false;
    }

    if (!WorldWrapper.BeginPlayInTestWorld())
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Failed to begin play in test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    AEMRNightShiftGameState* RunState = EMRCompleteGameFlowTests::SpawnRunState(World, *this);
    if (!RunState)
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UEMRRunRulesSubsystem* RunRules = EMRCompleteGameFlowTests::CreateRunRulesForTest(World, *this);
    if (!RunRules)
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    if (!EMRCompleteGameFlowTests::SetRunRulesIntProperty(RunRules, TEXT("NightShiftsPerCycle"), 3, *this))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    RunState->SetRunPhase(EER_RunPhase::InNightShift);
    RunState->SetRunFailed(false);
    RunState->SetFinalMissionUnlocked(false);
    RunState->SetCurrentCycleInfo(/*CycleIndex=*/2, /*CycleQuota=*/100.f, /*CycleStartRevenue=*/0.f);
    RunState->SetNightShiftIndexInCycle(0);
    RunState->SetTotalRevenue(100.f);

    const EER_RunPhase ResultPhase = RunRules->HandleNightShiftCompletion(RunState);

    TestEqual(TEXT("Returned phase should be MissionFinal"), ResultPhase, EER_RunPhase::MissionFinal);
    TestEqual(TEXT("Run phase should be MissionFinal"), RunState->GetRunPhase(), EER_RunPhase::MissionFinal);
    TestTrue(TEXT("Final mission must unlock even when quota is reached before the last shift"), RunState->IsFinalMissionUnlocked());
    TestEqual(TEXT("NightShift index should only advance for completed shift"), RunState->GetNightShiftIndexInCycle(), 1);
    TestFalse(TEXT("Run should not be marked failed"), RunState->HasRunFailed());

    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEMRRunRulesFinalPhaseAutomationTest,
    "Project.RunFlow.CompleteGameFlow.RunRulesFinalPhase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRRunRulesFinalPhaseAutomationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    AddExpectedError(TEXT("String Table not found"), EAutomationExpectedErrorFlags::Contains, 1);

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        AddError(TEXT("Failed to create test world."));
        return false;
    }

    if (!WorldWrapper.BeginPlayInTestWorld())
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Failed to begin play in test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    AEMRNightShiftGameState* RunState = EMRCompleteGameFlowTests::SpawnRunState(World, *this);
    if (!RunState)
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UEMRRunRulesSubsystem* RunRules = EMRCompleteGameFlowTests::CreateRunRulesForTest(World, *this);
    if (!RunRules)
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    RunState->SetRunPhase(EER_RunPhase::InNightShift);
    RunState->SetRunFailed(false);
    RunState->SetFinalMissionUnlocked(false);
    RunState->SetCurrentCycleInfo(/*CycleIndex=*/2, /*CycleQuota=*/100.f, /*CycleStartRevenue=*/0.f);
    RunState->SetNightShiftIndexInCycle(0);
    RunState->SetTotalRevenue(100.f);

    const EER_RunPhase ResultPhase = RunRules->HandleNightShiftCompletion(RunState);

    TestEqual(TEXT("Returned phase should be MissionFinal"), ResultPhase, EER_RunPhase::MissionFinal);
    TestEqual(TEXT("Run phase should be MissionFinal"), RunState->GetRunPhase(), EER_RunPhase::MissionFinal);
    TestTrue(TEXT("Final mission must be unlocked"), RunState->IsFinalMissionUnlocked());
    TestFalse(TEXT("Run should not be marked failed"), RunState->HasRunFailed());
    TestEqual(TEXT("NightShift index should advance"), RunState->GetNightShiftIndexInCycle(), 1);
    TestEqual(TEXT("NightShift remaining time should reset to 0"), RunState->GetRemainingTimeInNightShift(), 0.f);

    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEMRRunRulesFailurePathAutomationTest,
    "Project.RunFlow.CompleteGameFlow.RunRulesFailPath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRRunRulesFailurePathAutomationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    AddExpectedError(TEXT("String Table not found"), EAutomationExpectedErrorFlags::Contains, 1);

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        AddError(TEXT("Failed to create test world."));
        return false;
    }

    if (!WorldWrapper.BeginPlayInTestWorld())
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Failed to begin play in test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    AEMRNightShiftGameState* RunState = EMRCompleteGameFlowTests::SpawnRunState(World, *this);
    if (!RunState)
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UEMRRunRulesSubsystem* RunRules = EMRCompleteGameFlowTests::CreateRunRulesForTest(World, *this);
    if (!RunRules)
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    RunState->SetRunPhase(EER_RunPhase::InNightShift);
    RunState->SetRunFailed(false);
    RunState->SetFinalMissionUnlocked(false);
    RunState->SetCurrentCycleInfo(/*CycleIndex=*/0, /*CycleQuota=*/100.f, /*CycleStartRevenue=*/0.f);
    RunState->SetNightShiftIndexInCycle(0);
    RunState->SetTotalRevenue(50.f);

    const EER_RunPhase ResultPhase = RunRules->HandleNightShiftCompletion(RunState);

    TestEqual(TEXT("Returned phase should be RunFinished"), ResultPhase, EER_RunPhase::RunFinished);
    TestEqual(TEXT("Run phase should be RunFinished"), RunState->GetRunPhase(), EER_RunPhase::RunFinished);
    TestTrue(TEXT("Run should be marked failed"), RunState->HasRunFailed());
    TestFalse(TEXT("Final mission must remain locked"), RunState->IsFinalMissionUnlocked());
    TestEqual(TEXT("NightShift index should advance"), RunState->GetNightShiftIndexInCycle(), 1);
    TestEqual(TEXT("NightShift remaining time should reset to 0"), RunState->GetRemainingTimeInNightShift(), 0.f);

    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
