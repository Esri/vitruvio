# Vitruvio - CityEngine Plugin for Unreal Engine 4

Vitruvio is an [Unreal Engine 4](https://www.unrealengine.com/) (UE4) plugin for generation of procedural geometry based on ArcGIS CityEngine shape grammar rules. With Vitruvio, game designers or artists do not have to leave the UE4 environment to make use of CityEngine’s procedural modeling power. The buildings stay procedural all time and artists can change the height, style and appearance of buildings easily with a parametric interface. In addition, buildings can also be generated on the fly in a game at runtime.

As input, Vitruvio requires rule packages (RPK) which are authored in CityEngine. An RPK includes assets and a CGA rule file which encodes an architectural style. The download section below provides a link to the comprehensive "Favela" RPK which can be used out-of-the-box in Vitruvio.

![City generated using Vitruvio](doc/img/vitruvio_city.jpg)

## Documentation

* [User Guide](doc/usage.md)
* [Development Setup](doc/setup.md)
* [Downloads](doc/downloads.md)

External documentation:
* [CityEngine Tutorials](https://doc.arcgis.com/en/cityengine/latest/tutorials/introduction-to-the-cityengine-tutorials.htm)
* [CityEngine CGA Reference](https://doc.arcgis.com/en/cityengine/latest/cga/cityengine-cga-introduction.htm)
* [CityEngine Manual](https://doc.arcgis.com/en/cityengine/latest/help/cityengine-help-intro.htm)
* [CityEngine SDK API Reference](https://esri.github.io/cityengine-sdk/html/index.html)


## Licensing

Vitruvio is under the same license as the included [CityEngine SDK](https://github.com/Esri/esri-cityengine-sdk#licensing).

An exception is the Vitruvio source code (without CityEngine SDK, binaries, or object code), which is licensed under the Apache License, Version 2.0 (the “License”); you may not use this work except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
