// Copyright © 2017-2020 Esri R&D Center Zurich. All rights reserved.

using System.IO;
using System;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Net;
using UnrealBuildTool;
using System.ComponentModel.Design;
using System.Collections.Generic;
using System.Linq;

public class PRT : ModuleRules
{
	private readonly bool Debug;
	
	// PRT version and toolchain (needs to be correct for download URL)
	private const int PrtMajor = 2;
	private const int PrtMinor = 4;
	private const int PrtBuild = 7316;

	private const string PrtCoreDllName = "com.esri.prt.core.dll";

	private static readonly List<string> FilteredExtensionLibraries = new List<string>() { "DatasmithSDK.dll", "FreeImage317.dll", "com.esri.prt.unreal.dll" };

	private const long ErrorSharingViolation = 0x20;
	private const long ErrorLockViolation = 0x21;

	public PRT(ReadOnlyTargetRules Target) : base(Target)
	{
		// Debug print only enabled when plugin is installed directly into project (not in Engine)
		Debug = !PluginDirectory.EndsWith(Path.Combine("Plugins", "Marketplace", "Vitruvio"));
		
		bUseRTTI = true;
		bEnableExceptions = true;
		Type = ModuleType.External;

		AbstractPlatform Platform;
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			Platform = new WindowsPlatform(Debug);
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			Platform = new MacPlatform(Debug);
		}
		else
		{
			throw new System.PlatformNotSupportedException();
		}

		string LibDir = Path.Combine(ModuleDirectory, "lib", Platform.Name, "Release");
		string BinDir = Path.Combine(ModuleDirectory, "bin", Platform.Name, "Release");
		string IncludeDir = Path.Combine(ModuleDirectory, "include");

		// 1. Check if prt is already available and has correct version, otherwise download from official github repo
		bool PrtInstalled = Directory.Exists(LibDir) && Directory.Exists(BinDir);
		
		string PrtCorePath = Path.Combine(BinDir, PrtCoreDllName);
		bool PrtCoreExists = File.Exists(PrtCorePath);
		bool PrtVersionMatch = PrtCoreExists && CheckDllVersion(PrtCorePath, PrtMajor, PrtMinor, PrtBuild);

