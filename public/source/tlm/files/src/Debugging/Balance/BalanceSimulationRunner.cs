using Godot;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Commands;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Loot;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;
using TheLastMage.Save;

namespace TheLastMage.Debugging.Balance;

public sealed class BalanceSimulationRunner
{
    private const float FixedDelta = 0.1f;
    private const float BotArenaRadius = 18f;
    private const float BotDangerRadius = 12f;
    private const float BotComfortRadius = 8f;
    private readonly OfferGenerator _offerGenerator = new();

    private readonly record struct RunSpec(int Seed, ContentId MageId, IBalanceBotPolicy Policy, ContentId FocusSpellId);

    public BalanceExperimentResult Run(
        BalanceExperimentDefinition experiment,
        ContentRegistry content,
        ProfileSaveDto sourceProfile,
        Func<int, ProfileSaveDto, RunContext> createRunContext)
    {
        var policies = experiment.BotPolicyIds
            .Distinct(StringComparer.Ordinal)
            .Select(BalanceBotPolicyFactory.Create)
            .ToArray();
        if (policies.Length == 0)
        {
            policies = new[] { BalanceBotPolicyFactory.Create(BalanceBotPolicyIds.RandomBuild) };
        }
        var profile = BuildExperimentProfile(content, sourceProfile, experiment.ItemProfileMode);
        var result = new BalanceExperimentResult
        {
            ExperimentId = experiment.ExperimentId,
            Description = experiment.Description,
            StaticAnalysis = BalanceStaticAnalyzer.Analyze(content),
            MonteCarlo = new BalanceMonteCarloSimulator().Run(
                content,
                experiment,
                profile,
                policies,
                experiment.SeedStart)
        };

        result.Warnings.AddRange(result.StaticAnalysis.Warnings);
        var mageIds = ResolveMageIds(content, experiment);
        var focusSpellIds = ResolveFocusSpellIds(content, experiment);
        foreach (var spec in EnumerateRunSpecs(experiment, mageIds, policies, focusSpellIds))
        {
            var runProfile = BuildExperimentProfile(content, sourceProfile, experiment.ItemProfileMode);
            var context = createRunContext(spec.Seed, runProfile);
            var runResult = RunSingleExperiment(context, experiment, spec.Policy, spec.MageId, spec.Seed, spec.FocusSpellId);
            result.Runs.Add(runResult);
        }

        DetectOutliers(result);
        result.FinishedAtUtc = DateTime.UtcNow;
        return result;
    }

    public async Task<BalanceExperimentResult> RunWithProgressAsync(
        BalanceExperimentDefinition experiment,
        ContentRegistry content,
        ProfileSaveDto sourceProfile,
        Func<int, ProfileSaveDto, RunContext> createRunContext,
        Func<BalanceLabProgress, Task> progressCallback)
    {
        var policies = experiment.BotPolicyIds
            .Distinct(StringComparer.Ordinal)
            .Select(BalanceBotPolicyFactory.Create)
            .ToArray();
        if (policies.Length == 0)
        {
            policies = new[] { BalanceBotPolicyFactory.Create(BalanceBotPolicyIds.RandomBuild) };
        }

        var profile = BuildExperimentProfile(content, sourceProfile, experiment.ItemProfileMode);
        await progressCallback(new BalanceLabProgress
        {
            Phase = "static_analysis",
            TotalRuns = ResolveExpectedRunCount(experiment, content, policies)
        });

        var staticAnalysis = BalanceStaticAnalyzer.Analyze(content);
        await progressCallback(new BalanceLabProgress
        {
            Phase = "monte_carlo",
            TotalRuns = ResolveExpectedRunCount(experiment, content, policies)
        });

        var monteCarlo = new BalanceMonteCarloSimulator().Run(
            content,
            experiment,
            profile,
            policies,
            experiment.SeedStart);
        var result = new BalanceExperimentResult
        {
            ExperimentId = experiment.ExperimentId,
            Description = experiment.Description,
            StaticAnalysis = staticAnalysis,
            MonteCarlo = monteCarlo
        };

        result.Warnings.AddRange(result.StaticAnalysis.Warnings);
        var mageIds = ResolveMageIds(content, experiment);
        var focusSpellIds = ResolveFocusSpellIds(content, experiment);
        var specs = EnumerateRunSpecs(experiment, mageIds, policies, focusSpellIds).ToArray();
        await progressCallback(BuildProgress("running", result, specs.Length, default, "-"));

        for (var index = 0; index < specs.Length; index++)
        {
            var spec = specs[index];
            await progressCallback(BuildProgress("running", result, specs.Length, spec, "starting"));
            var runProfile = BuildExperimentProfile(content, sourceProfile, experiment.ItemProfileMode);
            var context = createRunContext(spec.Seed, runProfile);
            var runResult = RunSingleExperiment(context, experiment, spec.Policy, spec.MageId, spec.Seed, spec.FocusSpellId);
            result.Runs.Add(runResult);
            await progressCallback(BuildProgress("running", result, specs.Length, spec, runResult.Outcome));
        }

        DetectOutliers(result);
        result.FinishedAtUtc = DateTime.UtcNow;
        await progressCallback(BuildProgress("writing_report", result, specs.Length, default, "-"));
        return result;
    }

