# Imogen
GPU Texture Generator

WIP of a GPU Texture generator using dear imgui for UI. Not production ready and a bit messy but really fun to code.

Funny to see how it progresses:

screen from 2018-09-02
![Image of imogen](https://i.imgur.com/iQxLNEC.png)
screen from 2018-09-09
![Image of imogen](https://i.imgur.com/RQGHOfj.png)
screen from 2018-09-15
![Image of imogen](https://i.imgur.com/BJre6MN.png)

Use CMake and VisualStudio to build it. Only Windows system supported for now.

Features:
- Node based texture editing
- material library browser
- edit/change node shaders inside the app
- bake textures to .png
- PBR preview

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
- texture baking / batch baking
- more nodes: AO node, distance field,...
- multiple pass nodes
- misc parameters like baking directory,...
- compute node metadata by parsing GLSL shader code
- better parameter controls (sliders, color picker,..)
- node group/sub node: reuse a subnode in your new texture
- pin parameter controls and result from various nodes into one view
- Vulkan port
- Port to Linux (with SDL)
- ...