		if (!PrtInstalled || !PrtVersionMatch)
		{

			string PrtUrl = "https://github.com/Esri/esri-cityengine-sdk/releases/download";
			string PrtVersion = string.Format("{0}.{1}.{2}", PrtMajor, PrtMinor, PrtBuild);

			string PrtLibName = string.Format("esri_ce_sdk-{0}-{1}", PrtVersion, Platform.PrtPlatform);
			string PrtLibZipFile = PrtLibName + ".zip";
			string PrtDownloadUrl = Path.Combine(PrtUrl, PrtVersion, PrtLibZipFile);

			try
			{
				if (Directory.Exists(LibDir)) Directory.Delete(LibDir, true);
				if (Directory.Exists(BinDir)) Directory.Delete(BinDir, true);
				if (Directory.Exists(IncludeDir)) Directory.Delete(IncludeDir, true);

				if (Debug)
				{
					if (!PrtInstalled) Console.WriteLine("PRT not found");
					Console.WriteLine("Updating PRT");
				}

				if (Debug) System.Console.WriteLine("Downloading " + PrtDownloadUrl + "...");

				using (var Client = new WebClient())
				{
					Client.DownloadFile(PrtDownloadUrl, Path.Combine(ModuleDirectory, PrtLibZipFile));
				}

				if (Debug) System.Console.WriteLine("Extracting " + PrtLibZipFile + "...");

				Platform.ZipExtractor.Unzip(ModuleDirectory, PrtLibZipFile, PrtLibName);

				Directory.CreateDirectory(LibDir);
				Directory.CreateDirectory(BinDir);
				Copy(Path.Combine(ModuleDirectory, PrtLibName, "lib"), Path.Combine(ModuleDirectory, LibDir), FilteredExtensionLibraries);
				Copy(Path.Combine(ModuleDirectory, PrtLibName, "bin"), Path.Combine(ModuleDirectory, BinDir));
				Copy(Path.Combine(ModuleDirectory, PrtLibName, "include"), Path.Combine(ModuleDirectory, "include"));
			}
			finally
			{
				Directory.Delete(Path.Combine(ModuleDirectory, PrtLibName), true);
				File.Delete(Path.Combine(ModuleDirectory, PrtLibZipFile));
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

		// Add PRT core libraries
		if (Debug) Console.WriteLine("Adding PRT core libraries");
		foreach (string FilePath in Directory.GetFiles(BinDir))
		{
			string LibraryName = Path.GetFileName(FilePath);

			Platform.AddPrtCoreLibrary(FilePath, LibraryName, this);
		}
		
		// Add extension libraries as run-time dependencies
		if (Debug) Console.WriteLine("Adding PRT extension libraries");
		Platform.AddExtensionLibraries(LibDir, this);

		// Add include search path
		if (Debug) Console.WriteLine("Adding include search path " + IncludeDir);
		PublicSystemIncludePaths.Add(IncludeDir);
	}

	void Copy(string SrcDir, string DstDir, List<string> Filter = null)
	{
		Directory.CreateDirectory(DstDir);

		foreach (var CopyFile in Directory.GetFiles(SrcDir))
		{
			if (Filter == null || !Filter.Contains(Path.GetFileName(CopyFile)))
			{
				File.Copy(CopyFile, Path.Combine(DstDir, Path.GetFileName(CopyFile)));
			}
		}

		foreach (var Dir in Directory.GetDirectories(SrcDir))
		{
			Copy(Dir, Path.Combine(DstDir, Path.GetFileName(Dir)));
		}
	}

	public bool CheckDllVersion(string DllPath, int Major, int Minor, int Build)
	{
		FileVersionInfo Info = FileVersionInfo.GetVersionInfo(DllPath);
		string[] BuildVersions = Info.ProductVersion.Split(' ');
		int DllBuild = int.Parse(BuildVersions[BuildVersions.Length - 1]);

		bool Match = Info.FileMajorPart == Major && Info.FileMinorPart == Minor && DllBuild == Build;
		if (Debug && !Match)
		{
			Console.WriteLine(string.Format("Version {0}.{1}.{2} of \"{3}\" does not match expected version of Build file {4}.{5}.{6}",
				 Info.FileMajorPart, Info.FileMinorPart, DllBuild, Path.GetFileName(DllPath), Major, Minor, Build));
		}
		return Match;
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

		protected bool Debug;
		public AbstractPlatform(bool Debug)
		{
			this.Debug = Debug;
		}

		public virtual void AddExtensionLibraries(string SourceFolder, ModuleRules Rules)
		{
			foreach (string Dir in Directory.GetDirectories(SourceFolder))
			{
				AddExtensionLibraries(Dir, Rules);
			}
			
			foreach (string FilePath in Directory.GetFiles(SourceFolder))
			{
				Rules.RuntimeDependencies.Add(FilePath);
			}
		}

		public virtual void AddPrtCoreLibrary(string LibraryPath, string LibraryName, ModuleRules Rules)
		{
			if (Path.GetExtension(LibraryName) == DynamicLibExtension)
			{
				if (Debug) Console.WriteLine("Adding Runtime Library " + LibraryName);

				Rules.RuntimeDependencies.Add(LibraryPath);
				Rules.PublicDelayLoadDLLs.Add(LibraryName);
			}
		}
	}

	private class WindowsPlatform : AbstractPlatform
	{
		public override AbstractZipExtractor ZipExtractor { get { return new WindowsZipExtractor(); } }

		public override string Name { get { return "Win64"; } }
		public override string PrtPlatform { get { return "win10-vc1427-x86_64-rel-opt"; } }
		public override string DynamicLibExtension { get { return ".dll"; } }
		
		public WindowsPlatform(bool Debug) : base(Debug)
		{
		}
		
		public override void AddPrtCoreLibrary(string LibraryPath, string LibraryName, ModuleRules Rules)
		{
			base.AddPrtCoreLibrary(LibraryPath, LibraryName, Rules);

			if (Path.GetExtension(LibraryPath) == ".lib")
			{
				if (Debug) Console.WriteLine("Adding Public Additional Library " + LibraryName);

				Rules.PublicAdditionalLibraries.Add(LibraryPath);
			}
		}
	}

	private class MacPlatform : AbstractPlatform
	{
		public override AbstractZipExtractor ZipExtractor {	get { return new UnixZipExtractor(); } }
		
		public MacPlatform(bool Debug) : base(Debug)
		{
		} 
		
		public override string Name { get { return "Mac"; } }
		public override string PrtPlatform { get { return "osx12-ac81-x86_64-rel-opt"; } }
		public override string DynamicLibExtension { get { return ".dylib"; } }
	}
}
