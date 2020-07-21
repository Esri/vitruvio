# Building the UnrealGeometryEncoder Library

The UnrealGeometryEncoder is the library which is loaded by PRT at runtime. The UnrealGeometryEncoder calls the callback interface defined in IUnrealCallbacks.h If you need to build the Library you can follow the steps described here.

## Prerequisites
- To build the UnrealGeometryEncoder libary you will need a source build of Unreal (as otherwise an Unreal "TargetType.Program" build is not possible). To setup an Unreal Source build follow the steps described here https://github.com/EpicGames/UnrealEngine#getting-up-and-running
- Optionally a Python 3.8 installation (for easier setup)
- A host project setup (called VitruvioHost from now on) as described in the README.md in the root directory

## Windows Build Setup
1. Open a Command Prompt with administrator permissions (important as otherwise symlinks will not work)
2. Go to the "Extras" folder of the Vitruvio Plugin (eg cd "C:/dev/Epic Games/Unreal Projects/VitruvioHost/Plugins/Vitruvio/Vitruvio/Extras")
3. Run the setup.py (eg. python setup.py). This will setup a symlink of the UnrealGeometryEncoder source into the VitruvioHost/Source folder. This symlink can also be created manually if desired.
4. In the Windows Explorer navigate to the VitruvioHost root folder and run "Generate Visual Studio Project files" from the VitruvioHost.uproject context menu (this might take a while if PRT needs to be downloaded)
5. Open the Project in Visual Studio
6. Build the UnrealGeometryEncoder project found in the "Programs" directory. Building it will automatically update the UnrealGeometryEncoderLib in the ThirdParty folder of PRT with the latest include and library files as a postbuild step
