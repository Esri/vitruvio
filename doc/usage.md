# Usage

Vitruvio leverages CityEngine's Procedural Runtime (PRT) to generate buildings. As input it takes a *rule package (RPK)*, an *initial shape* and a *set of attributes*. The generation process starts with the initial shape as start shape, from which shape grammar rules are expanded. The attributes are parameters that control shape generation.

This section describes how to use the Vitruvio Actor in Unreal Engine 5 (UE5), export rule packages from CityEngine and how to import or create initial shapes.

**Note** that the plugin contains a demo folder with a simple level to try out Vitruvio. First, enable *Show Engine Content* and *Show Plugin Content* in the View Options (bottom right) of the Content Browser. The demo content can now be found in the *Vitruvio Content/Demo/* folder.

## Vitruvio Actor and Component

The *Vitruvio Component* allows the user to access the procedural generation. It can be attached to any Unreal Actor. If the Actor already has a valid initial shape component attached it will automatically be used as the initial shape for the building generation. Refer to [Initial Shapes](#Initial-Shapes) for more information.

For ease of use the Vitruvio plugin also provides the *Vitruvio Actor* which can be found in the *Place Actors* Panel and placed anywhere in the scene.

<img src="img/select_vitruvio_actor.jpg" width="400">

After placing the Vitruvio Actor in the scene the *Details* panel shows all relevant properties.

<img src="img/vitruvio_component.jpg" width="400">

**Generate Automatically:** Whether to generate automatically after changes to properties such as the initial shape, rule package or attributes.

**Hide Initial Shape after Generation:** Whether to hide the initial shape geometry after a model has been generated.

**Generate Collision Mesh:** Whether to generate a collision mesh (complex collision) after the generation.

**Rule Package:** The rule package to be used. For more information on how to export rule packages from CityEngine and importing them into UE5 see [Rule Packages](#Rule-Packages).

**Random Seed:** The random seed to be used for generation. See also [CityEngine Help](https://doc.arcgis.com/en/cityengine/2019.1/help/help-working-with-rules.htm#GUID-FD7F11D4-778E-4485-901B-E11DDD2099F2).

**Reports:** The generated [CGA report](https://doc.arcgis.com/en/cityengine/latest/cga/cga-report.htm) values. These values can be accessed with the [Blueprint API](#Blueprint-Support).

**Initial Shape Type:** The type of input initial shape used. For more information on how to import or create initial shapes see [Initial Shapes](#Initial-Shapes).

**Attributes:** The attributes of the selected rule package which control the generation. See also [Attributes](#Attributes).

## Rule Packages

A [rule package](https://doc.arcgis.com/en/cityengine/2019.0/help/help-rule-package.htm) (RPK) is a compressed package, containing compiled CGA rule files, plus all needed referenced assets and data. RPKs can be exported in CityEngine by right clicking on a CGA file and using the *Share As...* menu. Make sure to include all necessary assets in *Additional Files* and set *Save package to file* to the path you want to export the RPK to.

<img src="img/export_rpk.jpg" width="300">

The exported RPK can then be dragged into the Unreal Editor’s *Content Browser* which will import it into your project.

**Note** that there is currently a limit of 2GB file size for imported RPKs.

<img src="img/import_rpk.jpg" width="400">

The imported RPK Asset can now be dragged on the *Rule Package* field of a Vitruvio Actor to assign it.

## Initial Shapes

Initial shapes ([CGA modeling overview](https://doc.arcgis.com/en/cityengine/latest/help/help-cga-modeling-overview.htm)) represent the input geometry which typically are polygons that represent a lot or a building footprint. Vitruvio supports two kind of initial shapes, *Static Meshes* and *Splines*.

### Static Mesh

To change the initial shape geometry you can assign a Static Mesh to the *Initial Shape Mesh* field. **Note** that currently only planar geometry is supported. 

<img src="img/change_static_mesh.jpg" width="400">

### Splines

To use a Spline as an initial shape, change the **Initial Shape Type** drop down to **Spline**.

<img src="img/spline_edit.gif" width="800">

To copy a spline point, select an existing point, press alt and drag the point. Spline points can either be linear or curved. The type of an individual point can be changed by selecting the *Spline Component* of the *InitialShapeSpline* and in the *Selected Points* header of the details panel.

## Attributes

Attributes control the procedural generation of a model. The set of available attributes depends on the underlying rule of the assigned *Rule Package*. 

Selecting a *Vitruvio Actor* will display all its attributes in the details panel. The values can be changed to control the procedural generation.

<img src="img/vitruvio_attributes.gif" width="800">

**Note:** if *generate automatically* is enabled, every attribute change will regenerate the model.  

## Advanced

### Initial Shapes from CityEngine to Unreal Engine

This section explains how to export a set of building footprints from CityEngine and how to import them into Unreal to use them as initial shapes for building generation.

#### CityEngine

To export initial shape building footprints from CityEngine the [Datasmith Exporter](https://doc.arcgis.com/en/cityengine/latest/help/help-export-unreal.htm) can be used. Select the building footprints in the viewport and then choose **File** &rarr; **Export models…** to export them. Select the *Unreal Engine* exporter and make sure to set *Export Geometry* to **Shapes** and *Mesh Merging* to **Per Initial Shape**. This will make sure that each footprint (without generated models) is exported individually.

<img src="img/export_initial_shapes.jpg" width="400">

#### Unreal Engine

**Note:**  To import datasmith files, the **Datasmith Importer** plugin needs to be enabled in your project. Go to **Edit** &rarr; **Plugins** and verify that the **Datasmith Importer** plugin is enabled.

First import the datasmith file from CityEngine using the **Datasmith** importer. The default import settings can be used.

Now convert all imported initial shapes to Vitruvio Actors:

1. Select the *DatasmithSceneActor* (this is the root Actor of the Datasmith scene)
2. Right click and choose **Select All Viable Initial Shapes in Hierarchy**. This will select all child Actors which are viable initial shapes (meaning all Actors which either have a valid *StaticMeshComponent* or *SplineComponent* attached).

<img src="img/select_vitruvio_actors.jpg" width="400">

3. Right click again on any selected Actor and choose **Convert to Vitruvio Actor**. In the opened dialog, choose any RPK you want to assign. This will convert all selected Actors to Vitruvio Actors and assign the chosen RPK.

### Blueprint Support

This section explains how to use Blueprints with Vitruvio Actors. Unreal Engines' [Blueprint System](https://docs.unrealengine.com/5.0/en-US/blueprints-visual-scripting-in-unreal-engine) is a powerful visual scripting language. The VitruvioComponent provides several Blueprint functions to control its behavior, by for example setting new attribute values or accessing reports after the generation of a model.

**Note:** the *generate automatically* flag of the VitruvioComponent is ignored for Blueprint API calls. Re-generation is controlled for each function call individually via a parameter. 

#### Setup

We first need to retrieve the VitruvioComponent to get access to all neccessary functions. In the **ReportingDemo** scene (in the "VitruvioContent/Demo" folder) open the [Level Blueprint](https://docs.unrealengine.com/4.27/en-US/ProgrammingAndScripting/Blueprints/UserGuide/Types/LevelBlueprint/) and drag the *Candler Building* Actor from the Outliner into the Blueprint to access the VitruvioComponent as follows:

<img title="" src="img/access_vitruvio_actor.jpg" alt="" width="800">

#### Modifying Attributes

Next, we modify the **nFloor** attribute of the *Report Building* Vitruvio Actor as follows:

<img title="" src="img/set_float_attribute.jpg" alt="" width="800">

This call is executed asynchrounsly. The default execution path is executed immediately after the call has completed, *Attributes Evaluated* is executed once the attributes have been evaluated and *Generate Completed* is executed once the new model has been generated. If *Generate Model* is set to false, *Generate Completed* is never executed. 

All other Vitruvio Blueprint calls follow a similar structure.

**Note:** that when multiple asynchronous Vitruvio Blueprint calls are executed simultaneously, only one *Generate Completed* is executed.

**Note:** that the fully qualified attribute name needs to be passed. The fully qualified name includes the CGA style prefix (which is *Default\$* for all attributes for now). You can access the name from Details panel by right clicking on an attribute and selecting **Copy Fully Qualified Attribute Name**.

#### Accessing Reports

We can now access the reports and print them as follows:

<img title="" src="img/print_reports.jpg" alt="" width="800">

The *Attributes Evaluated* execution path is used since we need to wait until the new attributes have been evaluated before we can access the new report values.

### Batch Generation

Batch generation allows for the simultaneous generation of several models in batches, enhancing performance.

<img title="" src="img/vitruvio_batch_generate.jpg" alt="" width="800">

Batch generation can be controlled per Vitruvio Actor using the *Batch Generate* option. It is recommended to use this feature if your scene contains many Vitruvio Actors.

The advantages include improved generation as well as rendering performance. However, if a Vitruvio Actor's attributes are changed at runtime (for example, via user input), it is recommended not to allow batch generation since every attribute change requires regenerating the entire batch.

For advanced use cases the *Grid Dimension* (which controls the batch size) on the *Vitruvio Batch Actor* can be changed.

### Asset Replacements

Vitruvio Actors support automated asset (Materials and Instances) replacements using Data Tables.

![](img/vitruvio_replacements.jpg)

To begin the replacement workflow, select either *Replace Materials* or *Replace Instances* on the Vitruvio Actor. This action will prompt a dialogue where you can first select (or create) a Data Asset where the replacements are stored. **Note** that this Data Asset can also be applied to other Vitruvio Actors to implement the same replacements. Once selected, you can define the actual replacements and apply them. These replacements will now take effect after every model regeneration.