    public BalanceRunResult Replay(
        BalanceExperimentDefinition experiment,
        int seed,
        ContentId mageId,
        string botPolicyId,
        ProfileSaveDto sourceProfile,
        ContentRegistry content,
        Func<int, ProfileSaveDto, RunContext> createRunContext,
        ContentId focusSpellId = default)
    {
        var profile = BuildExperimentProfile(content, sourceProfile, experiment.ItemProfileMode);
        var context = createRunContext(seed, profile);
        var resolvedFocusSpellId = focusSpellId.IsValid
            ? focusSpellId
            : ResolveFocusSpellIds(content, experiment).FirstOrDefault();
        return RunSingleExperiment(context, experiment, BalanceBotPolicyFactory.Create(botPolicyId), mageId, seed, resolvedFocusSpellId);
    }

    private BalanceRunResult RunSingleExperiment(
        RunContext context,
        BalanceExperimentDefinition experiment,
        IBalanceBotPolicy policy,
        ContentId mageId,
        int seed,
        ContentId focusSpellId)
    {
        context.State.IsBenchmarkRun = false;
        context.State.MarkDebugRun("balance lab simulation");
        context.State.SuppressAutomaticEnemySpawns = false;
        context.State.HumanityRemaining = Math.Max(1, experiment.StartingHumanity);
        context.State.Player.Position = Vector3.Zero;
        if (mageId.IsValid)
        {
            context.Commands.TryExecute(new SelectMageCommand(mageId, allowLocked: true), context, out _);
        }

        ConfigureSpellLoadout(context, experiment, focusSpellId, seed);
        var effectiveFocusSpellId = focusSpellId.IsValid ? focusSpellId : context.State.Spellbook.Slots.FirstOrDefault(slot => slot.HasSpell)?.SpellId ?? ContentId.From(string.Empty);
        var result = new BalanceRunResult
        {
            RunId = context.State.RunId.ToString(),
            Seed = seed,
            ExperimentId = experiment.ExperimentId,
            RunMode = experiment.RunMode.ToString(),
            MageId = context.State.SelectedMageId.ToString(),
            BotPolicyId = policy.Id,
            FocusSpellId = effectiveFocusSpellId.IsValid ? effectiveFocusSpellId.ToString() : "-",
            ItemProfileMode = experiment.ItemProfileMode.ToString(),
            Humanity = context.State.HumanityRemaining,
            Souls = context.State.Souls,
            Materials = context.State.Materials,
            ReproductionCommand =
                $"balance_replay experiment={experiment.ExperimentId} seed={seed} mage={context.State.SelectedMageId} policy={policy.Id} focus={effectiveFocusSpellId} mode={experiment.RunMode}"
        };
        result.StartingSpellIds.AddRange(context.State.Spellbook.Slots.Where(slot => slot.HasSpell).Select(slot => slot.SpellId.ToString()));

        var recorder = new BalanceRunEventRecorder(result, context.State.Player.EntityId);
        recorder.Attach(context.Events);
        try
        {
            SimulateRun(context, experiment, policy, seed, result);
        }
        finally
        {
            recorder.Detach(context.Events);
        }

        result.DayReached = context.State.DayIndex;
        result.Humanity = context.State.HumanityRemaining;
        result.Souls = context.State.Souls;
        result.Materials = context.State.Materials;
        result.FinalHealth = ReadPlayerHealth(context);
        result.FeedbackDrops = context.State.DebugMetrics.FeedbackBudgetDropsThisFrame;
        result.PerformancePeakMs = MathF.Max(result.PerformancePeakMs, context.State.DebugMetrics.FrameMilliseconds);
        result.AllocationPeakBytes = Math.Max(result.AllocationPeakBytes, context.State.DebugMetrics.AllocatedBytesThisFrame);
        if (context.State.CurrentPhase == RunPhase.RunDefeat)
        {
            result.Outcome = "defeat";
            if (result.DeathCause == "-")
            {
                result.DeathCause = context.State.DebugMetrics.DeathCauseSummary;
            }
        }
        else if (context.State.HumanityRemaining <= 0 || HasClearedFinalConfiguredDay(context, experiment, result))
        {
            result.Outcome = "victory";
        }
        else if (result.DayReached >= experiment.MaxSimulatedDays || context.State.CurrentPhase == RunPhase.DayCombat)
        {
            result.Outcome = "timeout";
        }

        return result;
    }

