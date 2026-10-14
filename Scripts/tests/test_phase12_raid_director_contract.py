#!/usr/bin/env python3
"""Static source-contract checks for Phase 12 Raid Director integration."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "Source" / "MyProject2"
DIRECTOR_H = (SRC / "RaidCore" / "RaidDirectorSubsystem.h").read_text(encoding="utf-8")
DIRECTOR_CPP = (SRC / "RaidCore" / "RaidDirectorSubsystem.cpp").read_text(encoding="utf-8")
CONTEXT = (SRC / "RaidCore" / "RaidContext.h").read_text(encoding="utf-8")
LIFECYCLE_H = (SRC / "RaidPhaseR1_Trigger" / "RaidLifecycleComponent.h").read_text(encoding="utf-8")
LIFECYCLE_CPP = (SRC / "RaidPhaseR1_Trigger" / "RaidLifecycleComponent.cpp").read_text(encoding="utf-8")
SCHEDULER_CPP = (SRC / "RaidPhaseR1_Trigger" / "RaidSchedulerComponent.cpp").read_text(encoding="utf-8")
PLAN_CPP = (SRC / "RaidPhaseR3_Planner" / "HordePlanAsset.cpp").read_text(encoding="utf-8")
STAGE_CPP = (SRC / "RaidPhaseR2_Difficulty" / "RaidStageTableAsset.cpp").read_text(encoding="utf-8")
BUDGET_CPP = (SRC / "RaidPhaseR4_Budget" / "SpawnBudgetTracker.cpp").read_text(encoding="utf-8")
ENEMY_H = (SRC / "RaidEnemy.h").read_text(encoding="utf-8")
ENEMY_CPP = (SRC / "RaidEnemy.cpp").read_text(encoding="utf-8")
DAMAGE_CPP = (SRC / "RaidPhaseR8_Defense" / "StructureDamageResolver.cpp").read_text(encoding="utf-8")
LOG_H = (SRC / "Phase1_Physical" / "LogActor.h").read_text(encoding="utf-8")
BRIDGE_CPP = (SRC / "Phase4_Construction" / "BridgeComponent.cpp").read_text(encoding="utf-8")
LOG_SAVE_H = (SRC / "Phase7_Integration" / "SaveGameSubsystem.h").read_text(encoding="utf-8")
LOG_SAVE_CPP = (SRC / "Phase7_Integration" / "SaveGameSubsystem.cpp").read_text(encoding="utf-8")
HARNESS_H = (SRC / "RaidPhaseR11_Testing" / "RaidTestHarness.h").read_text(encoding="utf-8")
HARNESS_CPP = (SRC / "RaidPhaseR11_Testing" / "RaidTestHarness.cpp").read_text(encoding="utf-8")
RUNNER = (ROOT / "Scripts" / "run_python_tests.sh").read_text(encoding="utf-8")


def between(source: str, start: str, end: str) -> str:
    a = source.find(start)
    if a < 0:
        raise AssertionError(f"missing start anchor: {start}")
    b = source.find(end, a + len(start))
    if b < 0:
        raise AssertionError(f"missing end anchor after: {start}")
    return source[a:b]


class Phase12RaidDirectorContract(unittest.TestCase):
    def test_raid_start_is_single_active_and_validates_before_lifecycle_activation(self):
        start = between(DIRECTOR_CPP, "void URaidDirectorSubsystem::StartRaid(", "void URaidDirectorSubsystem::EndRaid(")
        self.assertIn("Context.bEventActive", start)
        self.assertIn("Debouncer->TryAcquireEventLock", start)
        self.assertLess(start.find("ValidateCurrentPlan"), start.find("Lifecycle->BeginEvent(Context)"))
        self.assertIn("StageTable->ValidateTable", start)
        self.assertIn("Context.Players.IsEmpty()", start)

    def test_plan_and_every_weighted_spawn_member_fail_closed_on_invalid_data(self):
        self.assertIn("CurrentPlan->ValidatePlan(OutError)", DIRECTOR_CPP)
        valid = between(DIRECTOR_CPP, "bool URaidDirectorSubsystem::ValidateCurrentPlan(", "bool URaidDirectorSubsystem::HasNavigationData(")
        for required in ("EnemyGroups->FindGroup", "Group->Members.IsEmpty()", "Member.EnemyID.IsNone()",
                         "FMath::IsFinite(Member.Weight)", "Member.Weight<=0.f",
                         "Enemies->FindEnemy(Member.EnemyID)", "!Definition||!Definition->EnemyClass"):
            self.assertIn(required, valid)
        self.assertIn("Waves.Num()>64", PLAN_CPP)
        self.assertIn("TotalRequested>100000", PLAN_CPP)

    def test_one_director_owns_spawn_accounting_and_wave_wide_event_budget(self):
        spawn = between(DIRECTOR_CPP, "void URaidDirectorSubsystem::ProcessSpawns(", "bool URaidDirectorSubsystem::ValidateCurrentPlan(")
        self.assertEqual(spawn.count("SpawnActor<AActor>"), 1)
        self.assertIn("EventBudgetRemaining", spawn)
        self.assertIn("ComputeSpawnRequest(FMath::Min(Budget->GetRemainingBudget(),EventBudgetRemaining),Free)", spawn)
        self.assertLess(spawn.find("if(!Enemy)continue"), spawn.find("Budget->OnSpawned()"))
        self.assertIn("EventBudgetRemaining=FMath::Max(0,EventBudgetRemaining-1)", spawn)
        self.assertIn("Counter->OnEntitySpawned(Enemy,EnemyID)", spawn)
        self.assertIn("ManagedEnemies.AddUnique(Enemy)", spawn)
        self.assertIn("Budget->Initialize(FMath::Min(Next.TotalCount,EventBudgetRemaining))", DIRECTOR_CPP)

    def test_cancel_only_destroys_actors_owned_by_this_director(self):
        self.assertIn("TArray<TObjectPtr<AActor>> ManagedEnemies", DIRECTOR_H)
        cleanup = between(DIRECTOR_CPP, "void URaidDirectorSubsystem::CleanupManagedEnemies(", "void URaidDirectorSubsystem::HandleEnemyDestroyed(")
        self.assertIn("TArray<TObjectPtr<AActor>> ToDestroy=ManagedEnemies", cleanup)
        self.assertIn("Enemy->OnDestroyed.RemoveDynamic", cleanup)
        self.assertIn("Counter->OnEntityRemoved(Enemy)", cleanup)
        self.assertIn("Enemy->Destroy()", cleanup)
        stop = between(ENEMY_CPP, "void RaidKit::StopRaid(", "int32 RaidKit::CountEnemies(")
        self.assertIn("Director->CancelRaid", stop)
        self.assertNotIn("TActorIterator<ARaidEnemy>", stop)
        self.assertNotIn("It->Destroy()", stop)

    def test_opt_in_scheduler_is_driven_by_director_and_retries_failed_start_bounded(self):
        self.assertIn("SetAutomaticSchedulingEnabled", DIRECTOR_H)
        config = between(DIRECTOR_CPP, "bool URaidDirectorSubsystem::SetAutomaticSchedulingEnabled(", "void URaidDirectorSubsystem::Tick(")
        self.assertIn("FMath::IsFinite(IntervalSeconds)", config)
        self.assertIn("IntervalSeconds<30.0f||IntervalSeconds>604800.0f", config)
        tick = between(DIRECTOR_CPP, "void URaidDirectorSubsystem::Tick(", "void URaidDirectorSubsystem::StartRaid(")
        self.assertIn("Scheduler->IsEventDue", tick)
        self.assertIn("StartRaid(AutoScheduleEventID,AutoScheduleEventType)", tick)
        self.assertIn("ScheduleNextEvent(World->GetTimeSeconds()+60.0f,0.0f,0.0f)", tick)
        self.assertIn("Scheduler->ScheduleNextEvent", config)

    def test_nav_queries_share_frame_budget_and_bound_path_output(self):
        self.assertIn("FrameBudget->ResetFrame()", DIRECTOR_CPP)
        query = between(DIRECTOR_CPP, "bool URaidDirectorSubsystem::FindPathForEnemy(", "void URaidDirectorSubsystem::CleanupManagedEnemies(")
        self.assertIn("FrameBudget->TryConsumePathfind()", query)
        self.assertIn("Path->PathPoints.Num()<2||Path->PathPoints.Num()>256", query)
        self.assertIn("OutPath=Path->PathPoints", query)
        tick = between(DIRECTOR_CPP, "void URaidDirectorSubsystem::Tick(", "void URaidDirectorSubsystem::StartRaid(")
        self.assertIn("FrameBudget->ResetFrame()", tick)

    def test_enemy_uses_director_navmesh_path_when_available_and_does_not_bypass_failed_query(self):
        self.assertIn("HasNavigationData()", ENEMY_CPP)
        self.assertIn("Director->FindPathForEnemy(this, Goal, NewPath)", ENEMY_CPP)
        nav_move = between(ENEMY_CPP, "FVector MoveDirection = ToGoal.GetSafeNormal();", "FHitResult Hit;")
        self.assertIn("if (!Director->FindPathForEnemy", nav_move)
        self.assertIn("return; // Never bypass an available navmesh", nav_move)
        self.assertIn("CachedNavWaypoint", ENEMY_H)
        self.assertIn("virtual void BeginPlay() override", ENEMY_H)

    def test_structural_damage_is_finite_and_invalidates_navmesh_after_destruction(self):
        self.assertIn("FMath::IsFinite(Damage)", DAMAGE_CPP)
        self.assertIn("Damage<=0.f", DAMAGE_CPP)
        self.assertIn("DamageComponent->ApplyDamage", DAMAGE_CPP)
        self.assertIn("Navigation->AddDirtyArea", DAMAGE_CPP)
        self.assertIn("Logistics->SetInfrastructureStatus(Log->InfrastructureLinkKey,false,0.0f,1.0f)", DAMAGE_CPP)
        self.assertIn("DamageResolver->ApplyDamageToBlock", ENEMY_CPP)

    def test_damaged_bridge_route_key_is_bound_to_physical_pieces_and_saved(self):
        self.assertIn("FName InfrastructureLinkKey = NAME_None", LOG_H)
        self.assertIn("A->InfrastructureLinkKey=LogisticsLinkKey", BRIDGE_CPP)
        self.assertIn("InfrastructureLinkKey = NAME_None", LOG_SAVE_H)
        self.assertIn("D.InfrastructureLinkKey=L->InfrastructureLinkKey", LOG_SAVE_CPP)
        self.assertIn("L->InfrastructureLinkKey=D.InfrastructureLinkKey", LOG_SAVE_CPP)

    def test_raid_context_lifecycle_moves_across_preparing_active_intermission_and_end(self):
        self.assertIn("SetPhase(FRaidContext& Context,ERaidLifecycleState NewState)", LIFECYCLE_H)
        self.assertIn("C.Lifecycle=S", LIFECYCLE_CPP)
        self.assertIn("Lifecycle->SetPhase(Context,ERaidLifecycleState::Intermission)", DIRECTOR_CPP)
        self.assertIn("Lifecycle->SetPhase(Context,ERaidLifecycleState::Active)", DIRECTOR_CPP)
        self.assertIn("Lifecycle->EndEvent(Context)", DIRECTOR_CPP)
        self.assertIn("Lifecycle->CancelEvent(Context,Reason)", DIRECTOR_CPP)

    def test_stage_configuration_has_bounded_finite_difficulty_and_population_values(self):
        for required in ("FMath::IsFinite(LevelMultiplier)", "FMath::IsFinite(SurvivalTimeMultiplier)",
                         "StageIndexes.Contains", "ConcurrentPopulation>256", "TotalBudget>100000",
                         "FMath::IsFinite(S.Budget.RaidDuration)"):
            self.assertIn(required, STAGE_CPP)

    def test_runtime_harness_includes_plan_budget_lifecycle_and_damage_contracts(self):
        for test_name in ("TestRaidPlanValidation", "TestRaidStageBudgetValidation",
                          "TestRaidLifecycleTransitions", "TestStructureDamageSanitization"):
            self.assertIn(test_name, HARNESS_H)
            self.assertIn(test_name, HARNESS_CPP)
        self.assertIn('TEXT("RaidPlanValidation")', HARNESS_CPP)
        self.assertIn('TEXT("StructureDamageSanitization")', HARNESS_CPP)

    def test_spawn_scheduler_and_damage_components_are_not_duplicate_owners(self):
        self.assertIn("SpawnActor<AActor>", DIRECTOR_CPP)
        self.assertIn("URaidDirectorSubsystem* Director = World ? World->GetSubsystem<URaidDirectorSubsystem>()", ENEMY_CPP)
        self.assertIn("Director->CancelRaid", ENEMY_CPP)
        self.assertIn("FNavigationSystem::GetCurrent<UNavigationSystemV1>", DAMAGE_CPP)
        self.assertNotIn("SpawnActor<ARaidEnemy>", ENEMY_CPP)

    def test_phase12_contract_is_registered_in_ci_runner(self):
        path = "Scripts/tests/test_phase12_raid_director_contract.py"
        self.assertIn(path, RUNNER)
        self.assertIn("PHASE 12 RAID DIRECTOR CONTRACT", RUNNER)


if __name__ == "__main__":
    unittest.main(verbosity=2)
