# CoreContracts

**Responsibility:** Shared engine-neutral types and rules that prevent modules from inventing incompatible meanings.

Owns stable entity/region IDs, explicit units and coordinate conventions, result/error categories, validation outcomes, versioned data contracts and shared invariants.

Does not own gameplay policy, Unreal reflection/macros, actors, rendering, collision queries or file I/O.

**Contracts to define:** EntityId, RegionId, ModuleResult, ValidationIssue, transform/unit conventions and state-transition results.

**Required tests:** ID stability, invalid input rejection, conversions and deterministic serialization representation.

**Status:** Design scaffold only; no implementation or passing-test claim.