    private static bool HasClearedFinalConfiguredDay(
        RunContext context,
        BalanceExperimentDefinition experiment,
        BalanceRunResult result)
    {
        return result.DayReached >= Math.Max(1, experiment.MaxSimulatedDays)
            && context.State.CurrentPhase != RunPhase.DayCombat
            && context.State.CurrentPhase != RunPhase.RunDefeat;
    }

    private void SimulateRun(
        RunContext context,
        BalanceExperimentDefinition experiment,
        IBalanceBotPolicy policy,
        int seed,
        BalanceRunResult result)
    {
        var random = new System.Random(seed ^ StableHash(policy.Id) ^ StableHash(result.FocusSpellId));
        var phaseMachine = context.GetSystem<RunPhaseStateMachine>();
        var totalDays = Math.Max(1, experiment.MaxSimulatedDays);
        context.State.DayIndex = 1;
        phaseMachine.SetPhase(RunPhase.DayCombat);
        while (context.State.DayIndex <= totalDays
            && context.State.CurrentPhase != RunPhase.RunDefeat
            && context.State.CurrentPhase != RunPhase.RunVictory)
        {
            var elapsedThisDay = 0f;
            var dayDuration = ResolveDayDuration(experiment);
            var nextCastAt = 0f;
            var activatableUsed = false;
            while (elapsedThisDay < dayDuration
                && context.State.CurrentPhase == RunPhase.DayCombat)
            {
                UpdateBotMovement(context, random, FixedDelta);
                if (elapsedThisDay >= nextCastAt)
                {
                    var spellId = ChooseReadySpell(context, policy, random, ContentId.From(result.FocusSpellId));
                    if (spellId.IsValid)
                    {
                        CastThroughCommand(context, spellId, random);
                    }

                    nextCastAt = elapsedThisDay + 0.18f + (float)random.NextDouble() * 0.16f;
                }

                if (!activatableUsed && policy.ShouldUseActivatable(context, elapsedThisDay))
                {
                    context.Commands.TryExecute(new UseActivatableItemCommand(), context, out _);
                    activatableUsed = true;
                }

                TickProductionSystems(context, FixedDelta, result);
                elapsedThisDay += FixedDelta;
                result.ElapsedSimulatedSeconds += FixedDelta;
            }

            if (context.State.CurrentPhase == RunPhase.DayCombat)
            {
                break;
            }

            if (context.State.CurrentPhase == RunPhase.RunDefeat || context.State.HumanityRemaining <= 0)
            {
                break;
            }

            if (context.State.CurrentPhase != RunPhase.NightMarket)
            {
                phaseMachine.SetPhase(RunPhase.NightMarket);
            }

            ChooseNightOffer(context, experiment, policy, random, result);
            if (context.State.DayIndex < totalDays)
            {
                context.Commands.TryExecute(new StartNextDayCommand(), context, out _);
            }
            else
            {
                break;
            }
        }
    }

