// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class MyProject2 : ModuleRules
{
	public MyProject2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false; // many package files define same-named helpers in anonymous namespaces
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "CableComponent", "GeometryScriptingCore", "GeometryFramework", "PhysicsCore", "AIModule", "GameplayTasks", "NavigationSystem", "DeveloperSettings", "ProceduralMeshComponent", "WorldSnap" });

		PrivateDependencyModuleNames.AddRange(new string[] { "RenderCore", "RHI" });

		// Imported package folders use unprefixed includes ("LogTypes.h"), so each sub-folder is an include root.
		PublicIncludePaths.Add(ModuleDirectory);
		foreach (string Dir in Directory.GetDirectories(ModuleDirectory))
		{
			if (Path.GetFileName(Dir) != "Wood")
			{
				PublicIncludePaths.Add(Dir);
			}
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
