# Imogen
GPU/CPU Texture Generator

GPU Texture generator using dear imgui for UI. Not production ready and a bit messy but really fun to code. This is an hybrid project that can run natively or on the web (thanks to emscripten).
Basically, add GPU and CPU nodes in a graph to manipulate and generate images.
A web build is available here : http://skaven.fr/imogen/
![Image of Imogen 0.13 Web Edition](https://i.imgur.com/ahkqR7v.png)
Currently nodes can be written in GLSL or C or Python.


![Image of Imogen 0.9](https://i.imgur.com/sQfO5Br.png)
![Image of Imogen 0.9](https://i.imgur.com/jQbx2Yu.png)

Use CMake and VisualStudio to build it. Windows and web builds are available.

Web Edition limitations:
- no threaded jobs
- no C/Python nodes
- no Python plugins
- no file load/save

Features:
- Node based texture editing
- material library browser
- edit/change node shaders inside the app
- bake textures to .png, .jpg, .tga, .bmp, .hdr, mp4
- PBR preview
- timeline for parameters animation

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

RapidJSON - https://github.com/Tencent/rapidjson

nanosvg - Mikko Mononen https://github.com/memononen/nanosvg

GLSL-PathTracer - knightcrawler25 https://github.com/knightcrawler25/GLSL-PathTracer

imgui_markdown - Juliette Foucaut https://github.com/juliettef/imgui_markdown

IconFontCppHeaders - Juliette Foucaut https://github.com/juliettef/IconFontCppHeaders

CGLTF - Johannes Kuhlmann https://github.com/jkuhlmann/cgltf

