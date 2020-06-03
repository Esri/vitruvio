// Copyright 2019 - 2020 Esri. All Rights Reserved.

using System.IO;
using System;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Net;
using UnrealBuildTool;
using System.ComponentModel.Design;

public class PRT : ModuleRules
{
	private const bool Debug = true;

	// PRT version and toolchain (needs to be correct for download URL)
	private const int PrtMajor = 2;
	private const int PrtMinor = 1;
	private const int PrtBuild = 5705;

	public PRT(ReadOnlyTargetRules Target) : base(Target)
	{
		bUseRTTI = true;
		bEnableExceptions = true;
		Type = ModuleType.External;

		AbstractPlatform Platform;
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			Platform = new WindowsPlatform();
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			Platform = new MacPlatform();
		}
		else
		{
			throw new System.PlatformNotSupportedException();
		}

		string LibDir = Path.Combine(ModuleDirectory, "lib", Platform.Name, "Release");
		string BinDir = Path.Combine(ModuleDirectory, "bin", Platform.Name, "Release");
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

				string PrtLibName = string.Format("esri_ce_sdk-{0}-{1}", PrtVersion, Platform.PrtPlatform);
				string PrtLibZipFile = PrtLibName + ".zip";
				string PrtDownloadUrl = Path.Combine(PrtUrl, PrtVersion, PrtLibZipFile);

				if (Debug) System.Console.WriteLine("Downloading " + PrtDownloadUrl + "...");

				using (var Client = new WebClient())
				{
					Client.DownloadFile(PrtDownloadUrl, Path.Combine(ModuleDirectory, PrtLibZipFile));
				}

				if (Debug) System.Console.WriteLine("Extracting " + PrtLibZipFile + "...");

				Platform.ZipExtractor.Unzip(ModuleDirectory, PrtLibZipFile, PrtLibName);

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
		string ModuleBinariesDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../..", "Binaries", Platform.Name));

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
		if (Debug) Console.WriteLine("Adding PRT core libraries to binary directory " + ModuleBinariesDir);
		foreach (string FilePath in Directory.GetFiles(BinDir))
		{
			string FileName = Path.GetFileName(FilePath);
			string LibraryPath = Path.Combine(ModuleBinariesDir, FileName);

			Platform.CopyPrtCoreLibrary(FilePath, FileName, LibraryPath, LibDir, this);
		}

		// Copy PRT extension libraries
		if (Debug) Console.WriteLine("Adding PRT extension libraries to binary directory " + ModuleBinariesDir);
		foreach (string FilePath in Directory.GetFiles(LibDir))
		{
			if (Path.GetExtension(FilePath) == Platform.DynamicLibExtension)
			{
				string FileName = Path.GetFileName(FilePath);
				string DllPath = Path.Combine(ModuleBinariesDir, FileName);
				if (Debug) Console.WriteLine("Adding Extension Library " + FileName);
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
			// For some reason File.Copy does not always preserve the creation time so we set it manually
			DateTime CreationTime = System.IO.File.GetCreationTime(SrcLib);
			System.IO.File.Copy(SrcLib, DstLibDir, true);
			System.IO.File.SetCreationTime(DstLibDir, CreationTime);
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

	private abstract class AbstractZipExtractor
	{
		public void Unzip(string WorkingDir, string ZipFile, string Destination)
		{
			var ExpandedArguments = string.Format(Arguments, ZipFile, Destination);

			ProcessStartInfo ProcStartInfo = new System.Diagnostics.ProcessStartInfo(Command, ExpandedArguments)
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

		public abstract string Command { get; }
		public abstract string Arguments { get; }
	}

	private class WindowsZipExtractor : AbstractZipExtractor
	{
		public override string Command { get { return "cmd"; } }

		public override string Arguments
		{
			get
			{
				return "/c PowerShell -Command \" & Expand-Archive -Path {0} -DestinationPath {1}\"";
			}
		}
	}

	private class UnixZipExtractor : AbstractZipExtractor
	{
		public override string Command { get { return "unzip"; } }

		public override string Arguments
		{
			get
			{
				return "-q {0} -d {1}";
			}
		}
	}

	private abstract class AbstractPlatform
	{
		public abstract AbstractZipExtractor ZipExtractor { get; }

		public abstract string Name { get; }
		public abstract string PrtPlatform { get; }
		public abstract string DynamicLibExtension { get; }

		public virtual void CopyPrtCoreLibrary(string FilePath, string FileName, string LibraryPath, string LibDir, ModuleRules Rules)
		{
			if (Path.GetExtension(FilePath) == DynamicLibExtension)
			{
				if (Debug) Console.WriteLine("Adding Runtime Dpendency " + FileName);

				CopyLibraryFile(LibDir, FilePath, LibraryPath);

				Rules.RuntimeDependencies.Add(LibraryPath);
				Rules.PublicDelayLoadDLLs.Add(FileName);
			}
		}
	}

	private class WindowsPlatform : AbstractPlatform
	{
		public override AbstractZipExtractor ZipExtractor { get { return new WindowsZipExtractor(); } }

		public override string Name { get { return "Win64"; } }
		public override string PrtPlatform { get { return "win10-vc141-x86_64-rel-opt"; } }
		public override string DynamicLibExtension { get { return ".dll"; } }

		public override void CopyPrtCoreLibrary(string FilePath, string FileName, string LibraryPath, string LibDir, ModuleRules Rules)
		{
			base.CopyPrtCoreLibrary(FilePath, FileName, LibraryPath, LibDir, Rules);

			if (Path.GetExtension(FilePath) == ".lib")
			{
				if (Debug) Console.WriteLine("Adding Public Additional Library " + FileName);

				CopyLibraryFile(LibDir, FilePath, LibraryPath);

				Rules.PublicAdditionalLibraries.Add(LibraryPath);
			}
		}
	}

	private class MacPlatform : AbstractPlatform
	{
		public override AbstractZipExtractor ZipExtractor {	get { return new UnixZipExtractor(); } }

		public override string Name { get { return "Mac"; } }
		public override string PrtPlatform { get { return "osx12-ac81-x86_64-rel-opt"; } }
		public override string DynamicLibExtension { get { return ".dylib"; } }
	}
}
