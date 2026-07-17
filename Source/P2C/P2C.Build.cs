// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class P2C : ModuleRules
{
	public P2C(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"P2C",
			"P2C/Variant_Platforming",
			"P2C/Variant_Platforming/Animation",
			"P2C/Variant_Combat",
			"P2C/Variant_Combat/AI",
			"P2C/Variant_Combat/Animation",
			"P2C/Variant_Combat/Gameplay",
			"P2C/Variant_Combat/Interfaces",
			"P2C/Variant_Combat/UI",
			"P2C/Variant_SideScrolling",
			"P2C/Variant_SideScrolling/AI",
			"P2C/Variant_SideScrolling/Gameplay",
			"P2C/Variant_SideScrolling/Interfaces",
			"P2C/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
