// Copyright 2019 - 2020 Esri. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Vitruvio : ModuleRules
{
	public Vitruvio(ReadOnlyTargetRules Target) : base(Target)
	{
		bUseRTTI = true;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrecompileForTargets = PrecompileTargetsType.Any;

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));

		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private", "Util"));

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"RHI",
				"RenderCore",
				"Projects",
				"SlateCore",
				"Slate",
				"AppFramework",
				"CoreUObject",
				"InputCore",
				"MeshDescription",
				"StaticMeshDescription",
				"ImageWrapper",
				"ImageCore",
				"PRT",
				"UnrealGeometryEncoderLib"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"SlateCore",
				"Slate",
				"AppFramework",
			}
		);
	}
}
