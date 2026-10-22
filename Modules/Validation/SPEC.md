# Validation — Specification

## Responsibility
Own the evidence process and cross-module scenarios, not gameplay mechanics.

## Test levels
1. Shared data/contract tests without an engine.
2. Module unit tests with fake providers/adapters.
3. Cross-module tests for revision, event and transaction semantics.
4. Determinism/performance tests with bounded workloads.
5. Later: Unreal UBT/UHT build, Automation, Editor/PIE and target-hardware profiling.

## Failure discipline
- The runner exits non-zero if build or a required test fails.
- Expected test-name registration is not evidence of execution.
- Every real run records revision, date/time, compiler/engine version, command, output log, pass/fail/skip counts and known limits.
- Brace or source consistency checks are reported as static checks, not runtime verification.

## Mandatory scenarios
- Scan/classification denies dig on Unknown and StaticGeometry.
- Stale dig revision is rejected without changing density.
- Valid dig changes authoritative density, dirties expected chunks and advances revision.
- Stale mesh job is rejected.
- Snap preview remains committable only when all source revisions remain current.
- Shoring plan is deterministic and missing pieces remain visible as incomplete.
- Removing a support recomputes affected graph and emits no duplicate collapse.
- Survival consumes only valid/fresh environment samples.
- Save/reload reproduces canonical supported state.
- Injected restore/spawn/commit failure preserves pre-load state.
- Restored support is not active before geometry publication and graph revalidation.

## Gates
Specified → Implemented → Unit-tested → Cross-module-verified → Unreal-adapted → Unreal-verified. Every transition requires evidence; a blocked gate is reported rather than skipped.


## Status
Seven portable test groups and CMake/CTest/CI configuration are present; they remain unverified until a real run passes. Broader property/stress testing and UE 5.8 gates remain.
