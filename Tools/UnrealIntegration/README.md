# World Mechanics — Unreal integration tools

The current repository keeps portable C++ algorithms under `Modules/` for CMake build/tests. To bring them into Unreal Engine 5.8 incrementally, run `Export-Phase.ps1` once per phase. The script builds a cumulative `Plugins/WorldMechanics` plugin with one runtime module, so public engine-neutral C++ classes stay in one module instead of requiring Unreal module export macros across DLL boundaries.

See [the full import guide](../../Docs/UNREAL_INCREMENTAL_INTEGRATION.md) and [phase manifest](phase-manifest.json).

Example (run from the repository checkout):

```powershell
.\Tools\UnrealIntegration\Export-Phase.ps1 -Phase 0 -ProjectRoot "D:\Games\MyProject2"
```

After each phase, build the Unreal Engine 5.8 Editor target and check the resulting errors before running the next phase. The package does not claim that engine-facing adapters have been implemented or validated.
