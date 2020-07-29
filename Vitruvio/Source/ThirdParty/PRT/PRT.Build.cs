// Copyright 2019 - 2020 Esri. All Rights Reserved.

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
	private const bool Debug = true;

	// PRT version and toolchain (needs to be correct for download URL)
	private const int PrtMajor = 2;
	private const int PrtMinor = 1;
	private const int PrtBuild = 5705;

	private const string PrtCoreDllName = "com.esri.prt.core.dll";
	private static string[] ExtensionLibraries = { "com.esri.prt.adaptors.dll", "com.esri.prt.codecs.dll", "VueExport.dll" };

	const long ERROR_SHARING_VIOLATION = 0x20;
	const long ERROR_LOCK_VIOLATION = 0x21;

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
				Copy(Path.Combine(ModuleDirectory, PrtLibName, "lib"), Path.Combine(ModuleDirectory, LibDir));
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

		// Copy PRT core libraries
		if (Debug) Console.WriteLine("Adding PRT core libraries");
		foreach (string FilePath in Directory.GetFiles(BinDir))
		{
			string LibraryName = Path.GetFileName(FilePath);

			Platform.AddPrtCoreLibrary(FilePath, LibraryName, this);
		}

		// Delete unused PRT extension libraries
		if (Debug) Console.WriteLine("Deleting unused PRT extension libraries");
		foreach (string FilePath in Directory.GetFiles(LibDir))
		{
			string FileName = Path.GetFileName(FilePath);
			if (Path.GetExtension(FilePath) == Platform.DynamicLibExtension)
			{
				if (!Array.Exists(ExtensionLibraries, e => e == Path.GetFileName(FilePath)))
				{
					File.Delete(FilePath);
				} 
				else
				{
					RuntimeDependencies.Add(FilePath);
					PublicDelayLoadDLLs.Add(FileName);
				}
			}
		}

		// Add include search path
		if (Debug) Console.WriteLine("Adding include search path " + IncludeDir);
		PublicSystemIncludePaths.Add(IncludeDir);
	}

	private static void CopyLibraryFile(string SrcLibDir, string SrcFile, string DstLibFile)
	{
		string SrcLib = Path.Combine(SrcLibDir, SrcFile);
		if (!System.IO.File.Exists(DstLibFile) || System.IO.File.GetCreationTime(SrcLib) > System.IO.File.GetCreationTime(DstLibFile))
		{
		
			if (Debug) Console.WriteLine("\tCopying " + SrcFile + " to " + DstLibFile);

			try
			{
				// For some reason File.Copy does not always preserve the creation time so we set it manually
				DateTime CreationTime = System.IO.File.GetCreationTime(SrcLib);
				System.IO.File.Copy(SrcLib, DstLibFile, true);
				System.IO.File.SetCreationTime(DstLibFile, CreationTime);
			} 
			catch (IOException Ex)
			{
				// Check if the library is currently locked (happens if a build is triggered which needs to redownload/install PRT while Unreal is running).
				// If so, we abort the build and let the user know that a build from "source" is required (with Unreal closed).
				long Win32ErrorCode = Ex.HResult & 0xFFFF;
				if (Win32ErrorCode == ERROR_SHARING_VIOLATION || Win32ErrorCode == ERROR_LOCK_VIOLATION)
				{
					string ErroMessage = string.Format("'{0}'is currently locked by another process. Trying to install new PRT library while Unreal is running is only possible from a source build.", DstLibFile);
					throw new Exception(ErroMessage);
				}
				throw Ex;
			}

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

	public static bool CheckDllVersion(string DllPath, int Major, int Minor, int Build)
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
		public override string PrtPlatform { get { return "win10-vc141-x86_64-rel-opt"; } }
		public override string DynamicLibExtension { get { return ".dll"; } }

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

		public override string Name { get { return "Mac"; } }
		public override string PrtPlatform { get { return "osx12-ac81-x86_64-rel-opt"; } }
		public override string DynamicLibExtension { get { return ".dylib"; } }
	}
}