    private void ChooseNightOffer(
        RunContext context,
        BalanceExperimentDefinition experiment,
        IBalanceBotPolicy policy,
        System.Random random,
        BalanceRunResult result)
    {
        var offers = _offerGenerator.Generate(
            context.Content,
            context.Profile,
            context.State.Build,
            random,
            experiment.ItemPoolId);
        context.State.Market.SetOffers(offers);
        if (offers.Count == 0)
        {
            return;
        }

        var currentSpell = context.State.Spellbook.Slots.FirstOrDefault(slot => slot.HasSpell)?.SpellId ?? ContentId.From(string.Empty);
        var chosen = policy.ChooseOffer(context, offers, random, currentSpell);
        if (chosen < 0 || chosen >= offers.Count)
        {
            return;
        }

        context.Commands.TryExecute(new ChooseMarketOfferCommand(chosen), context, out _);
    }

    private static bool CastThroughCommand(RunContext context, ContentId spellId, System.Random random)
    {
        var slotIndex = -1;
        for (var i = 0; i < context.State.Spellbook.Slots.Count; i++)
        {
            var slot = context.State.Spellbook.Slots[i];
            if (slot.HasSpell && slot.SpellId.Equals(spellId))
            {
                slotIndex = i;
                break;
            }
        }

        if (slotIndex < 0)
        {
            return false;
        }

        var direction = ChooseAimDirection(context, random);
        context.State.Player.AimDirection = direction;
        return context.Commands.TryExecute(
            new CastSpellCommand(context.State.Player.EntityId, slotIndex, context.State.Player.Position, direction),
            context,
            out _);
    }

    private static bool IsSpellReady(RunContext context, ContentId spellId)
    {
        foreach (var slot in context.State.Spellbook.Slots)
        {
            if (slot.HasSpell && slot.SpellId.Equals(spellId) && slot.IsReady)
            {
                return true;
            }
        }

        return false;
    }

    private static ContentId ChooseReadySpell(
        RunContext context,
        IBalanceBotPolicy policy,
        System.Random random,
        ContentId focusSpellId)
    {
        if (focusSpellId.IsValid && IsSpellReady(context, focusSpellId))
        {
            return focusSpellId;
        }

        var policySpell = policy.ChooseSpell(context, random);
        if (policySpell.IsValid && IsSpellReady(context, policySpell))
        {
            return policySpell;
        }

        return context.State.Spellbook.Slots
            .Where(slot => slot.HasSpell && slot.IsReady)
            .Select(slot => slot.SpellId)
            .FirstOrDefault();
    }

    private static Vector3 ChooseAimDirection(RunContext context, System.Random random)
    {
        var enemySystem = context.GetSystem<EnemySystem>();
        var combat = context.GetSystem<CombatSystem>();
        var playerPosition = context.State.Player.Position;
        var closestDistanceSquared = float.MaxValue;
        var closestPosition = Vector3.Zero;
        var found = false;
        foreach (var enemy in enemySystem.ActiveEnemies)
        {
            if (!combat.IsAlive(enemy.EntityId))
            {
                continue;
            }

            var distanceSquared = enemy.Position.DistanceSquaredTo(playerPosition);
            if (distanceSquared < closestDistanceSquared)
            {
                closestDistanceSquared = distanceSquared;
                closestPosition = enemy.Position;
                found = true;
            }
        }

        if (!found)
        {
            var angle = random.NextDouble() * Math.Tau;
            return new Vector3((float)Math.Sin(angle), 0f, -(float)Math.Cos(angle)).Normalized();
        }

        var direction = closestPosition - playerPosition;
        direction.Y = 0f;
        return direction.LengthSquared() <= 0.0001f ? Vector3.Forward : direction.Normalized();
    }

