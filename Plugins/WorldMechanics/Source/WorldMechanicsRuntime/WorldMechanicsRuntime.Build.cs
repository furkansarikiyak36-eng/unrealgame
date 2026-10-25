using UnrealBuildTool;

public class WorldMechanicsRuntime : ModuleRules
{
    public WorldMechanicsRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(new string[] { "Core" });
    }
}