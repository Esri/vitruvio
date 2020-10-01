# Usage

To generate a model, Vitruvio leverages the CityEngine Procedural Runtime (PRT), which takes a *rule package (RPK)*, an *initial shape* and a *set of attributes* as inputs. The generation process starts with the initial shape as start shape, from which shape grammar rules are expanded. The attributes are parameters that control shape generation.

This section describes how to export rule packages from CityEngine, how to import or create initial shapes and how to use the Vitruvio Actor or Component in UE4.

## Vitruvio Component

The *Vitruvio Component* allows the user to access the procedural generation. For ease of use there is also a *Vitruvio Actor* available, which can be found in the *Place Actors* Panel and placed anywhere in the scene.

<img src="img/select_vitruvio_actor.jpg" width="400">

After placing the actor in the scene and selecting it, the *Details* panel shows all relevant properties.

<img src="img/vitruvio_component.jpg" width="400">

**Initial Shape Type:** The type of input initial shape used. For more information on how to import or create initial shapes see Initial Shapes.

**Rule Package:** The rule package to be used. For more information on how to export rule packages from CityEngine and importing them into UE4 see Rule Packages.

**Random Seed**: The random seed to be used for generation. See also [CityEngine Help](https://doc.arcgis.com/en/cityengine/2019.1/help/help-working-with-rules.htm#GUID-FD7F11D4-778E-4485-901B-E11DDD2099F2).

**Generate Automatically:** Whether to generate automatically after changes to relevant properties such as the initial shape, rule package or attributes.

**Hide after Generation:** Whether to hide the initial shape geometry after a model has been generated

