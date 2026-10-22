# Mechanics Architecture

## Intent
The nine development phases are implementation history. The target deliverable is one coherent system organized by mechanic responsibility.

## Boundaries
- Modules/: engine-neutral contracts, rules, state models and tests as developed.
- Adapters/UnrealEngine/: future translation to Unreal actors, components, subsystems, reflection, physics, assets and lifecycle.
- Source/MyProject2/: existing Unreal project code. It remains in place until module contracts and migration are validated.

This architecture scaffold does not claim existing Unreal source has been extracted or made engine-independent.

## Principles
1. One owner per rule/state.
2. Explicit contracts between modules.
3. One-way documented dependency direction.
4. Domain state is authoritative; rendering and physics are adapters.
5. Invalid/unsupported state produces explicit errors.
6. Cross-module behavior is proven by scenario tests.
7. Never report integration without recorded evidence.

## Maturity gates
- Designed: responsibilities, I/O, dependencies and invariants documented.
- Implemented: source exists and conforms to contracts.
- Unit-tested: tests actually executed and results recorded.
- Cross-module verified: scenarios pass across module boundaries.
- Unreal-adapted: adapter source exists and builds in target UE version.
- Unreal-verified: relevant Automation/PIE scenarios actually run.

A module cannot skip a gate by renaming a phase or listing test names.