    private static void UpdateBotMovement(RunContext context, System.Random random, float delta)
    {
        var enemySystem = context.GetSystem<EnemySystem>();
        var combat = context.GetSystem<CombatSystem>();
        var position = context.State.Player.Position;
        var desired = Vector3.Zero;
        var closestDistance = float.MaxValue;
        var closestVector = Vector3.Zero;
        var liveEnemies = 0;

        foreach (var enemy in enemySystem.ActiveEnemies)
        {
            if (!combat.IsAlive(enemy.EntityId))
            {
                continue;
            }

            liveEnemies++;
            var away = position - enemy.Position;
            away.Y = 0f;
            var distance = away.Length();
            if (distance < closestDistance)
            {
                closestDistance = distance;
                closestVector = away;
            }

            if (distance > 0.001f && distance < BotDangerRadius)
            {
                desired += away / distance * ((BotDangerRadius - distance) / BotDangerRadius);
            }
        }

        if (liveEnemies == 0)
        {
            desired = -Flatten(position);
        }
        else if (closestDistance > BotComfortRadius * 1.4f && closestVector.LengthSquared() > 0.0001f)
        {
            desired -= closestVector.Normalized() * 0.35f;
        }
        else if (closestVector.LengthSquared() > 0.0001f)
        {
            var closestDirection = closestVector.Normalized();
            var strafeSign = random.Next(2) == 0 ? -1f : 1f;
            desired += new Vector3(-closestDirection.Z, 0f, closestDirection.X) * strafeSign * 0.45f;
        }

        var flatPosition = Flatten(position);
        var distanceFromCenter = flatPosition.Length();
        if (distanceFromCenter > BotArenaRadius * 0.65f)
        {
            desired += -flatPosition.Normalized() * Mathf.Clamp((distanceFromCenter - BotArenaRadius * 0.65f) / (BotArenaRadius * 0.35f), 0f, 1.25f);
        }

        if (desired.LengthSquared() <= 0.0001f)
        {
            return;
        }

        var tags = Array.Empty<TagId>();
        var moveSpeed = PlayerAttributeResolver.ResolveValue(
            context,
            PlayerAttributeResolver.MoveSpeed,
            context.State.Player.BaseMoveSpeed,
            tags,
            false);
        var nextPosition = position + desired.Normalized() * MathF.Max(0.1f, moveSpeed) * 0.85f * delta;
        var nextFlat = Flatten(nextPosition);
        if (nextFlat.Length() > BotArenaRadius)
        {
            nextFlat = nextFlat.Normalized() * BotArenaRadius;
            nextPosition = new Vector3(nextFlat.X, position.Y, nextFlat.Z);
        }

        context.State.Player.Position = nextPosition;
    }

    private static Vector3 Flatten(Vector3 value)
    {
        return new Vector3(value.X, 0f, value.Z);
    }

    private static void TickProductionSystems(RunContext context, float delta, BalanceRunResult result)
    {
        context.State.DebugMetrics.ResetFrameCounters();
        context.State.DebugMetrics.FrameMilliseconds = delta * 1000f;
        foreach (var system in context.Systems)
        {
            system.FixedTick(delta);
        }

        foreach (var system in context.Systems)
        {
            system.Tick(delta);
        }

        result.PerformancePeakMs = MathF.Max(result.PerformancePeakMs, context.State.DebugMetrics.FrameMilliseconds);
        result.AllocationPeakBytes = Math.Max(result.AllocationPeakBytes, context.State.DebugMetrics.AllocatedBytesThisFrame);
        result.FeedbackDrops += context.State.DebugMetrics.FeedbackBudgetDropsThisFrame;
    }

    private static float ResolveDayDuration(BalanceExperimentDefinition experiment)
    {
        if (experiment.MaxSimulatedSecondsPerDay > 0f)
        {
            return experiment.MaxSimulatedSecondsPerDay;
        }

        return experiment.RunMode switch
        {
            BalanceRunMode.ShortRun => 8f,
            BalanceRunMode.VerticalSlice => 20f,
            BalanceRunMode.AcceleratedLongRun => 30f,
            BalanceRunMode.Stress => 12f,
            _ => 12f
        };
    }

    private static IReadOnlyList<ContentId> ResolveMageIds(ContentRegistry content, BalanceExperimentDefinition experiment)
    {
        var explicitMages = experiment.MageIncludeList
            .Where(id => id.IsValid && content.Mages.ContainsKey(id))
            .Distinct()
            .ToArray();
        return explicitMages.Length > 0
            ? explicitMages
            : content.Mages.Keys.OrderBy(id => id.ToString(), StringComparer.Ordinal).ToArray();
    }

    private static IReadOnlyList<ContentId> ResolveFocusSpellIds(ContentRegistry content, BalanceExperimentDefinition experiment)
    {
        var explicitSpells = experiment.SpellPool
            .Where(id => id.IsValid && content.Spells.ContainsKey(id))
            .Distinct()
            .ToArray();
        return explicitSpells.Length > 0
            ? explicitSpells
            : content.Spells.Keys.OrderBy(id => id.ToString(), StringComparer.Ordinal).ToArray();
    }

