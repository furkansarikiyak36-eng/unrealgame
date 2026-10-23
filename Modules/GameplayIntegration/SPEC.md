# GameplayIntegration — Specification

## Responsibility

This module owns deterministic, bounded gameplay decisions and preflight plans that can be tested without Unreal Engine. It is not an Actor factory and does not mutate the actual game world. The Unreal adapter must revalidate live world/inventory revisions and commit actual changes.

## Raid target scoring

Each candidate carries the same world revision and normalized features: threat, accessibility, strategic value and defensive exposure. A validated weight set computes weighted tactical utility and subtracts a bounded normalized distance penalty. Dead, invisible, unreachable and stale candidates are excluded. Exact score ties use the lower stable target ID. Malformed features, duplicate IDs and excessive candidate sets fail explicitly.

The scorer is a deterministic utility model, not AI perception or pathfinding. The caller remains responsible for feature measurement and current-target validation.

## Spawn decision pipeline

Candidates are deterministically sorted by descending utility then ascending stable candidate ID. Each attempted candidate must have a matching world revision, navigation evidence, region permission and an empty overlap result. The result uses typed dispositions: approved, no candidates, attempt budget exhausted, navigation unavailable, or all candidates blocked.

The pipeline returns an authorization/preflight decision only. It does not spawn a UObject/Actor and cannot guarantee the candidate stays clear; the Unreal adapter must revalidate immediately before spawn. Attempts are bounded by both request and caller budget.

## Chunk work scheduling

The scheduler holds bounded per-chunk queues and services chunks round-robin, one job at a time, so a single busy chunk cannot monopolize the queue. Work carries source revision and estimated cost. Stale jobs are discarded; jobs from a future/unpublished revision and jobs that do not fit the remaining per-drain cost are deferred. Same-kind work for one chunk is coalesced only when the incoming revision is strictly newer. Examination count, queue size, per-job cost, per-drain cost and dispatch count are capped.

Scheduling does not mark authoritative world state dirty, execute jobs or publish derived products. The consumer must perform the work and use the existing source-revision gate before publishing mesh/collision/navigation output. The portable scheduler uses deterministic ordered chunk keys; production scheduling may add spatial locality after fairness is measured.

## M1–M6 material rules

The resolver supports Global, Definition, Composite and Instance scopes. Resolution precedence is Instance > Composite > Definition > Global; within a scope, higher explicit priority wins, then lower rule ID. A rule also has an inclusive observed-value interval and a typed action (fire, water, impact, weather or time).

Candidate IDs for a material/action pair are cached. Upsert/remove increments the registry revision and invalidates the cache atomically. Duplicate rule IDs represent a replacement of that rule, not an additional ambiguous rule. A missing matching rule returns a typed NotFound result. The effects are a result snapshot, not a direct durability/heat mutation.

This unifies rule lookup across M1–M6 without replacing the existing UE material definitions, material instance state, composite layers, plaster/metal logic, or M6 persistence/chunk adapters.

## Plant growth and animal care

The portable plant model evaluates bounded substeps and temperature/light/water/N-P-K response. It consumes no more than the supplied resources, tracks age and growth, increments revision once per committed update and flags the maturity transition. It does not grow every plant in a world or own crop actors.

Animal care advances growth/health, feed/water consumption, production and reproductive readiness with bounded substeps. Reproduction planning groups eligible mature/healthy/ready animals by species and chooses stable ID-ordered female/male pairs. Pair output is independent of caller input order. It does not spawn offspring actors or mutate feed stores globally; those operations require world/inventory transactions.

## Crafting transaction planning

Crafting preflight validates the source inventory, ingredient counts, output definitions, stack metadata, slot capacity and weight limit. It stages a complete replacement slot array in private memory, debits ingredients, fills existing stacks, adds sorted output stacks and returns a plan only when every check passes.

The plan stores the source inventory revision plus a fingerprint of the full preflight snapshot. Revalidation rejects a stale revision or same-revision content drift. The caller must stage any world output actors first, then revalidate and replace the inventory once; failure must leave the live inventory unchanged. This module does not spawn world drops or commit Unreal inventory components.

## Status

Portable algorithms and regressions are present. This module is not Unreal-adapted or Unreal-verified. Engine navigation/overlap queries, actor spawning, live raid telemetry, chunk worker execution, actual material state application, plant/animal actors and transactional inventory replacement remain integration work.
