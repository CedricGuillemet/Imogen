# Imogen
GPU Texture Generator

WIP of a GPU Texture generator using dear imgui for UI.

![Image of imogen](https://i.imgur.com/zs64qO5.png)

Simple roadmap:

~~node edit~~

~~node evaluation list~~

~~basic load/save~~

~~node parameters with preview~~

- add nodes (noise, blur, voronoi, simple ops,..) and make nice textures

Distant roadmap when I can make some nice textures and get self confident enough 
- undo/redo
- bullet proof serialization
- material/PBR preview
- texture baking / batch baking
- baking for 64k demos (node parameters + baker source/sdk)
- texture library browser
- MOAR nodes
- compute node metadata by parsing GLSL shader code
- better parameter controls (sliders, color picker,..)
- misc settings like baking dimension
- hot loading here and there
- ease of integration in your 64k demo/tools
- color coding of nodes depending of their type(generator, blur, transform, material, ...)
- node group/sub node: reuse a subnode in your new texture
- ...