    private static int ResolveExpectedRunCount(
        BalanceExperimentDefinition experiment,
        ContentRegistry content,
        IReadOnlyList<IBalanceBotPolicy> policies)
    {
        if (experiment.TotalSimulationRuns > 0)
        {
            return Math.Max(1, experiment.TotalSimulationRuns);
        }

        return experiment.EnumerateSeeds().Count
            * Math.Max(1, ResolveMageIds(content, experiment).Count)
            * Math.Max(1, policies.Count);
    }

    private static BalanceLabProgress BuildProgress(
        string phase,
        BalanceExperimentResult result,
        int totalRuns,
        RunSpec spec,
        string lastOutcome)
    {
        var completedRuns = result.Runs.Count;
        var totalSimulatedSeconds = result.Runs.Sum(run => run.ElapsedSimulatedSeconds);
        var runCount = Math.Max(1, completedRuns);
        return new BalanceLabProgress
        {
            Phase = phase,
            CompletedRuns = completedRuns,
            TotalRuns = totalRuns,
            Seed = spec.Seed,
            MageId = spec.MageId.IsValid ? spec.MageId.ToString() : "-",
            BotPolicyId = spec.Policy?.Id ?? "-",
            FocusSpellId = spec.FocusSpellId.IsValid ? spec.FocusSpellId.ToString() : "-",
            CurrentDay = result.Runs.LastOrDefault()?.DayReached ?? 0,
            CurrentRunSeconds = result.Runs.LastOrDefault()?.ElapsedSimulatedSeconds ?? 0f,
            TotalSimulatedSeconds = totalSimulatedSeconds,
            Defeats = result.Runs.Count(run => string.Equals(run.Outcome, "defeat", StringComparison.OrdinalIgnoreCase)),
            Victories = result.Runs.Count(run => string.Equals(run.Outcome, "victory", StringComparison.OrdinalIgnoreCase)),
            Timeouts = result.Runs.Count(run => string.Equals(run.Outcome, "timeout", StringComparison.OrdinalIgnoreCase)),
            AverageDayReached = completedRuns == 0 ? 0f : (float)result.Runs.Average(run => run.DayReached),
            AverageDamageOutput = completedRuns == 0 ? 0f : (float)result.Runs.Average(run => run.TotalDamageDealt),
            AverageDamageTaken = completedRuns == 0 ? 0f : (float)result.Runs.Average(run => run.PlayerDamageTaken),
            LastOutcome = lastOutcome
        };
    }

    private static IEnumerable<RunSpec> EnumerateRunSpecs(
        BalanceExperimentDefinition experiment,
        IReadOnlyList<ContentId> mageIds,
        IReadOnlyList<IBalanceBotPolicy> policies,
        IReadOnlyList<ContentId> focusSpellIds)
    {
        if (experiment.TotalSimulationRuns > 0)
        {
            var runCount = Math.Max(1, experiment.TotalSimulationRuns);
            var explicitSeeds = experiment.ExplicitSeeds;
            for (var index = 0; index < runCount; index++)
            {
                var seed = explicitSeeds.Count > 0
                    ? explicitSeeds[index % explicitSeeds.Count] + index / explicitSeeds.Count
                    : experiment.SeedStart + index;
                var focusSpellId = focusSpellIds.Count == 0
                    ? ContentId.From(string.Empty)
                    : focusSpellIds[index % focusSpellIds.Count];
                var policy = policies[(index / Math.Max(1, focusSpellIds.Count)) % policies.Count];
                var mageId = mageIds[(index / Math.Max(1, focusSpellIds.Count * policies.Count)) % mageIds.Count];
                yield return new RunSpec(seed, mageId, policy, focusSpellId);
            }

            yield break;
        }

        foreach (var seed in experiment.EnumerateSeeds())
        {
            foreach (var mageId in mageIds)
            {
                foreach (var policy in policies)
                {
                    yield return new RunSpec(seed, mageId, policy, ContentId.From(string.Empty));
                }
            }
        }
    }

