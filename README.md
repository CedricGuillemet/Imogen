# Imogen
GPU Texture Generator

WIP of a GPU Texture generator using dear imgui for UI. Not production ready. You can only edit the "default" material. I do it for fun and to learn a couple things.

screen from 2018-09-02
![Image of imogen](https://i.imgur.com/iQxLNEC.png)
screen from 2018-09-09
![Image of imogen](https://i.imgur.com/RQGHOfj.png)
Simple roadmap:

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

Roadmap/todo
- undo/redo
- bullet proof serialization
- material/PBR preview
- texture baking / batch baking
- baking for 64k demos (node parameters + baker source/sdk)
- texture library browser
- MOAR nodes: AO node
- compute node metadata by parsing GLSL shader code
- better parameter controls (sliders, color picker,..)
- misc settings like baking dimension
- ease of integration in your 64k demo/tools
- node group/sub node: reuse a subnode in your new texture
- pin parameter controls and result from various nodes into one view
- ...
