import Imogen
import os
import math

def appendHotKeys(f):
    f.write("# Default Hot Keys\n");
    f.write("\n");
    f.write("Action|Description|Hot key\n");
    f.write("-|-|-\n");
    hotkeys = Imogen.GetHotKeys()
    for h in hotkeys:
        f.write(h["name"]+"|"+h["description"]+"|"+h["keys"]+"\n")
    f.write("\n");

def saveScreen(filePath, content):
    Imogen.AutoLayout()
    Imogen.Render()
    Imogen.Render()
    Imogen.CaptureScreen(filePath, content)
    
def blendExample(filePath, operation, content):
    Imogen.NewGraph("GraphForBlend")
    blendNode = Imogen.AddNode("Blend")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "File name", "Media/Pictures/PartyCat.jpg")
    sineNode = Imogen.AddNode("Sine")
    Imogen.Connect(imageRead, 0, blendNode, 0)
    Imogen.Connect(sineNode, 0, blendNode, 1)
    Imogen.SetParameter(blendNode, "Operation", operation)
    saveScreen(filePath, content)
    #Imogen.DeleteGraph()


def appendTable(tab, lineSize, f, makeLink = False):
    lt = len(tab)
    lineSize = lineSize if lineSize < lt else lt
    rowCount = int(math.ceil(float(lt)/float(lineSize)))
    for t in range(0, rowCount):
        for r in range(0,2):
            if r and not t:
                f.write(("-|"*(lineSize-1 if lt >= lineSize else lt))+"-\n");
            for i in range(0, lineSize):
                if i:
                    f.write("|")
                index = t * lineSize + i
                if  index < lt:
                    if r:
                        if makeLink:
                            f.write("[{}](#{})".format(tab[index][1], tab[index][1]))
                        else:
                            f.write(tab[index][1])
                    else:
                        f.write("![node picture]("+tab[index][0]+")")
                
            f.write("\n")
    f.write("\n\n")