    private static void ConfigureSpellLoadout(
        RunContext context,
        BalanceExperimentDefinition experiment,
        ContentId focusSpellId,
        int seed)
    {
        if (!focusSpellId.IsValid && experiment.SpellPool.Count == 0)
        {
            return;
        }

        var spells = ResolveFocusSpellIds(context.Content, experiment);
        if (spells.Count == 0)
        {
            return;
        }

        var selected = new List<ContentId>(TheLastMage.Gameplay.Spells.SpellbookState.MaxSlots);
        if (focusSpellId.IsValid && context.Content.Spells.ContainsKey(focusSpellId))
        {
            selected.Add(focusSpellId);
        }

        var offset = Math.Abs(seed) % spells.Count;
        for (var i = 0; i < spells.Count && selected.Count < TheLastMage.Gameplay.Spells.SpellbookState.MaxSlots; i++)
        {
            var spellId = spells[(i + offset) % spells.Count];
            if (!selected.Contains(spellId))
            {
                selected.Add(spellId);
            }
        }

        context.State.Spellbook.ClearSlots();
        for (var slotIndex = 0; slotIndex < selected.Count; slotIndex++)
        {
            context.State.Spellbook.AssignSlot(slotIndex, selected[slotIndex]);
        }

        context.State.Spellbook.SelectSlot(0);
    }

    private static ProfileSaveDto BuildExperimentProfile(
        ContentRegistry content,
        ProfileSaveDto sourceProfile,
        BalanceItemProfileMode mode)
    {
        var profile = new ProfileSaveDto
        {
            UnlockedMageIds = sourceProfile.UnlockedMageIds.ToList(),
            UnlockedSpellIds = sourceProfile.UnlockedSpellIds.ToList(),
            UnlockedItemIds = sourceProfile.UnlockedItemIds.ToList(),
            DiscoveredItemIds = sourceProfile.DiscoveredItemIds.ToList(),
            CompletedAchievementIds = sourceProfile.CompletedAchievementIds.ToList()
        };

        if (mode is BalanceItemProfileMode.AllProductionUnlocked or BalanceItemProfileMode.AllProductionUnlockedAndDiscovered)
        {
            foreach (var mageId in content.Mages.Keys)
            {
                AddUnique(profile.UnlockedMageIds, mageId.ToString());
            }

            foreach (var item in content.Items.Values.Where(item => !item.Id.ToString().StartsWith("regression_item_", StringComparison.Ordinal)))
            {
                AddUnique(profile.UnlockedItemIds, item.Id.ToString());
                if (mode == BalanceItemProfileMode.AllProductionUnlockedAndDiscovered)
                {
                    AddUnique(profile.DiscoveredItemIds, item.Id.ToString());
                }
            }
        }

        return profile;
    }

    private static void AddUnique(List<string> values, string value)
    {
        if (!values.Contains(value, StringComparer.Ordinal))
        {
            values.Add(value);
        }
    }

    private static float ReadPlayerHealth(RunContext context)
    {
        return context.TryGetSystem<CombatSystem>(out var combat)
            && combat != null
            && combat.TryGetHealth(context.State.Player.EntityId, out var health)
            && health != null
            ? health.CurrentHealth
            : context.State.Player.BaseMaxHealth;
    }

    private static void DetectOutliers(BalanceExperimentResult result)
    {
        foreach (var run in result.Runs)
        {
            if (run.ProcCapHits > 0 || run.FeedbackDrops > 0 || run.AllocationPeakBytes > 256_000)
            {
                result.Warnings.Add($"Outlier run `{run.RunId}` seed={run.Seed} policy={run.BotPolicyId} procCaps={run.ProcCapHits} feedbackDrops={run.FeedbackDrops} allocPeak={run.AllocationPeakBytes}.");
            }
        }

        foreach (var item in result.MonteCarlo.PolicySummaries.SelectMany(summary => summary.PickCounts))
        {
            var offerTotal = result.MonteCarlo.PolicySummaries.Sum(summary => summary.OfferCounts.GetValueOrDefault(item.Key));
            if (item.Value > 0 && offerTotal > 0 && item.Value / (float)offerTotal > 0.9f)
            {
                result.Warnings.Add($"Item `{item.Key}` has very high pick correlation ({item.Value}/{offerTotal}) in Monte Carlo policies.");
            }
        }
    }

    private static int StableHash(string value)
    {
        unchecked
        {
            var hash = 2166136261;
            foreach (var character in value)
            {
                hash ^= character;
                hash *= 16777619;
            }

            return (int)hash;
        }
    }
}
