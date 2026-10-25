// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WorldSnap : ModuleRules
{
	public WorldSnap(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
	}
}
