# Imogen - User documentation


1. [Nodes](#Nodes)
1. [Default Hot Keys](#Default-Hot-Keys)


# Nodes
## Generator
![node picture](Pictures/Circle.png)|![node picture](Pictures/Square.png)|![node picture](Pictures/Checker.png)|![node picture](Pictures/Sine.png)|![node picture](Pictures/Hexagon.png)|![node picture](Pictures/CircleSplatter.png)
-|-|-|-|-|-
[Circle](#Circle)|[Square](#Square)|[Checker](#Checker)|[Sine](#Sine)|[Hexagon](#Hexagon)|[CircleSplatter](#CircleSplatter)
![node picture](Pictures/NGon.png)|![node picture](Pictures/GradientBuilder.png)|![node picture](Pictures/ReactionDiffusion.png)|![node picture](Pictures/Disolve.png)||
[NGon](#NGon)|[GradientBuilder](#GradientBuilder)|

## Transform
![node picture](Pictures/Transform.png)|![node picture](Pictures/Pixelize.png)|![node picture](Pictures/Tile.png)|![node picture](Pictures/PolarCoords.png)|![node picture](Pictures/Swirl.png)|![node picture](Pictures/Crop.png)
-|-|-|-|-|-
[Transform](#Transform)|[Pixelize](#Pixelize)|[Tile](#Tile)|[PolarCoords](#PolarCoords)|[Swirl](#Swirl)|[Crop](#Crop)
![node picture](Pictures/Warp.png)|![node picture](Pictures/EdgeDetect.png)|![node picture](Pictures/Kaleidoscope.png)|![node picture](Pictures/Palette.png)|![node picture](Pictures/ChannelPacker.png)|
[Warp](#Warp)|[EdgeDetect](#EdgeDetect)|[Kaleidoscope](#Kaleidoscope)|[Palette](#Palette)|

## Filter
![node picture](Pictures/SmoothStep.png)|![node picture](Pictures/Blur.png)|![node picture](Pictures/NormalMap.png)|![node picture](Pictures/Invert.png)|![node picture](Pictures/Ramp.png)|![node picture](Pictures/Clamp.png)
-|-|-|-|-|-
[SmoothStep](#SmoothStep)|[Blur](#Blur)|[NormalMap](#NormalMap)|[Invert](#Invert)|[Ramp](#Ramp)|[Clamp](#Clamp)
![node picture](Pictures/AO.png)||

## Material
![node picture](Pictures/LambertMaterial.png)|![node picture](Pictures/PBR.png)|![node picture](Pictures/TerrainPreview.png)|![node picture](Pictures/PathTracer.png)|![node picture](Pictures/PBR2.png)|
-|-|-|-|-|-
[LambertMaterial](#LambertMaterial)|[PBR](#PBR)|[TerrainPreview](#TerrainPreview)|[PathTracer](#PathTracer)|

## Blend
![node picture](Pictures/MADD.png)|![node picture](Pictures/Blend.png)|![node picture](Pictures/NormalMapBlending.png)|||
-|-|-|-


## None
![node picture](Pictures/Color.png)||

## Noise
![node picture](Pictures/iqnoise.png)|![node picture](Pictures/PerlinNoise.png)|![node picture](Pictures/Voronoi.png)|||
-|-|-|-


## File
![node picture](Pictures/ImageRead.png)|![node picture](Pictures/ImageWrite.png)|![node picture](Pictures/Thumbnail.png)|![node picture](Pictures/SVG.png)|![node picture](Pictures/SceneLoader.png)|![node picture](Pictures/GLTFRead.png)
-|-|-|-|-|-
[ImageRead](#ImageRead)|[ImageWrite](#ImageWrite)|[Thumbnail](#Thumbnail)|[SVG](#SVG)|[SceneLoader](#SceneLoader)|[GLTFRead](#GLTFRead)


## Paint
![node picture](Pictures/Paint2D.png)|![node picture](Pictures/Paint3D.png)|||

## Cubemap
![node picture](Pictures/PhysicalSky.png)|![node picture](Pictures/CubemapView.png)|![node picture](Pictures/EquirectConverter.png)|![node picture](Pictures/CubeRadiance.png)||
-|-|-|-|-
[PhysicalSky](#PhysicalSky)|[CubemapView](#CubemapView)|

## Fur
![node picture](Pictures/FurGenerator.png)|![node picture](Pictures/FurDisplay.png)|![node picture](Pictures/FurIntegrator.png)|||
-|-|-|-


## Circle
![node picture](Pictures/Circle.png)

Category : Generator
### Description
This is a super node. believe me!
### Parameters
1. Radius
This is a super parameter. believe me!
1. T
This is a super parameter. believe me!

## Transform
![node picture](Pictures/Transform.png)

Category : Transform
### Description
This is a super node. believe me!
### Parameters
1. Translate
This is a super parameter. believe me!
1. Scale
This is a super parameter. believe me!
1. Rotation
This is a super parameter. believe me!

## Square
![node picture](Pictures/Square.png)

Category : Generator
### Description
This is a super node. believe me!
### Parameters
1. Width
This is a super parameter. believe me!

## Checker
![node picture](Pictures/Checker.png)

Category : Generator
### Description
This is a super node. believe me!
### Parameters
No parameter for this node.

## Sine
![node picture](Pictures/Sine.png)

Category : Generator
### Description
This is a super node. believe me!
### Parameters
1. Frequency
This is a super parameter. believe me!
1. Angle
This is a super parameter. believe me!

## SmoothStep
![node picture](Pictures/SmoothStep.png)

Category : Filter
### Description
This is a super node. believe me!
### Parameters
1. Low
This is a super parameter. believe me!
1. High
This is a super parameter. believe me!

## Pixelize
![node picture](Pictures/Pixelize.png)

Category : Transform
### Description
This is a super node. believe me!
### Parameters
1. scale
This is a super parameter. believe me!

## Blur
![node picture](Pictures/Blur.png)

Category : Filter
### Description
This is a super node. believe me!
### Parameters
1. Type
This is a super parameter. believe me!
1. angle
This is a super parameter. believe me!
1. strength
This is a super parameter. believe me!
1. passCount
This is a super parameter. believe me!

## NormalMap
![node picture](Pictures/NormalMap.png)

Category : Filter
### Description
This is a super node. believe me!
### Parameters
1. spread
This is a super parameter. believe me!
1. Invert
This is a super parameter. believe me!

## LambertMaterial
![node picture](Pictures/LambertMaterial.png)

Category : Material
### Description
This is a super node. believe me!
### Parameters
1. view
This is a super parameter. believe me!

## MADD
![node picture](Pictures/MADD.png)

Category : Blend
### Description
This is a super node. believe me!
### Parameters
1. Mul Color
This is a super parameter. believe me!
1. Add Color
This is a super parameter. believe me!

## Hexagon
![node picture](Pictures/Hexagon.png)

Category : Generator
### Description
This is a super node. believe me!
### Parameters
No parameter for this node.

## Blend
![node picture](Pictures/Blend.png)

Category : Blend
### Description
This is a super node. believe me!
### Parameters
1. A
This is a super parameter. believe me!
1. B
This is a super parameter. believe me!
1. Operation
This is a super parameter. believe me!

![node picture](Examples/Example_Blend_0.png)|![node picture](Examples/Example_Blend_1.png)|![node picture](Examples/Example_Blend_2.png)
-|-|-
blend enum 0|blend enum 1|blend enum 2
![node picture](Examples/Example_Blend_3.png)|![node picture](Examples/Example_Blend_4.png)|![node picture](Examples/Example_Blend_5.png)
blend enum 3|blend enum 4|blend enum 5
![node picture](Examples/Example_Blend_6.png)|![node picture](Examples/Example_Blend_7.png)|![node picture](Examples/Example_Blend_8.png)
blend enum 6|blend enum 7|blend enum 8
![node picture](Examples/Example_Blend_9.png)|![node picture](Examples/Example_Blend_10.png)|![node picture](Examples/Example_Blend_11.png)
blend enum 9|blend enum 10|blend enum 11
![node picture](Examples/Example_Blend_12.png)||

## Invert
![node picture](Pictures/Invert.png)

Category : Filter
### Description
This is a super node. believe me!
### Parameters
No parameter for this node.

## CircleSplatter
![node picture](Pictures/CircleSplatter.png)

Category : Generator
### Description
This is a super node. believe me!
### Parameters
1. Distance
This is a super parameter. believe me!
1. Radius
This is a super parameter. believe me!
1. Angle
This is a super parameter. believe me!
1. Count
This is a super parameter. believe me!

## Ramp
![node picture](Pictures/Ramp.png)

Category : Filter
### Description
This is a super node. believe me!
### Parameters
1. Ramp
This is a super parameter. believe me!

## Tile
![node picture](Pictures/Tile.png)

Category : Transform
### Description
This is a super node. believe me!
### Parameters
1. Offset 0
This is a super parameter. believe me!
1. Offset 1
This is a super parameter. believe me!
1. Overlap
This is a super parameter. believe me!
1. Scale
This is a super parameter. believe me!

## Color
![node picture](Pictures/Color.png)

Category : None
### Description
This is a super node. believe me!
### Parameters
1. Color
This is a super parameter. believe me!

## NormalMapBlending
![node picture](Pictures/NormalMapBlending.png)

Category : Blend
### Description
This is a super node. believe me!
### Parameters
1. Technique
This is a super parameter. believe me!

## iqnoise
![node picture](Pictures/iqnoise.png)

Category : Noise
### Description
This is a super node. believe me!
### Parameters
1. Translation
This is a super parameter. believe me!
1. Size
This is a super parameter. believe me!
1. U
This is a super parameter. believe me!
1. V
This is a super parameter. believe me!

## PerlinNoise
![node picture](Pictures/PerlinNoise.png)

Category : Noise
### Description
This is a super node. believe me!
### Parameters
1. Translation
This is a super parameter. believe me!
1. Octaves
This is a super parameter. believe me!
1. lacunarity
This is a super parameter. believe me!
1. gain
This is a super parameter. believe me!

## PBR
![node picture](Pictures/PBR.png)

Category : Material
### Description
This is a super node. believe me!
### Parameters
1. View
This is a super parameter. believe me!
1. Displacement Factor
This is a super parameter. believe me!
1. Geometry
This is a super parameter. believe me!

## PolarCoords
![node picture](Pictures/PolarCoords.png)

Category : Transform
### Description
This is a super node. believe me!
### Parameters
1. Type
This is a super parameter. believe me!

## Clamp
![node picture](Pictures/Clamp.png)

Category : Filter
### Description
This is a super node. believe me!
### Parameters
1. Min
This is a super parameter. believe me!
1. Max
This is a super parameter. believe me!

## ImageRead
![node picture](Pictures/ImageRead.png)

Category : File
### Description
This is a super node. believe me!
### Parameters
1. File name
This is a super parameter. believe me!
1. +X File name
This is a super parameter. believe me!
1. -X File name
This is a super parameter. believe me!
1. +Y File name
This is a super parameter. believe me!
1. -Y File name
This is a super parameter. believe me!
1. +Z File name
This is a super parameter. believe me!
1. -Z File name
This is a super parameter. believe me!

## ImageWrite
![node picture](Pictures/ImageWrite.png)

Category : File
### Description
This is a super node. believe me!
### Parameters
1. File name
This is a super parameter. believe me!
1. Format
This is a super parameter. believe me!
1. Quality
This is a super parameter. believe me!
1. Width
This is a super parameter. believe me!
1. Height
This is a super parameter. believe me!
1. Mode
This is a super parameter. believe me!
1. Export
This is a super parameter. believe me!

## Thumbnail
![node picture](Pictures/Thumbnail.png)

Category : File
### Description
This is a super node. believe me!
### Parameters
1. Make
This is a super parameter. believe me!

## Paint2D
![node picture](Pictures/Paint2D.png)

Category : Paint
### Description
This is a super node. believe me!
### Parameters
1. Size
This is a super parameter. believe me!

## Swirl
![node picture](Pictures/Swirl.png)

Category : Transform
### Description
This is a super node. believe me!
### Parameters
1. Angles
This is a super parameter. believe me!

## Crop
![node picture](Pictures/Crop.png)

Category : Transform
### Description
This is a super node. believe me!
### Parameters
1. Quad
This is a super parameter. believe me!

## PhysicalSky
![node picture](Pictures/PhysicalSky.png)

Category : Cubemap
### Description
This is a super node. believe me!
### Parameters
1. ambient
This is a super parameter. believe me!
1. lightdir
This is a super parameter. believe me!
1. Kr
This is a super parameter. believe me!
1. rayleigh brightness
This is a super parameter. believe me!
1. mie brightness
This is a super parameter. believe me!
1. spot brightness
This is a super parameter. believe me!
1. scatter strength
This is a super parameter. believe me!
1. rayleigh strength
This is a super parameter. believe me!
1. mie strength
This is a super parameter. believe me!
1. rayleigh collection power
This is a super parameter. believe me!
1. mie collection power
This is a super parameter. believe me!
1. mie distribution
This is a super parameter. believe me!
1. Size
This is a super parameter. believe me!

## CubemapView
![node picture](Pictures/CubemapView.png)

Category : Cubemap
### Description
This is a super node. believe me!
### Parameters
1. view
This is a super parameter. believe me!
1. Mode
This is a super parameter. believe me!
1. LOD
This is a super parameter. believe me!

## EquirectConverter
![node picture](Pictures/EquirectConverter.png)

Category : Cubemap
### Description
This is a super node. believe me!
### Parameters
1. Mode
This is a super parameter. believe me!
1. Size
This is a super parameter. believe me!

## NGon
![node picture](Pictures/NGon.png)

Category : Generator
### Description
This is a super node. believe me!
### Parameters
1. Sides
This is a super parameter. believe me!
1. Radius
This is a super parameter. believe me!
1. T
This is a super parameter. believe me!

## GradientBuilder
![node picture](Pictures/GradientBuilder.png)

Category : Generator
### Description
This is a super node. believe me!
### Parameters
1. Gradient
This is a super parameter. believe me!

## Warp
![node picture](Pictures/Warp.png)

Category : Transform
### Description
This is a super node. believe me!
### Parameters
1. Strength
This is a super parameter. believe me!
1. Mode
This is a super parameter. believe me!

## TerrainPreview
![node picture](Pictures/TerrainPreview.png)

Category : Material
### Description
This is a super node. believe me!
### Parameters
1. Camera
This is a super parameter. believe me!

## AO
![node picture](Pictures/AO.png)

Category : Filter
### Description
This is a super node. believe me!
### Parameters
1. strength
This is a super parameter. believe me!
1. area
This is a super parameter. believe me!
1. falloff
This is a super parameter. believe me!
1. radius
This is a super parameter. believe me!

## FurGenerator
![node picture](Pictures/FurGenerator.png)

Category : Fur
### Description
This is a super node. believe me!
### Parameters
1. Hair count
This is a super parameter. believe me!
1. Length factor
This is a super parameter. believe me!

## FurDisplay
![node picture](Pictures/FurDisplay.png)

Category : Fur
### Description
This is a super node. believe me!
### Parameters
1. Camera
This is a super parameter. believe me!

## FurIntegrator
![node picture](Pictures/FurIntegrator.png)

Category : Fur
### Description
This is a super node. believe me!
### Parameters
No parameter for this node.

## SVG
![node picture](Pictures/SVG.png)

Category : File
### Description
This is a super node. believe me!
### Parameters
1. File name
This is a super parameter. believe me!
1. DPI
This is a super parameter. believe me!

## SceneLoader
![node picture](Pictures/SceneLoader.png)

Category : File
### Description
This is a super node. believe me!
### Parameters
1. File name
This is a super parameter. believe me!

## PathTracer
![node picture](Pictures/PathTracer.png)

Category : Material
### Description
This is a super node. believe me!
### Parameters
1. Mode
This is a super parameter. believe me!
1. Camera
This is a super parameter. believe me!

## EdgeDetect
![node picture](Pictures/EdgeDetect.png)

Category : Transform
### Description
This is a super node. believe me!
### Parameters
1. Edges
This is a super parameter. believe me!
1. Radius
This is a super parameter. believe me!

## Voronoi
![node picture](Pictures/Voronoi.png)

Category : Noise
### Description
This is a super node. believe me!
### Parameters
1. Point Count
This is a super parameter. believe me!
1. Seed
This is a super parameter. believe me!
1. Distance Blend
This is a super parameter. believe me!
1. Square Width
This is a super parameter. believe me!

## Kaleidoscope
![node picture](Pictures/Kaleidoscope.png)

Category : Transform
### Description
This is a super node. believe me!
### Parameters
1. Center
This is a super parameter. believe me!
1. Start Angle
This is a super parameter. believe me!
1. Splits
This is a super parameter. believe me!
1. Symetry
This is a super parameter. believe me!

## Palette
![node picture](Pictures/Palette.png)

Category : Transform
### Description
This is a super node. believe me!
### Parameters
1. Palette
This is a super parameter. believe me!
1. Dither Strength
This is a super parameter. believe me!

## ReactionDiffusion
![node picture](Pictures/ReactionDiffusion.png)

Category : Generator
### Description
This is a super node. believe me!
### Parameters
1. boost
This is a super parameter. believe me!
1. divisor
This is a super parameter. believe me!
1. colorStep
This is a super parameter. believe me!
1. passCount
This is a super parameter. believe me!
1. Size
This is a super parameter. believe me!

## Disolve
![node picture](Pictures/Disolve.png)

Category : Generator
### Description
This is a super node. believe me!
### Parameters
1. passCount
This is a super parameter. believe me!
1. Frequency
This is a super parameter. believe me!
1. Strength
This is a super parameter. believe me!
1. Randomization
This is a super parameter. believe me!
1. VerticalShift
This is a super parameter. believe me!

## GLTFRead
![node picture](Pictures/GLTFRead.png)

Category : File
### Description
This is a super node. believe me!
### Parameters
1. File name
This is a super parameter. believe me!
1. Camera
This is a super parameter. believe me!

## Paint3D
![node picture](Pictures/Paint3D.png)

Category : Paint
### Description
This is a super node. believe me!
### Parameters
1. Size
This is a super parameter. believe me!
1. Camera
This is a super parameter. believe me!

## CubeRadiance
![node picture](Pictures/CubeRadiance.png)

Category : Cubemap
### Description
This is a super node. believe me!
### Parameters
1. Mode
This is a super parameter. believe me!
1. Size
This is a super parameter. believe me!
1. Sample Count
This is a super parameter. believe me!

## PBR2
![node picture](Pictures/PBR2.png)

Category : Material
### Description
This is a super node. believe me!
### Parameters
1. View
This is a super parameter. believe me!
1. Depth factor
This is a super parameter. believe me!

## ChannelPacker
![node picture](Pictures/ChannelPacker.png)

Category : Transform
### Description
This is a super node. believe me!
### Parameters
1. R
This is a super parameter. believe me!
1. G
This is a super parameter. believe me!
1. B
This is a super parameter. believe me!
1. A
This is a super parameter. believe me!

# Default Hot Keys

Action|Description|Hot key
-|-|-
Layout|Reorder nodes in a simpler layout|Ctrl + L
PlayPause|Play or Stop current animation|F5
AnimationFirstFrame|Set current time to the first frame of animation|V
AnimationNextFrame|Move to the next animation frame|N
AnimationPreviousFrame|Move to previous animation frame|B
MaterialExport|Export current material to a file|Ctrl + E
MaterialImport|Import a material file in the library|Ctrl + I
ToggleLibrary|Show or hide Libaray window|Ctrl + 1
ToggleNodeGraph|Show or hide Node graph window|Ctrl + 2
ToggleLogger|Show or hide Logger window|Ctrl + 3
ToggleSequencer|Show or hide Sequencer window|Ctrl + 4
ToggleParameters|Show or hide Parameters window|Ctrl + 5
MaterialNew|Create a new graph|Ctrl + N
ReloadShaders|Reload them|F7
DeleteSelectedNodes|Delete selected nodes in the current graph|Del
AnimationSetKey|Make a new animation key with the current parameters values at the current time|S
HotKeyEditor|Open the Hotkey editor window|Ctrl + K
NewNodePopup|Open the new node menu|Tab
Undo|Undo the last operation|Ctrl + Z
Redo|Redo the last undo|Ctrl + Shift +Z
Copy|Copy the selected nodes|Ctrl + C
Cut|Cut the selected nodes|Ctrl + X
Paste|Paste previously copy/cut nodes|Ctrl + V
BuildMaterial|Build current material|Ctrl + B
