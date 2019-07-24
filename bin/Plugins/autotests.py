import Imogen
import os
import math
import datetime

def setDefaultCubemap(node):
    Imogen.SetParameter(node, "XPosFilename", "Media/EnvMaps/Fjaderholmarna/posx.jpg")
    Imogen.SetParameter(node, "XNegFilename", "Media/EnvMaps/Fjaderholmarna/negx.jpg")
    Imogen.SetParameter(node, "YPosFilename", "Media/EnvMaps/Fjaderholmarna/posy.jpg")
    Imogen.SetParameter(node, "YNegFilename", "Media/EnvMaps/Fjaderholmarna/negy.jpg")
    Imogen.SetParameter(node, "ZPosFilename", "Media/EnvMaps/Fjaderholmarna/posz.jpg")
    Imogen.SetParameter(node, "ZNegFilename", "Media/EnvMaps/Fjaderholmarna/negz.jpg")
    
def imageTests():
    graph = Imogen.NewGraph("ImageRead01")
    
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "filename", "Autotests/Assets/Vancouver.jpg")
    
    imageWrite = Imogen.AddNode("ImageWrite")
    Imogen.SetParameter(imageWrite, "filename", "Autotests/Run/Vancouver01.jpg")
    
    Imogen.Connect(imageRead, 0, imageWrite, 0)
    #graph.Build()
    #Imogen.DeleteGraph()

    
def autotests():
    startTime = datetime.datetime.now()
    Imogen.SetSynchronousEvaluation(True)
    
    imageTests()
    
    Imogen.SetSynchronousEvaluation(False)
    endTime = datetime.datetime.now()
    Imogen.Log("Autotests in {}\n".format(endTime - startTime))


Imogen.RegisterPlugin("Autotests", "import Plugins.autotests as plg\nplg.autotests()") 
