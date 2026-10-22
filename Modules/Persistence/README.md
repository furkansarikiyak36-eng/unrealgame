# Persistence

**Responsibility:** Capture and restore validated system state atomically.

Owns versioned module records, schema migration policy, snapshot validation, staging, commit/rollback, stable IDs and cross-module reference validation. Serialization and Unreal SaveGame objects are adapters.

**Rules:** Validate before mutating live state. Stage restore and commit only after required records pass. On failure preserve pre-load state. Unsupported state must fail clearly or use a documented fallback; never silently drop data.

**Tests:** Round-trip, unknown schema, migration, missing references, partial-record rejection, rollback and legacy compatibility.

**Status:** Design scaffold only. Existing Unreal persistence code remains in Source and is not yet refactored into this module.
