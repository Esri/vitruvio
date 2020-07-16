// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealGeometryEncoderTarget : TargetRules
{
	public UnrealGeometryEncoderTarget(TargetInfo Target)
		: base(Target)
	{
		Type = TargetType.Program;
		SolutionDirectory = "Programs";

		LaunchModuleName = "UnrealGeometryEncoder";
		ExeBinariesSubFolder = "UnrealGeometryEncoder";

		LinkType = TargetLinkType.Monolithic;
		bShouldCompileAsDLL = true;

		bBuildDeveloperTools = false;
		bUseMallocProfiler = false;
		bBuildWithEditorOnlyData = true;
		bCompileAgainstEngine = false;
		bCompileAgainstCoreUObject = false;
		bCompileAgainstApplicationCore = false;
		bCompileICU = false;
		bUsesSlate = false;
		bDisableDebugInfo = true;
		bUsePDBFiles = true;
		bHasExports = true;
		bIsBuildingConsoleApplication = true;
	}
}
