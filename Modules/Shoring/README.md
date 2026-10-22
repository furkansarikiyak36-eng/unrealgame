# Shoring

**Responsibility:** Plan and validate temporary/permanent support around an excavation.

Owns support-piece definitions, placement constraints, layout planner, spacing/grid policy, clearance checks, coverage and unresolved-risk reports. Actor spawning is an adapter concern.

**Rules:** Planner output is inspectable and deterministic. A proposed piece is not installed until placement succeeds. Spacing rules are explicit per piece type. StructuralIntegrity consumes support state through a contract.

**Tests:** Spacing, boundary fit, obstruction, incomplete coverage, invalid dimensions and plan-to-placement consistency.

**Status:** Design scaffold only.
