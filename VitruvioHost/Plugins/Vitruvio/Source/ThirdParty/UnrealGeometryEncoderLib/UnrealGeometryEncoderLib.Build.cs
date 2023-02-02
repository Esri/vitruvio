/* Copyright 2023 Esri
 *
 * Licensed under the Apache License Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
