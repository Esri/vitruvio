// Copyright 2019 - 2020 Esri. All Rights Reserved.

using System.IO;
using System;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Net;
using UnrealBuildTool;

public class PRT : ModuleRules
{
	private const bool Debug = true;

	// PRT version and toolchain (needs to be correct for download URL)
	private const int PrtMajor = 2;
	private const int PrtMinor = 1;
	private const int PrtBuild = 5705;

	private const String PrtPlatformWindows = "win10-vc141-x86_64-rel-opt";
	private const String PrtPlatformMacOS = "osx12-ac81-x86_64-rel-opt";


	public PRT(ReadOnlyTargetRules Target) : base(Target)
	{
		bUseRTTI = true;
		bEnableExceptions = true;
		Type = ModuleType.External;

		String PlatformDir;
		String PlatformLibPre;
		String PlatformLibExt;
		String PrtPlatform;
		IZipExtractor Extractor;
		if (Target.Platform == UnrealTargetPlatform.Win64) 
		{
			PlatformDir = "Win64";
			PlatformLibPre = "";
			PlatformLibExt = ".dll";
			PrtPlatform = PrtPlatformWindows;
			Extractor = new WindowsZipExtractor();
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PlatformDir = "Mac";
			PlatformLibPre = "lib";
			PlatformLibExt = ".dylib";
			PrtPlatform = PrtPlatformMacOS;
			Extractor = new UnixZipExtractor();
		}
		else 
		{
			throw new System.PlatformNotSupportedException();
		}

		string LibDir = Path.Combine(ModuleDirectory, "lib", PlatformDir, "Release");
		string BinDir = Path.Combine(ModuleDirectory, "bin", PlatformDir, "Release");
		string IncludeDir = Path.Combine(ModuleDirectory, "include");

		// TODO improve checking if already installed
		
		// 1. Check if prt is already available, otherwise download from official github repo
		bool PrtInstalled = Directory.Exists(LibDir) && Directory.Exists(BinDir);
		if (!PrtInstalled)
		{
			try
			{
				if (Directory.Exists(LibDir)) Directory.Delete(LibDir, true);
				if (Directory.Exists(BinDir)) Directory.Delete(BinDir, true);
				if (Directory.Exists(IncludeDir)) Directory.Delete(IncludeDir, true);


				if (Debug) Console.WriteLine("PRT not found");

				string PrtUrl = "https://github.com/Esri/esri-cityengine-sdk/releases/download";
				string PrtVersion = string.Format("{0}.{1}.{2}", PrtMajor, PrtMinor, PrtBuild);

				string PrtLibName = string.Format("esri_ce_sdk-{0}-{1}", PrtVersion, PrtPlatform);
				string PrtLibZipFile = PrtLibName + ".zip";
				string PrtDownloadUrl = Path.Combine(PrtUrl, PrtVersion, PrtLibZipFile);

				if (Debug) System.Console.WriteLine("Downloading " + PrtDownloadUrl + "...");

				using (var Client = new WebClient())
				{
					Client.DownloadFile(PrtDownloadUrl, Path.Combine(ModuleDirectory, PrtLibZipFile));
				}

				if (Debug) System.Console.WriteLine("Extracting " + PrtLibZipFile + "...");

				Extractor.Unzip(ModuleDirectory, PrtLibZipFile, PrtLibName);

				Directory.CreateDirectory(LibDir);
				Directory.CreateDirectory(BinDir);
				Copy(Path.Combine(ModuleDirectory, PrtLibName, "lib"), Path.Combine(ModuleDirectory, LibDir));
				Copy(Path.Combine(ModuleDirectory, PrtLibName, "bin"), Path.Combine(ModuleDirectory, BinDir));
				Copy(Path.Combine(ModuleDirectory, PrtLibName, "include"), Path.Combine(ModuleDirectory, "include"));

				Directory.Delete(Path.Combine(ModuleDirectory, PrtLibName), true);
				File.Delete(Path.Combine(ModuleDirectory, PrtLibZipFile));
			}
			finally
			{
				// TODO cleanup
			}
		} 
		else if (Debug)
		{
			Console.WriteLine("PRT found");
		}

		// 2. Copy libraries to module binaries directory and add dependencies
		string ModuleBinariesDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../..", "Binaries", PlatformDir));

		if (Debug)
		{
			System.Console.WriteLine("PRT Source Lib Dir: " + LibDir);
			System.Console.WriteLine("PRT Source Bin Dir: " + BinDir);
			System.Console.WriteLine("PRT Source Include Dir: " + IncludeDir);
			System.Console.WriteLine("Module Binaries Dir: " + ModuleBinariesDir);
		}

		Directory.CreateDirectory(ModuleBinariesDir);
		PublicRuntimeLibraryPaths.Add(ModuleBinariesDir);

		// Copy PRT core libraries
		// see for example https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Plugins/Runtime/LeapMotion/Source/LeapMotion/LeapMotion.Build.cs
		if (Debug) Console.WriteLine("Adding PRT core libraries to binary directory " + ModuleBinariesDir);
		foreach (string FilePath in Directory.GetFiles(BinDir))
		{
			string FileName = Path.GetFileName(FilePath);
			string LibraryPath = Path.Combine(ModuleBinariesDir, FileName);

			if (Path.GetExtension(FilePath) == PlatformLibExt)
			{
				if (Debug) Console.WriteLine("Adding Runtime Dpendency " + FileName);

				CopyLibraryFile(LibDir, FilePath, LibraryPath);

				RuntimeDependencies.Add(LibraryPath);
				PublicDelayLoadDLLs.Add(FileName);
			}

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				if (Path.GetExtension(FilePath) == ".lib")
				{
					if (Debug) Console.WriteLine("Adding Public Additional Library" + FileName);

					CopyLibraryFile(LibDir, FilePath, LibraryPath);
					PublicAdditionalLibraries.Add(LibraryPath);
				}
			}
		}

