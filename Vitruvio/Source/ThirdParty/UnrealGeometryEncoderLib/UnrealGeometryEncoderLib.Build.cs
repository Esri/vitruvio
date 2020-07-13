// Copyright 2019 - 2020 Esri. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class UnrealGeometryEncoderLib : ModuleRules
{
	public UnrealGeometryEncoderLib(ReadOnlyTargetRules Target) : base(Target)
	{
		bUseRTTI = true;
		bEnableExceptions = true;
		Type = ModuleType.External;

		string LibDir = Path.Combine(ModuleDirectory, "lib", "Win64", "Release");
		string IncludeDir = Path.Combine(ModuleDirectory, "include");
		string EncoderDllName = "UnrealGeometryEncoder.dll";

		RuntimeDependencies.Add(Path.Combine(LibDir, EncoderDllName));
		PublicDelayLoadDLLs.Add(EncoderDllName);

		PublicAdditionalLibraries.Add(Path.Combine(LibDir, "UnrealGeometryEncoder.lib"));

		PublicSystemIncludePaths.Add(IncludeDir);
	}
}
