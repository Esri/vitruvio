## Development Setup

This page describes how to setup development for ArcGIS CityEngine for Unreal Engine. This requires [setting up Visual Studio for Unreal Engine](https://docs.unrealengine.com/en-US/ProductionPipelines/DevelopmentSetup/VisualStudioSetup/index.html) and [Git](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git).

**Note** that the Plugin is called _Vitruvio_ internally.

1. Checkout the code via `git clone https://github.com/Esri/vitruvio.git`
2. For convenience the repository contains a project called _VitruvioHost_ with the actual Vitruvio Plugin in the _Plugins_ folder
3. Navigate to the _VitruvioHost_ folder in your checked out repository
4. Right click the _VitruvioHost.uproject_ file and run _Generate Visual Studio project files_. This will download the PRT library and setup the project. **Note** this might take a while
5. After the project files have been generated the project can be opened and compiled
