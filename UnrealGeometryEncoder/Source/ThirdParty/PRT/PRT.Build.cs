using System.IO;
using System;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Net;
using UnrealBuildTool;

public class PRT : ModuleRules
{
	private const bool Debug = true;

	private const int PrtMajor = 2;
	private const int PrtMinor = 1;
	private const int PrtBuild = 5705;

	public PRT(ReadOnlyTargetRules Target) : base(Target)
	{
		bUseRTTI = true;
		bEnableExceptions = true;
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string LibFolder = Path.Combine(ModuleDirectory, "lib", "Win64", "Release");
			string BinFolder = Path.Combine(ModuleDirectory, "bin", "Win64", "Release");

			// TODO improve checking if already installed
			// 1. Check if prt is already available, otherwise download from official github repo
			bool PrtInstalled = Directory.Exists(LibFolder) && Directory.Exists(BinFolder);
			if (!PrtInstalled)
			{
				try
				{
					if (Directory.Exists(LibFolder)) Directory.Delete(LibFolder, true);
					if (Directory.Exists(BinFolder)) Directory.Delete(BinFolder, true);

					if (Debug) Console.WriteLine("PRT not found");

					string PrtBuildUrl = "https://github.com/Esri/esri-cityengine-sdk/releases/download";
					string Version = string.Format("{0}.{1}.{2}", PrtMajor, PrtMinor, PrtBuild);

					string Platform = "win10-vc141-x86_64-rel-opt";
					string LibName = string.Format("esri_ce_sdk-{0}-{1}", Version, Platform);
					string LibZipFile = LibName + ".zip";
					string PrtLib = Path.Combine(PrtBuildUrl, Version, LibZipFile);

					if (Debug) System.Console.WriteLine("Downloading " + PrtLib + "...");

					using (var Client = new WebClient())
					{
						Client.DownloadFile(PrtLib, Path.Combine(ModuleDirectory, LibZipFile));
					}

					if (Debug) System.Console.WriteLine("Extracting " + LibZipFile + "...");

					IZipExtractor Extractor = new WindowsZipExtractor();
					Extractor.Unzip(ModuleDirectory, LibZipFile, LibName);

					Directory.CreateDirectory(LibFolder);
					Directory.CreateDirectory(BinFolder);
					Copy(Path.Combine(ModuleDirectory, LibName, "lib"), Path.Combine(ModuleDirectory, LibFolder));
					Copy(Path.Combine(ModuleDirectory, LibName, "bin"), Path.Combine(ModuleDirectory, BinFolder));
					Copy(Path.Combine(ModuleDirectory, LibName, "include"), Path.Combine(ModuleDirectory, "include"));

					Directory.Delete(Path.Combine(ModuleDirectory, LibName), true);
					File.Delete(Path.Combine(ModuleDirectory, LibZipFile));
				}
				finally
				{
					// TODO cleanup
				}
			}

			// 2. Add necessary includes
			PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));

			string BinariesFolder = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../..", "Binaries", "Win64"));

			PublicRuntimeLibraryPaths.Add(BinariesFolder);

			Directory.CreateDirectory(BinariesFolder);

			if (Debug)
			{
				System.Console.WriteLine("LibFolder: " + LibFolder);
				System.Console.WriteLine("BinFolder: " + BinFolder);
				System.Console.WriteLine("ThirdPartyBinFolder: " + BinariesFolder);
			}

			// Add the import library
			PublicAdditionalLibraries.Add(Path.Combine(BinFolder, "com.esri.prt.core.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(BinFolder, "glutess.lib"));

			PublicDelayLoadDLLs.Add("com.esri.prt.core.dll");
			PublicDelayLoadDLLs.Add("glutess.dll");

			// 3. Add libraries to Binaries folder
			// see for example https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Plugins/Runtime/LeapMotion/Source/LeapMotion/LeapMotion.Build.cs
			foreach (string BinFile in Directory.GetFiles(BinFolder))
			{
				if (Path.GetExtension(BinFile) == ".dll")
				{
					string BinaryDllPath = Path.Combine(BinariesFolder, Path.GetFileName(BinFile));
					if (Debug) System.Console.WriteLine("Copy \"" + Path.GetFileName(BinFile) + "\" to " + BinaryDllPath);
					File.Copy(Path.Combine(LibFolder, BinFile), BinaryDllPath, true);

					RuntimeDependencies.Add(BinaryDllPath);
				}
			}

			// 4. Add prt extension libraries to Binaries/lib folder
			Directory.CreateDirectory(Path.Combine(BinariesFolder, "lib"));
			foreach (string LibFile in Directory.GetFiles(LibFolder))
			{
				if (Path.GetExtension(LibFile) == ".dll")
				{
					string BinaryDllPath = Path.Combine(BinariesFolder, "lib", Path.GetFileName(LibFile));
					if (Debug) System.Console.WriteLine("Copy \"" + Path.GetFileName(LibFile) + "\" to " + BinaryDllPath);
					File.Copy(Path.Combine(LibFolder, LibFile), BinaryDllPath, true);
				}
			}
		}
	}

	void Copy(string SourceDir, string TargetDir)
	{
		Directory.CreateDirectory(TargetDir);

		foreach (var CopyFile in Directory.GetFiles(SourceDir))
		{
			File.Copy(CopyFile, Path.Combine(TargetDir, Path.GetFileName(CopyFile)));
		}

		foreach (var Dir in Directory.GetDirectories(SourceDir))
		{
			Copy(Dir, Path.Combine(TargetDir, Path.GetFileName(Dir)));
		}
	}

	private interface IZipExtractor
	{
		void Unzip(string WorkingDir, string ZipFile, string Destination);
	}

	private class WindowsZipExtractor : IZipExtractor
	{
		public void Unzip(string WorkingDir, string ZipFile, string Destination)
		{
			var Command = string.Format("PowerShell -Command \" & Expand-Archive -Path {0} -DestinationPath {1}\"", ZipFile, Destination);

			ProcessStartInfo ProcStartInfo = new System.Diagnostics.ProcessStartInfo("cmd", "/c " + Command)
			{
				WorkingDirectory = WorkingDir,
				UseShellExecute = false,
				CreateNoWindow = true
			};

			System.Diagnostics.Process UnzipProcess = new System.Diagnostics.Process
			{
				StartInfo = ProcStartInfo, 
				EnableRaisingEvents = true
			};
			UnzipProcess.Start();
			UnzipProcess.WaitForExit();
		}
	}
}