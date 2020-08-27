// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

using System.IO;
using System;
using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
	public class UnrealGeometryEncoder : ModuleRules
	{

		public UnrealGeometryEncoder(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"PRT",
			});

			bUseRTTI = true;
			bEnableExceptions = true;
		}
	}
}