def generateExample(nodeName, baseDir, f):
    exampleWithCatImage = ["Pixelize", "PolarCoords", "Swirl", "Crop", "Kaleidoscope", "Palette", "Blur", "Invert", "Lens", "MADD", "SmoothStep", "Clamp"]
    exampleWithCubeMap = ["CubemapView", "CubeRadiance"]
    
    if nodeName in exampleWithCubeMap:
        Imogen.NewGraph("GraphFor"+nodeName)
        imageRead = Imogen.AddNode("ImageRead")
        Imogen.SetParameter(imageRead, "+X File name", "Media/EnvMaps/Fjaderholmarna/posx.jpg")
        Imogen.SetParameter(imageRead, "-X File name", "Media/EnvMaps/Fjaderholmarna/negx.jpg")
        Imogen.SetParameter(imageRead, "+Y File name", "Media/EnvMaps/Fjaderholmarna/posy.jpg")
        Imogen.SetParameter(imageRead, "-Y File name", "Media/EnvMaps/Fjaderholmarna/negy.jpg")
        Imogen.SetParameter(imageRead, "+Z File name", "Media/EnvMaps/Fjaderholmarna/posz.jpg")
        Imogen.SetParameter(imageRead, "-Z File name", "Media/EnvMaps/Fjaderholmarna/negz.jpg")
        
        exNode = Imogen.AddNode(nodeName)
        Imogen.Connect(imageRead, 0, exNode, 0)
        if nodeName == "CubeRadiance":
            Imogen.SetParameter(exNode, "Mode", "1")
        if nodeName == "CubemapView":
            Imogen.SetParameter(exNode, "Mode", "2")
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")
        
    elif nodeName == "ChannelPacker":
        Imogen.NewGraph("GraphFor"+nodeName)
        circle = Imogen.AddNode("Circle")
        square = Imogen.AddNode("Square")
        checker = Imogen.AddNode("Checker")
        color = Imogen.AddNode("Color")
        packer = Imogen.AddNode("ChannelPacker")
        Imogen.Connect(circle, 0, packer, 0)
        Imogen.Connect(square, 0, packer, 1)
        Imogen.Connect(checker, 0, packer, 2)
        Imogen.Connect(color, 0, packer, 3)
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")
        
    elif nodeName == "AO":
        Imogen.NewGraph("GraphFor"+nodeName)
        perlin = Imogen.AddNode("PerlinNoise")
        ao = Imogen.AddNode("AO")
        Imogen.Connect(perlin, 0, ao, 0)
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")
        
    elif nodeName == "Ramp":
        Imogen.NewGraph("GraphFor"+nodeName)
        perlin = Imogen.AddNode("PerlinNoise")
        gradient = Imogen.AddNode("GradientBuilder")
        ramp = Imogen.AddNode("Ramp")
        Imogen.Connect(perlin, 0, ramp, 0)
        Imogen.Connect(gradient, 0, ramp, 1)
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")
    
    elif nodeName == "Warp":
        Imogen.NewGraph("GraphFor"+nodeName)
        circle = Imogen.AddNode("Circle")
        imageRead = Imogen.AddNode("ImageRead")
        exNode = Imogen.AddNode(nodeName)
        Imogen.SetParameter(imageRead, "File name", "Media/Pictures/PartyCat.jpg")
        Imogen.SetParameter(circle, "T", "1.0")
        Imogen.SetParameter(exNode, "Mode", "1")
        Imogen.Connect(imageRead, 0, exNode, 0)
        Imogen.Connect(circle, 0, exNode, 1)
            
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")

    elif nodeName == "ReactionDiffusion" or nodeName == "Disolve" or nodeName == "Transform" or nodeName == "Tile" or nodeName == "NormalMap":
        Imogen.NewGraph("GraphFor"+nodeName)
        circle = Imogen.AddNode("Circle")
        exNode = Imogen.AddNode(nodeName)
        Imogen.Connect(circle, 0, exNode, 0)
        
        if nodeName == "Transform":
            Imogen.SetParameter(exNode, "Scale", "5.0,5.0")
            Imogen.SetParameter(exNode, "Rotation", "45.0")
        elif nodeName == "Tile":
            Imogen.Connect(circle, 0, exNode, 1)
            Imogen.SetParameter(exNode, "Scale", "10.0")
            Imogen.SetParameter(exNode, "Offset 0", "0.5")
        elif nodeName == "NormalMap":
            Imogen.SetParameter(circle, "T", "1.0")
            
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")
    elif nodeName == "NormalMapBlending":
        Imogen.NewGraph("GraphFor"+nodeName)
        circle = Imogen.AddNode("Circle")
        Imogen.SetParameter(circle, "T", "1.0")
        ngon = Imogen.AddNode("NGon")
        normal1 = Imogen.AddNode("NormalMap")
        normal2 = Imogen.AddNode("NormalMap")
        Imogen.Connect(circle, 0, normal1, 0)
        Imogen.Connect(ngon, 0, normal2, 0)
        normalBlend = Imogen.AddNode("NormalMapBlending")
        Imogen.Connect(normal1, 0, normalBlend, 0)
        Imogen.Connect(normal2, 0, normalBlend, 1)
        
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")
    elif nodeName == "ImageRead" :
        Imogen.NewGraph("GraphFor"+nodeName)
        imageRead = Imogen.AddNode(nodeName)
        Imogen.SetParameter(imageRead, "File name", "Media/Pictures/PartyCat.jpg")
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")
    elif nodeName == "EquirectConverter" :
        Imogen.NewGraph("GraphFor"+nodeName)
        imageRead = Imogen.AddNode("ImageRead")
        converter = Imogen.AddNode(nodeName)
        Imogen.SetParameter(imageRead, "File name", "Media/EnvMaps/Equirect/studio022.hdr")
        Imogen.Connect(imageRead, 0, converter, 0)
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")
    elif nodeName == "EdgeDetect" :
        Imogen.NewGraph("GraphFor"+nodeName)
        voronoi = Imogen.AddNode("Voronoi")
        edgeDetect = Imogen.AddNode("EdgeDetect")
        Imogen.Connect(voronoi, 0, edgeDetect, 0)
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")
        
    elif nodeName in exampleWithCatImage :
        Imogen.NewGraph("GraphFor"+nodeName)
        imageRead = Imogen.AddNode("ImageRead")
        Imogen.SetParameter(imageRead, "File name", "Media/Pictures/PartyCat.jpg")
        exNode = Imogen.AddNode(nodeName)
        Imogen.Connect(imageRead, 0, exNode, 0)
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")

    elif nodeName == "SVG" :
        Imogen.NewGraph("GraphFor"+nodeName)
        imageRead = Imogen.AddNode(nodeName)
        Imogen.SetParameter(imageRead, "File name", "Media/Pictures/23.svg")
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")

    elif nodeName == "Blend" :
        blendExample(baseDir+"Pictures"+"/"+nodeName+".png", "0", "FinalNode")
        
        tab = []
        for index in range(0, 12):
            blendImage = "Examples/Example_"+nodeName+"_"+str(index)+".png"
            blendExample(baseDir+blendImage, str(index), "Graph")
            tab.append((blendImage, "blend enum " + str(index)))
            
        appendTable(tab, 3, f)
        
    else:
        Imogen.NewGraph("GraphFor"+nodeName)
        imageRead = Imogen.AddNode(nodeName)
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
        saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
        #Imogen.DeleteGraph()
        f.write("### Example\n")
        f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")
    
    
