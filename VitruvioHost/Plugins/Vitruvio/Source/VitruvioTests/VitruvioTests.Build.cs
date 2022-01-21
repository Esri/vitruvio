// Copyright 2017-2020 Esri R&D Center Zurich. All Rights Reserved.

using UnrealBuildTool;

public class VitruvioTests : ModuleRules
{
	public VitruvioTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		bAddDefaultIncludePaths = true;
		PublicDependencyModuleNames.AddRange(new []
		{
			"Core", 
			"CoreUObject", 
			"Engine",
			"Vitruvio"
		});
		
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
