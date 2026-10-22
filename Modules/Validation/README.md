# Validation

**Responsibility:** Verify module contracts and cross-module scenarios through repeatable tests.

Owns test taxonomy, shared invariants, regression gates, performance budgets, determinism checks and readiness criteria for Unreal adapter work.

**Scenarios:** scan → classify → excavate; terrain edit → support replan; support removal → structural assessment → collapse event; depth/environment change → exposure update; save → mutate → restore; injected restore failure → unchanged live state.

**Rules:** Test names listed in a runner are not evidence of execution. Record commands, environment, pass/fail counts and failures. Unreal build/PIE is a later adapter gate.

**Status:** Design scaffold only.
