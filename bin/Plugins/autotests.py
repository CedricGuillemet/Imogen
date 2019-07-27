import Imogen
import os
import math
import datetime

def setDefaultCubemap(node):
    Imogen.SetParameter(node, "XPosFilename", "Autotests/Assets/Lycksele/posx.jpg")
    Imogen.SetParameter(node, "XNegFilename", "Autotests/Assets/Lycksele/negx.jpg")
    Imogen.SetParameter(node, "YPosFilename", "Autotests/Assets/Lycksele/posy.jpg")
    Imogen.SetParameter(node, "YNegFilename", "Autotests/Assets/Lycksele/negy.jpg")
    Imogen.SetParameter(node, "ZPosFilename", "Autotests/Assets/Lycksele/posz.jpg")
    Imogen.SetParameter(node, "ZNegFilename", "Autotests/Assets/Lycksele/negz.jpg")
    
def imageTests():

    # read one jpg, write it back
    Imogen.NewGraph("ImageRead01")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Assets/Vancouver.jpg")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver01.jpg")
    Imogen.Connect(imageRead, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    
    #read 6 images, write the cubemap dds
    Imogen.NewGraph("ImageRead02")
    imageRead = Imogen.AddNode("ImageRead")
    setDefaultCubemap(imageRead)
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Cubemap01.dds")
    Imogen.SetParameter(imageWrite, "format", "5")
    Imogen.Connect(imageRead, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    
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
    
    Imogen.NewGraph("ImageRead05")
    physicalSky = Imogen.AddNode("PhysicalSky")
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Cubemap03.dds")
    Imogen.SetParameter(imageWrite, "format", "5")
    Imogen.Connect(physicalSky, 0, imageWrite, 0)
    Imogen.Build()
    Imogen.DeleteGraph()
    
    
    

    
def autotests():
    startTime = datetime.datetime.now()
    Imogen.SetSynchronousEvaluation(True)
    
    imageTests()
    
    Imogen.SetSynchronousEvaluation(False)
    endTime = datetime.datetime.now()
    Imogen.Log("Autotests in {}\n".format(endTime - startTime))


Imogen.RegisterPlugin("Autotests", "import Plugins.autotests as plg\nplg.autotests()") 
