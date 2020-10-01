# Vitruvio - CityEngine Plugin for Unreal Engine 4

Vitruvio allows the execution of Esri CityEngine *rules* within Unreal Engine 4. Therefore, an artist does not have to leave their familiar Unreal toolset anymore to make use of CityEngine’s procedural modeling power.

Vitruvio requires rule packages (RPK) as input which are authored in CityEngine. An RPK includes assets and a CGA rule file which encodes an architectural style.

## Windows Development Setup

1. Checkout the code via `git clone https://devtopia.esri.com/Zurich-R-D-Center/vitruvio.git`
2. Create a new Unreal project (called host project from now on)
3. Create a folder named *Plugins* in the root directory of the host project
4. Start PowerShell as administrator
5. Create a symbolic link from the vitruvio repo to a folder named Vitruvio in the *Plugins* folder of the host project. Using the `New-Item` command. For example: `New-Item -Path "C:\dev\Epic Games\Unreal Projects\VitruvioHost\Plugins\Vitruvio" -ItemType SymbolicLink -Value "C:\dev\git\vitruvio\Vitruvio"`
6. Right click the uproject file of your Host project and run Generate Visual Studio project files. This will download the PRT library and setup the project. **Note** this might take a while.
7. After the project files have been generated the project can be opened.
