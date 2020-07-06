// Copyright 2019 - 2020 Esri. All Rights Reserved.

using UnrealBuildTool;

public class Vitruvio : ModuleRules
{
	public Vitruvio(ReadOnlyTargetRules Target) : base(Target)
	{
		bUseRTTI = true;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrecompileForTargets = PrecompileTargetsType.Any;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"RHI",
				"RenderCore",
				"PRT",
				"Projects",
				"SlateCore",
				"Slate",
				"AppFramework",
				"CoreUObject",
				"InputCore",
				"MeshDescription",
				"StaticMeshDescription",
				"ImageWrapper",
				"HTTP",
				"zlib"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"SlateCore",
				"Slate",
				"AppFramework",
				"UnrealGeometryEncoder"
			}
		);
	}
}
