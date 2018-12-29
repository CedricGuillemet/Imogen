# Imogen
GPU/CPU Texture Generator

WIP of a GPU Texture generator using dear imgui for UI. Not production ready and a bit messy but really fun to code.
Basically, add GPU and CPU nodes in a graph to manipulate and generate images. Nodes are hardcoded now but a discovery system is planned.
Currently nodes can be written in GLSL or C or Python.

![Image of Imogen 0.4](https://i.imgur.com/pmliWGl.png)
![Image of Imogen 0.4](https://i.imgur.com/jNWsXD6.png)

Use CMake and VisualStudio to build it. Only Windows system supported for now.

Features:
- Node based texture editing
- material library browser
- edit/change node shaders inside the app
- bake textures to .png, .jpg, .tga, .bmp, .hdr, mp4
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

-----------
This software uses the following (awesome) projects:

Dear ImGui - Omar Cornut https://github.com/ocornut/imgui

ImGuiColorTextEdit - Balazs Jako https://github.com/BalazsJako/ImGuiColorTextEdit

stb_image, stb_image_write - Sean T. Barett https://github.com/nothings/stb

EnkiTS - Doug Binks https://github.com/dougbinks/enkiTS

Tiny C Compiler - Fabrice Bellard https://bellard.org/tcc/

SDL2 - https://www.libsdl.org/

NativeFileDialog - Michael Labbe https://github.com/mlabbe/nativefiledialog

gl3w - Slavomir Kaslev https://github.com/skaslev/gl3w

TinyDir - https://github.com/cxong/tinydir

cmft - cubemap filtering tool - Dario Manesku https://github.com/dariomanesku/cmft

dear imgui color scheme - codz01 https://github.com/ocornut/imgui/issues/1902#issuecomment-429445321

FFMPEG - Fabrice Bellard

Python 3 - Python.org

pybind 11 - https://github.com/pybind/pybind11

nanosvg - Mikko Mononen https://github.com/memononen/nanosvg
