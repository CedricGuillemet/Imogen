# Imogen
GPU/CPU Texture Generator

WIP of a GPU Texture generator using dear imgui for UI. Not production ready and a bit messy but really fun to code.
Basically, add GPU and CPU nodes in a graph to manipulate and generate images. Nodes are hardcoded now but a discovery system is planned.
Currently nodes can be written in GLSL or C. Python  is coming next.

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
- Polar coordinates
- ...

Check the project page for roadmap.
