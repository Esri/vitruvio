## Development Setup

This page describes how to setup Vitruvio for development. This requires [setting up Visual Studio for Unreal Engine](https://docs.unrealengine.com/en-US/ProductionPipelines/DevelopmentSetup/VisualStudioSetup/index.html) and [Git](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git).

1. Checkout the code via `git clone https://devtopia.esri.com/Zurich-R-D-Center/vitruvio.git`
2. For convenience the repository contains a project called *VitruvioHost* with the actual Vitruvio Plugin in the *Plugins* folder
6. Navigate to the *VitruvioHost* folder in your checked out repository
4. Right click the *VitruvioHost.uproject* file and run *Generate Visual Studio project files*. This will download the PRT library and setup the project. **Note** this might take a while
5. After the project files have been generated the project can be opened and compiled
