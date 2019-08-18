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
    
def imageTests():

    ###################################################
    # read one jpg, write it back
    
    Imogen.NewGraph("ImageRead01")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Assets/Vancouver.jpg")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.Connect(imageRead, 0, imageWrite, 0)
    
    # free
    Imogen.SetParameter(imageWrite, "mode", "0")
    Imogen.SetParameter(imageWrite, "width", "2048")
    Imogen.SetParameter(imageWrite, "height", "2048")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver-free.jpg")
    Imogen.Build()    
    # w=512 h=keep ratio
    Imogen.SetParameter(imageWrite, "mode", "1")
    Imogen.SetParameter(imageWrite, "width", "512")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver-width512.jpg")
    Imogen.Build()
    # h= 1024 w=keep ratio
    Imogen.SetParameter(imageWrite, "mode", "2")
    Imogen.SetParameter(imageWrite, "height", "1024")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver-height1024.jpg")
    Imogen.Build()
    # same size as source
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver-same.jpg")
    Imogen.SetParameter(imageWrite, "mode", "3")
    Imogen.Build()
    #  same size as source PNG
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver-same.png")
    Imogen.SetParameter(imageWrite, "format", "1")
    Imogen.Build()
    #  same size as source TGA
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver-same.tga")
    Imogen.SetParameter(imageWrite, "format", "2")
    Imogen.Build()
    #  same size as source BMP
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver-same.bmp")
    Imogen.SetParameter(imageWrite, "format", "3")
    Imogen.Build()
    #  same size as source HDR
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver-same.hdr")
    Imogen.SetParameter(imageWrite, "format", "4")
    Imogen.Build()
    #  same size as source DDS
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver-same.dds")
    Imogen.SetParameter(imageWrite, "format", "5")
    Imogen.Build()
    #  same size as source KTX
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver-same.ktx")
    Imogen.SetParameter(imageWrite, "format", "6")
    Imogen.Build()
    #  same size as source EXR
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver-same.exr")
    Imogen.SetParameter(imageWrite, "format", "7")
    Imogen.Build()
    Imogen.DeleteGraph()
    
    ###################################################
    #read 6 images, write the cubemap dds
    Imogen.NewGraph("ImageRead02")
    imageRead = Imogen.AddNode("ImageRead")
    setDefaultCubemap(imageRead)
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Cubemap01.dds")
    Imogen.SetParameter(imageWrite, "format", "5")
    Imogen.SetParameter(imageWrite, "mode", "3")
    Imogen.Connect(imageRead, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    '''
    #read equirect hdr, convert to cubemap, write dds
    Imogen.NewGraph("ImageRead03")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Assets/studio022.hdr")
    equirect = Imogen.AddNode("EquirectConverter")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Cubemap02.dds")
    Imogen.SetParameter(imageWrite, "format", "5")
    Imogen.Connect(imageRead, 0, equirect, 0)
    Imogen.Connect(equirect, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    
    #read a cubemap dds, convert to equirect, save jpg
    Imogen.NewGraph("ImageRead04")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Run/Cubemap01.dds")
    equirect = Imogen.AddNode("EquirectConverter")
    Imogen.SetParameter(equirect, "mode", "1")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Equirect01.jpg")
    Imogen.SetParameter(imageWrite, "format", "0")
    Imogen.Connect(imageRead, 0, equirect, 0)
    Imogen.Connect(equirect, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # physical sky to dds
    Imogen.NewGraph("ImageRead05")
    physicalSky = Imogen.AddNode("PhysicalSky")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Cubemap03.dds")
    Imogen.SetParameter(imageWrite, "format", "5")
    Imogen.Connect(physicalSky, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    '''
    # circle -> png
    Imogen.NewGraph("Gen01")
    circle = Imogen.AddNode("Circle")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Circle01.png")
    Imogen.SetParameter(imageWrite, "format", "1")
    Imogen.SetParameter(imageWrite, "width", "4096")
    Imogen.SetParameter(imageWrite, "height", "4096")
    Imogen.Connect(circle, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # circle -> jpg
    Imogen.NewGraph("Gen02")
    circle = Imogen.AddNode("Circle")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Circle02.jpg")
    Imogen.SetParameter(imageWrite, "format", "0")
    Imogen.SetParameter(imageWrite, "width", "512")
    Imogen.SetParameter(imageWrite, "height", "512")
    Imogen.Connect(circle, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    
    # save cube to jpg
    
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
    
    clearTests("Autotests/Run")
        
    imageTests()
    
    Imogen.SetSynchronousEvaluation(False)
    endTime = datetime.datetime.now()
    Imogen.Log("Autotests in {}\n".format(endTime - startTime))


Imogen.RegisterPlugin("Autotests", "import Plugins.autotests as plg\nplg.autotests()") 
