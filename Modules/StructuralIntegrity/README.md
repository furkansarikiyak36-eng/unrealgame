# StructuralIntegrity

**Responsibility:** Evaluate support relationships and determine damage, instability and collapse consequences.

Owns structural nodes/edges, connection state, support/load evaluation, damage propagation, collapse eligibility and events. Material strength is provided through definitions rather than hard-coded actor classes.

**Rules:** Separate analysis from physical destruction. Graph changes invalidate affected analyses. Numerical assumptions and thresholds are documented and testable. Physics may enact a result but does not silently replace authoritative rules.

**Tests:** Disconnected graph, support loss, load threshold, cascading failure, cycle handling and deterministic results.

**Status:** Design scaffold only.
