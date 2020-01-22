// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class PRTLibrary : ModuleRules
{
	public PRTLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
        bUseRTTI = true;
        Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add the import library
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Release", "com.esri.prt.core.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Release","glutess.lib"));

            RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Binaries/ThirdParty/com.esri.prt.core.dll"));
            RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Binaries/ThirdParty/glutess.dll"));
            RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Binaries/ThirdParty/com.esri.prt.adaptors.dll"));
            RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Binaries/ThirdParty/com.esri.prt.alembic.dll"));
            RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Binaries/ThirdParty/com.esri.prt.codecs.dll"));
            RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Binaries/ThirdParty/com.esri.prt.unreal.dll"));
            RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Binaries/ThirdParty/DatasmithSDK.dll"));
            RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Binaries/ThirdParty/FreeImage317.dll"));
            RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Binaries/ThirdParty/LibRaw.dll"));
            RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Binaries/ThirdParty/VueExport.dll"));
			
            PublicDelayLoadDLLs.Add("com.esri.prt.core.dll");
            PublicDelayLoadDLLs.Add("glutess.dll");
        }
        /*
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            //Left as an example:
            //PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "Mac", "Release", "libExampleLibrary.dylib"));
        }
        */
    }
}