		// Copy PRT extension libraries
		if (Debug) Console.WriteLine("Adding PRT core libraries to binary directory " + ModuleBinariesDir);
		foreach (string FilePath in Directory.GetFiles(LibDir))
		{
			if (Path.GetExtension(FilePath) == PlatformLibExt)
			{
				string FileName = Path.GetFileName(FilePath);
				string DllPath = Path.Combine(ModuleBinariesDir, FileName);
				if (Debug) Console.WriteLine("Adding " + FileName);
				CopyLibraryFile(LibDir, FilePath, DllPath);
				RuntimeDependencies.Add(DllPath);
			}
		}

		// Add include search path
		if (Debug) Console.WriteLine("Adding include search path " + IncludeDir);
		PublicSystemIncludePaths.Add(IncludeDir);
	}

	private static void CopyLibraryFile(string SrcLibDir, string SrcFile, string DstLibDir)
	{
		string SrcLib = Path.Combine(SrcLibDir, SrcFile);
		if (!System.IO.File.Exists(DstLibDir) || System.IO.File.GetCreationTime(SrcLib) > System.IO.File.GetCreationTime(DstLibDir))
		{
			System.IO.File.Copy(SrcLib, DstLibDir, true);
		}
	}

	void Copy(string SrcDir, string DstDir)
	{
		Directory.CreateDirectory(DstDir);

		foreach (var CopyFile in Directory.GetFiles(SrcDir))
		{
			File.Copy(CopyFile, Path.Combine(DstDir, Path.GetFileName(CopyFile)));
		}

		foreach (var Dir in Directory.GetDirectories(SrcDir))
		{
			Copy(Dir, Path.Combine(DstDir, Path.GetFileName(Dir)));
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

	private class UnixZipExtractor : IZipExtractor
	{
		public void Unzip(string WorkingDir, string ZipFile, string Destination)
		{
			var Arguments = string.Format("-q {0} -d {1}", ZipFile, Destination);

			ProcessStartInfo ProcStartInfo = new System.Diagnostics.ProcessStartInfo("unzip", Arguments)
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
