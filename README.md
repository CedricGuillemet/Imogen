# Imogen
GPU Texture Generator

WIP of a GPU Texture generator using dear imgui for UI.

![Image of imogen](https://i.imgur.com/iQxLNEC.png)

Simple roadmap:

~~node edit~~

~~node evaluation list~~

~~basic load/save~~

~~node parameters with preview~~

- add nodes (noise, blur, voronoi, simple ops,..) and make nice textures

Currently implemented nodes
- circle and square generator
- sine generator
- checker transform
- transform
- Mul/Add
- smoothstep
- pixelize
- blur
- normal map from height map
- sphere/plan previewer
- Hexagon
- Mul-Add colors
- Blend (add, mul, min, max)
- Invert color
- Circle Splatter
- Ramp
- Tile

Distant roadmap when I can make some nice textures and get self confident enough 
- undo/redo

~~graph dirty propagation~~

- bullet proof serialization
- material/PBR preview
- texture baking / batch baking
- baking for 64k demos (node parameters + baker source/sdk)
- texture library browser
- MOAR nodes: AO node
- compute node metadata by parsing GLSL shader code
- better parameter controls (sliders, color picker,..)
- misc settings like baking dimension

~~hot loading here and there. In tool shader editor~~

- ease of integration in your 64k demo/tools

~~- color coding of nodes depending of their type(generator, blur, transform, material, ...)~~

- node group/sub node: reuse a subnode in your new texture
- pin parameter controls and result from various nodes into one view
- ...