def generateDocumentation():
    baseDir = "Documentation/"
    
    metanodes = Imogen.GetMetaNodes()
    try:
        os.mkdir(baseDir)
    except OSError:
        pass
        
    try:
        os.mkdir(baseDir+"Pictures")
    except OSError:
        pass
        
    try:
        os.mkdir(baseDir+"Examples")
    except OSError:
        pass
    
    Imogen.SetSynchronousEvaluation(True)
    
    with open(baseDir+"ImogenUserDocumentation.md", "w") as f:
        f.write("# Imogen - User documentation\n\n\n")
        f.write("1. [Nodes](#Nodes)\n")
        f.write("1. [Default Hot Keys](#Default-Hot-Keys)\n")
        
        f.write("\n\n")
        f.write("# Nodes\n")
        
        # first pass to get categories/picture
        categories={}
        for node in metanodes:
            nodeCategory = node.get("category","None")
            categories[nodeCategory]=[]
            
        for node in metanodes:
            nodeName = node["name"];
            nodeCategory = node.get("category","None")
            nodePicture = "Pictures/"+nodeName+".png"
            categories[nodeCategory].append((nodePicture, nodeName))
            
        # write nodes table
        for categoryName, categoryNodes in categories.items():
            f.write("## "+categoryName+"\n")
            appendTable(categoryNodes, 6, f, True)
            
            
        # write notes
        for node in metanodes:
            nodeName = node["name"];
            nodeCategory = node.get("category","None")
            
            f.write("## "+nodeName+"\n")
            nodePicture = "Pictures/"+nodeName+".png"
            f.write("![node picture]("+nodePicture+")\n\n")
            f.write("Category : "+nodeCategory+"\n")
            f.write("### Description\n")
            f.write(node["description"]+"\n")
            f.write("### Parameters\n")
            parameters = node.get("parameters", None)
            if parameters is None:
                f.write("No parameter for this node.\n")
            else:
                for param in parameters:
                    f.write("1. " + param["name"]+"\n")
                    f.write(param["description"]+"\n")
            f.write("\n");
            generateExample(nodeName, baseDir, f)
        appendHotKeys(f)
        
    Imogen.SetSynchronousEvaluation(False)
    Imogen.Log("Documentation generated!\n")


Imogen.RegisterPlugin("Generate documentation", "import Plugins.generateDocumentation as plg\nplg.generateDocumentation()") 
