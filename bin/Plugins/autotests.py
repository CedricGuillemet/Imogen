import Imogen
import os
import math
import datetime
import shutil

def setDefaultCubemap(node):
    Imogen.SetParameter(node, "XPosFilename", "Autotests/Assets/Lycksele/posx.jpg")
    Imogen.SetParameter(node, "XNegFilename", "Autotests/Assets/Lycksele/negx.jpg")
    Imogen.SetParameter(node, "YPosFilename", "Autotests/Assets/Lycksele/posy.jpg")
    Imogen.SetParameter(node, "YNegFilename", "Autotests/Assets/Lycksele/negy.jpg")
    Imogen.SetParameter(node, "ZPosFilename", "Autotests/Assets/Lycksele/posz.jpg")
    Imogen.SetParameter(node, "ZNegFilename", "Autotests/Assets/Lycksele/negz.jpg")
    
def captureGraph(filename):
    Imogen.AutoLayout()
    Imogen.Render()
    Imogen.Render()
    Imogen.Render()
    Imogen.CaptureScreen(filename, "Graph")    
    
def imageTests(outDir):
    metanodes = Imogen.GetMetaNodes()
    Imogen.OpenLibrary(Imogen.NewLibrary(outDir, "tempLibrary", False), False)
    
    ###################################################
    # read one jpg, write it back
    Imogen.NewGraph("ImageRead01")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Assets/Vancouver.jpg")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.Connect(imageRead, 0, imageWrite, 0)
    
    # free
    Imogen.SetParameters(imageWrite, {"filename":outDir+"Vancouver-free.jpg", "mode": 0, "width": 2048, "height": 2048})
    Imogen.Build()    
    # w=512 h=keep ratio
    Imogen.SetParameters(imageWrite, {"mode":1, "width": 512, "filename": outDir+"Vancouver-width512.jpg"})
    Imogen.Build()
    # h= 1024 w=keep ratio
    Imogen.SetParameters(imageWrite, {"mode":2, "height": 1024, "filename": outDir+"Vancouver-height1024.jpg"})
    Imogen.Build()
    # same size as source
    Imogen.SetParameters(imageWrite, {"mode":3, "filename": outDir+"Vancouver-same.jpg"})
    Imogen.Build()
    #  same size as source PNG
    Imogen.SetParameters(imageWrite, {"format":1, "filename": outDir+"Vancouver-same.png"})
    Imogen.Build()
    #  same size as source TGA
    Imogen.SetParameters(imageWrite, {"format":2, "filename": outDir+"Vancouver-same.tga"})
    Imogen.Build()
    #  same size as source BMP
    Imogen.SetParameters(imageWrite, {"format":3, "filename": outDir+"Vancouver-same.bmp"})
    Imogen.Build()
    #  same size as source HDR
    Imogen.SetParameters(imageWrite, {"format":4, "filename": outDir+"Vancouver-same.hdr"})
    Imogen.Build()
    #  same size as source DDS
    Imogen.SetParameters(imageWrite, {"format":5, "filename": outDir+"Vancouver-same.dds"})
    Imogen.Build()
    #  same size as source KTX
    Imogen.SetParameters(imageWrite, {"format":6, "filename": outDir+"Vancouver-same.ktx"})
    Imogen.Build()
    #  same size as source EXR
    Imogen.SetParameters(imageWrite, {"format":7, "filename": outDir+"Vancouver-same.exr"})
    Imogen.Build()
    Imogen.DeleteGraph()
    
    ###################################################
    #read 6 images, write the cubemap dds
    Imogen.NewGraph("ImageRead02")
    imageRead = Imogen.AddNode("ImageRead")
    setDefaultCubemap(imageRead)
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameters(imageWrite, {"mode":3, "format":5, "filename": outDir+"Cubemap01.dds"})
    Imogen.Connect(imageRead, 0, imageWrite, 0)
    Imogen.Build()
    # jpg from cubemap
    Imogen.SetParameters(imageWrite, {"mode":3, "format":0, "filename": outDir+"JpgFromCubemap.jpg"})
    Imogen.Build()
    Imogen.DeleteGraph()
    
    #read equirect hdr, convert to cubemap, write dds
    Imogen.NewGraph("ImageRead03")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Assets/studio022.hdr")
    equirect = Imogen.AddNode("EquirectConverter")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", outDir+"Equirect2cubemap.dds")
    Imogen.SetParameter(imageWrite, "format", "5")
    Imogen.Connect(imageRead, 0, equirect, 0)
    Imogen.Connect(equirect, 0, imageWrite, 0)
    Imogen.SetParameter(imageWrite, "width", "256")
    Imogen.SetParameter(imageWrite, "height", "256")
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # read cube, save same cube
    Imogen.NewGraph("ImageRead03b")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", outDir+"Cubemap01.dds")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameters(imageWrite, {"mode":3, "format":5, "filename": outDir+"SameCubemap01.dds"})
    Imogen.Connect(imageRead, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    
    #################################################
    #read a cubemap dds, convert to equirect, save jpg
    Imogen.NewGraph("ImageRead04")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", outDir+"Cubemap01.dds")
    equirect = Imogen.AddNode("EquirectConverter")
    Imogen.SetParameter(equirect, "mode", "1")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", outDir+"EquirectFromCubemap01.jpg")
    Imogen.SetParameter(imageWrite, "format", "0")
    Imogen.Connect(imageRead, 0, equirect, 0)
    Imogen.Connect(equirect, 0, imageWrite, 0)
    Imogen.SetParameter(imageWrite, "width", "512")
    Imogen.SetParameter(imageWrite, "height", "256")
    Imogen.Build()
    
    # test with a resulting dds
    Imogen.SetParameter(imageRead, "filename", outDir+"Equirect2cubemap.dds")
    Imogen.SetParameter(imageWrite, "filename", outDir+"EquirectFromCubemap02.jpg")
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # physical sky to dds
    Imogen.NewGraph("ImageRead05")
    physicalSky = Imogen.AddNode("PhysicalSky")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "format":5, "filename": outDir+"Cubemap-PhysicalSky.dds"})
    Imogen.Connect(physicalSky, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # physical sky to equirect-jpg
    Imogen.NewGraph("ImageRead05")
    physicalSky = Imogen.AddNode("PhysicalSky")
    equirect = Imogen.AddNode("EquirectConverter")
    Imogen.SetParameter(equirect, "mode", "1")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameters(imageWrite, {"width":512, "height":256, "format":0, "filename": outDir+"EquirectFromCubemap03.jpg"})
    Imogen.Connect(physicalSky, 0, equirect, 0)
    Imogen.Connect(equirect, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # circle -> png
    Imogen.NewGraph("Gen01")
    circle = Imogen.AddNode("Circle")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameters(imageWrite, {"width":4096, "height":4096, "format":1, "filename": outDir+"Circle01.png"})
    Imogen.Connect(circle, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.SetParameters(imageWrite, {"width":512, "height":512, "format":0, "filename": outDir+"Circle02.jpg"})
    Imogen.Build()
    Imogen.DeleteGraph()
    
    ###########################################
    # crop -> jpg
    Imogen.NewGraph("Crop01")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Assets/Vancouver.jpg")
    imageWrite = Imogen.AddNode("ImageWrite")
    crop = Imogen.AddNode("Crop")
    Imogen.Connect(imageRead, 0, crop, 0)
    Imogen.Connect(crop, 0, imageWrite, 0)
    # 1K
    Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "mode":0, "filename": outDir+"crop-1024.jpg"})
    Imogen.Build()  
    # as source
    Imogen.SetParameter(imageWrite, "mode", "3")
    Imogen.SetParameter(imageWrite, "filename", outDir+"crop-sourceSize.jpg")
    Imogen.Build()  
    Imogen.DeleteGraph()
    

    ###########################################
    # channel packer to png
    Imogen.NewGraph("Packer")
    packer = Imogen.AddNode("ChannelPacker")
    circle = Imogen.AddNode("Circle")
    ngon = Imogen.AddNode("NGon")
    checker = Imogen.AddNode("Checker")
    Sine = Imogen.AddNode("Sine")
    Imogen.Connect(circle, 0, packer, 0)
    Imogen.Connect(ngon, 0, packer, 1)
    Imogen.Connect(checker, 0, packer, 2)
    Imogen.Connect(Sine, 0, packer, 3)
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.Connect(packer, 0, imageWrite, 0)
    Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "format":1, "filename": outDir+"ChannelPacker.png"})
    Imogen.Build()  
    Imogen.DeleteGraph()
    
    ###########################################
    # blur
    Imogen.NewGraph("Blur")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Assets/Vancouver.jpg")
    imageWrite = Imogen.AddNode("ImageWrite")
    blur = Imogen.AddNode("Blur")
    Imogen.Connect(imageRead, 0, blur, 0)
    Imogen.Connect(blur, 0, imageWrite, 0)
    Imogen.SetParameter(blur, "strength", "0.001")
    # 1 dir pass
    Imogen.SetParameter(blur, "angle", "22.5")
    Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "mode":0, "filename": outDir+"blur-directional-1pass.jpg"})
    Imogen.Build()
    # 30 dir passes
    Imogen.SetParameter(blur, "passCount", "30")
    Imogen.SetParameter(imageWrite, "filename", outDir+"blur-directional-30passes.jpg")
    Imogen.Build()
    # 1 box pass
    Imogen.SetParameter(blur, "type", "1")
    Imogen.SetParameter(blur, "passCount", "1")
    Imogen.SetParameter(imageWrite, "filename", outDir+"blur-box-1pass.jpg")
    Imogen.Build()
    # 30 box pass
    Imogen.SetParameter(blur, "passCount", "30")
    Imogen.SetParameter(imageWrite, "filename", outDir+"blur-box-30passes.jpg")
    Imogen.Build()
    Imogen.DeleteGraph()
    
    ###########################################
    # voronoi from image
    Imogen.NewGraph("Voronoi")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Assets/Vancouver.jpg")
    imageWrite = Imogen.AddNode("ImageWrite")
    voronoi = Imogen.AddNode("Voronoi")
    Imogen.Connect(imageRead, 0, voronoi, 0)
    Imogen.Connect(voronoi, 0, imageWrite, 0)
    Imogen.SetParameters(voronoi, {"size":30, "noise":0.5, "colorInterpolation":0})
    Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "mode":0, "filename": outDir+"Voronoi-image.jpg"})
    Imogen.Build()
    Imogen.DeleteGraph()
    # noise generator
    Imogen.NewGraph("Voronoi")
    imageWrite = Imogen.AddNode("ImageWrite")
    voronoi = Imogen.AddNode("Voronoi")
    Imogen.Connect(voronoi, 0, imageWrite, 0)
    Imogen.SetParameters(voronoi, {"size":20, "noise":1.0, "colorInterpolation":1})
    # from image
    Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "mode":0, "filename": outDir+"Voronoi-noise.jpg"})
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # circle -> distance -> png
    Imogen.NewGraph("Distance01")
    circle = Imogen.AddNode("Circle")
    distance = Imogen.AddNode("Distance")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameters(imageWrite, {"width":4096, "height":4096, "format":1, "filename": outDir+"Distance01.png"})
    Imogen.Connect(circle, 0, distance, 0)
    Imogen.Connect(distance, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # palette
    Imogen.NewGraph("Palette")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Assets/PartyCat.jpg")
    imageWrite = Imogen.AddNode("ImageWrite")
    palette = Imogen.AddNode("Palette")
    Imogen.Connect(imageRead, 0, palette, 0)
    Imogen.Connect(palette, 0, imageWrite, 0)
    Imogen.SetParameter(palette, "palette", "0")
    Imogen.SetParameter(palette, "ditherStrength", "0.0")
    Imogen.SetParameter(imageWrite, "format", "1")
    Imogen.SetParameter(imageWrite, "mode", "3")
    
    for pal in range(0, 10):
        Imogen.SetParameter(palette, "palette", "{}".format(pal))
        Imogen.SetParameter(imageWrite, "filename", (outDir+"Palette-{}.png").format(pal));
        Imogen.Build()
    Imogen.SetParameter(palette, "palette", "0")    
    for dither in range(0, 10):
        Imogen.SetParameter(palette, "ditherStrength", "{}".format(dither/10.0))
        Imogen.SetParameter(imageWrite, "filename", (outDir+"DitherPalette-0-{}.png").format(dither/10.0));
        Imogen.Build()

    # samplers
    Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "format":1, "mode":0, "filename": outDir+"Palette_1024_linear.png"})
    Imogen.Build()
    Imogen.SetSamplers(imageWrite, 0, {"filterMin" : 1})
    Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "format":1, "mode":0, "filename": outDir+"Palette_1024_nearest.png"})
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # reaction diffusion
    Imogen.NewGraph("ReactionDiffusion")
    circle = Imogen.AddNode("Circle")
    imageWrite = Imogen.AddNode("ImageWrite")
    reactionDiffusion = Imogen.AddNode("ReactionDiffusion")
    Imogen.SetParameters(imageWrite, {"width":4096, "height":4096, "format":1, "filename": outDir+"ReactionDiffusion01.png"})
    Imogen.Connect(circle, 0, reactionDiffusion, 0)
    Imogen.Connect(reactionDiffusion, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.SetParameters(reactionDiffusion, {"boost":0.72, "divisor":3.9, "colorStep":0.01})
    Imogen.SetParameter(imageWrite, "filename", outDir+"ReactionDiffusion02.png")
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # gltf read
    Imogen.NewGraph("GLTF01")
    gltf = Imogen.AddNode("GLTFRead")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(gltf, "filename", "Autotests/Assets/CartoonHead.gltf/scene.gltf")
    Imogen.SetCameraLookAt(gltf, 2., 1., 3.,0.,-1.,0.);
    Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "format":0, "filename": outDir+"gltf01.jpg"})
    Imogen.Connect(gltf, 0, imageWrite, 0)
    Imogen.Build()
    #Imogen.DeleteGraph()
    
    # blend
    Imogen.NewGraph("GraphForBlend")
    blendNode = Imogen.AddNode("Blend")
    imageRead = Imogen.AddNode("ImageRead")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Assets/PartyCat.jpg")
    sineNode = Imogen.AddNode("Sine")
    Imogen.Connect(imageRead, 0, blendNode, 0)
    Imogen.Connect(sineNode, 0, blendNode, 1)
    Imogen.Connect(blendNode, 0, imageWrite, 0)
    
    for node in metanodes:
        nodeName = node["name"];
        if nodeName == "Blend":
            param = next((p for p in node["parameters"] if p["typeString"] == "Enum"), None)
            index = 0
            for enum in param["enum"]:
                Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "mode":0, "filename": outDir+"Blend-"+enum+".jpg"})
                Imogen.SetParameters(blendNode, {"operation":index})
                index = index + 1
                Imogen.Build()
    Imogen.DeleteGraph()
    
    # cubemap radiance filter
    
    Imogen.NewGraph("CubeRadiance")
    imageRead = Imogen.AddNode("ImageRead")
    radiance = Imogen.AddNode("CubeRadiance")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.Connect(imageRead, 0, radiance, 0)
    Imogen.Connect(radiance, 0, imageWrite, 0)
    setDefaultCubemap(imageRead)
    Imogen.SetParameters(imageWrite, {"mode":3, "format":5, "filename": outDir+"CubemapRadiance.dds"})
    Imogen.SetParameters(radiance, {"mode":0, "size":1, "sampleCount":1000})
    Imogen.Build()
    Imogen.SetParameters(imageWrite, {"mode":3, "format":5, "filename": outDir+"CubemapIrradiance.dds"})
    Imogen.SetParameters(radiance, {"mode":1, "size":1, "sampleCount":1000})
    Imogen.Build()
    Imogen.DeleteGraph()
    
    Imogen.NewGraph("CubeRadiance")
    imageRead = Imogen.AddNode("ImageRead")
    radiance = Imogen.AddNode("CubeRadiance")
    equirect = Imogen.AddNode("EquirectConverter")
    Imogen.SetParameter(equirect, "mode", "1")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.Connect(imageRead, 0, radiance, 0)
    Imogen.Connect(radiance, 0, equirect, 0)
    Imogen.Connect(equirect, 0, imageWrite, 0)
    setDefaultCubemap(imageRead)
    Imogen.SetParameters(imageWrite, {"width":512, "height":256, "mode":0, "format":1, "filename": outDir+"EquirectRadiance.png"})
    Imogen.SetParameters(radiance, {"mode":0, "size":1, "sampleCount":1000})
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # thumbnail
    Imogen.NewGraph("ThumbnailTest")
    circle = Imogen.AddNode("Circle")
    thumbnail = Imogen.AddNode("Thumbnail")
    Imogen.Connect(circle, 0, thumbnail, 0)
    Imogen.Build()
    thumbnailImage = Imogen.GetThumbnailImage("ThumbnailTest")
    Imogen.WriteImage(outDir+"Thumbnail_1.png", thumbnailImage, 1, 0);
    Imogen.DeleteGraph()
    
    # multiplex !!
    Imogen.NewGraph("Multiplex")
    circle = Imogen.AddNode("Circle")
    ngon = Imogen.AddNode("NGon")
    checker = Imogen.AddNode("Checker")
    sine = Imogen.AddNode("Sine")
    multiplexLeft = Imogen.AddNode("Multiplex")
    multiplexRight = Imogen.AddNode("Multiplex")
    blend = Imogen.AddNode("Blend")
    imageWrite = Imogen.AddNode("ImageWrite")
    
    Imogen.Connect(checker, 0, multiplexLeft, 0)
    Imogen.Connect(checker, 0, multiplexRight, 4)
    Imogen.Connect(sine, 0, multiplexLeft, 2)
    Imogen.Connect(ngon, 0, multiplexLeft, 6)
    
    Imogen.Connect(circle, 0, multiplexRight, 1)
    Imogen.Connect(circle, 0, blend, 0)
    
    Imogen.Connect(multiplexLeft, 0, multiplexRight, 3)
    Imogen.Connect(multiplexRight, 0, blend, 1)
    
    Imogen.Connect(blend, 0, imageWrite, 0)
    
    Imogen.Connect(ngon, 0, multiplexRight, 7) # todelete
    
    captureGraph(outDir+"MultiplexGraph_0.png")
    
    Imogen.Disconnect(multiplexRight, 7)
    captureGraph(outDir+"MultiplexGraph_1.png")
    
    # get what's connected
    
    assert Imogen.GetMultiplexList(blend, 0) == [], "List must be empty"
    assert Imogen.GetMultiplexList(imageWrite, 0) == [], "List must be empty"
    assert Imogen.GetMultiplexList(blend, 1) == [0, 1, 2, 3], "List is incorrect"
    assert Imogen.GetMultiplexList(blend, 2) == [], "List must be empty"
    assert Imogen.GetMultiplexList(blend, 200) == [], "List must be empty"
    assert Imogen.GetSelectedMultiplex(blend, 0) == -1, "Wrong multiplex"
    assert Imogen.GetSelectedMultiplex(blend, 1) == -1, "Wrong multiplex"
    assert Imogen.GetSelectedMultiplex(blend, 2) == -1, "Wrong multiplex"
    assert Imogen.GetSelectedMultiplex(blend, 200) == -1, "Wrong multiplex"
    
    Imogen.SelectMultiplexIndex(1000, 1000, 1000)
    Imogen.SelectMultiplexIndex(blend, 1000, -18)
    Imogen.SelectMultiplexIndex(blend, 0, -18)
    Imogen.SelectMultiplexIndex(blend, 0, 0)
    Imogen.SelectMultiplexIndex(blend, 0, 1)
    Imogen.SelectMultiplexIndex(blend, 1, -1)
    Imogen.SelectMultiplexIndex(blend, 1, 18)
    Imogen.SelectMultiplexIndex(blend, 1, 1)
    assert Imogen.GetSelectedMultiplex(blend, 1) == 1, "Wrong multiplexed value"
    Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "mode":0, "format":0, "filename": outDir+"Multiplex_build0.jpg"})
    Imogen.Build()
    
    Imogen.SelectMultiplex(blend, 1, 300)
    Imogen.SelectMultiplex(blend, 1, 2)
    assert Imogen.GetSelectedMultiplex(blend, 1) == 2, "Wrong multiplexed value"
    Imogen.SetParameters(imageWrite, {"width":1024, "height":1024, "mode":0, "format":0, "filename": outDir+"Multiplex_build1.jpg"})
    Imogen.Build()
   
    Imogen.DelNode(2)
    Imogen.SetParameters(imageWrite-1, {"width":1024, "height":1024, "mode":0, "format":0, "filename": outDir+"Multiplex_build2.jpg"})
    Imogen.Build()
    captureGraph(outDir+"MultiplexGraph_2.png")
    
    assert Imogen.GetSelectedMultiplex(blend, 1) == -1, "Wrong multiplexed value after node deletion"
    
    Imogen.CloseCurrentLibrary()

    
def clearTests(folder):
    for the_file in os.listdir(folder):
        file_path = os.path.join(folder, the_file)
        try:
            if os.path.isfile(file_path):
                os.unlink(file_path)
            #elif os.path.isdir(file_path): shutil.rmtree(file_path)
        except Exception as e:
            print(e)
    

    
def autotests():
    startTime = datetime.datetime.now()
    Imogen.SetSynchronousEvaluation(True)
    
    outDir = "Autotests/Run/"+Imogen.GetRendererType()+"/"
    clearTests(outDir)
        
    imageTests(outDir)
    
    Imogen.SetSynchronousEvaluation(False)
    endTime = datetime.datetime.now()
    Imogen.Log("Autotests in {}\n".format(endTime - startTime))


Imogen.RegisterPlugin("Autotests", "import Plugins.autotests as plg\nplg.autotests()") 
