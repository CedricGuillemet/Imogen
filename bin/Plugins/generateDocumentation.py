import Imogen
import os
import math
import datetime

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
    
def finishGraph(f, nodeName, baseDir):
    saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
    saveScreen(baseDir+"Pictures"+"/"+nodeName+".png", "FinalNode")
    Imogen.DeleteGraph()
    f.write("### Example\n")
    f.write("![node example](Examples"+"/Example_"+nodeName+".png"+")\n\n")

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
    Imogen.DeleteGraph()

def paletteExample(filePath, operation, content):
    Imogen.NewGraph("GraphForPalette")
    paletteNode = Imogen.AddNode("Palette")
    imageRead = Imogen.AddNode("ImageRead")
    Imogen.SetParameter(imageRead, "File name", "Media/Pictures/PartyCat.jpg")
    Imogen.Connect(imageRead, 0, paletteNode, 0)
    Imogen.SetParameter(paletteNode, "Palette", operation)
    saveScreen(filePath, content)
    Imogen.DeleteGraph()
    
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

def setDefaultCubemap(node):
    Imogen.SetParameter(node, "+X File name", "Media/EnvMaps/Fjaderholmarna/posx.jpg")
    Imogen.SetParameter(node, "-X File name", "Media/EnvMaps/Fjaderholmarna/negx.jpg")
    Imogen.SetParameter(node, "+Y File name", "Media/EnvMaps/Fjaderholmarna/posy.jpg")
    Imogen.SetParameter(node, "-Y File name", "Media/EnvMaps/Fjaderholmarna/negy.jpg")
    Imogen.SetParameter(node, "+Z File name", "Media/EnvMaps/Fjaderholmarna/posz.jpg")
    Imogen.SetParameter(node, "-Z File name", "Media/EnvMaps/Fjaderholmarna/negz.jpg")
    
def generateExample(nodeName, baseDir, f, node):
    exampleWithCatImage = ["Pixelize", "PolarCoords", "Swirl", "Crop", "Kaleidoscope", "Palette", "Blur", "Invert", "Lens", "MADD", "SmoothStep", "Clamp"]
    exampleWithCubeMap = ["CubemapView", "CubeRadiance"]
    
    if nodeName == "Blend" :
        blendExample(baseDir+"Pictures"+"/"+nodeName+".png", "0", "FinalNode")
        tab = []
        
        param = next((p for p in node["parameters"] if p["typeString"] == "Enum"), None)
        
        index = 0
        for enum in param["enum"]:
            nodeImage = "Examples/Example_"+nodeName+"_"+enum+".png"
            nodeImage = nodeImage.replace(" ", "_")
            blendExample(baseDir+nodeImage, str(index), "Graph")
            tab.append((nodeImage, "Mode " + enum))
            index = index + 1
            
        appendTable(tab, 3, f)
        return
        
    if nodeName == "Palette":
        paletteExample(baseDir+"Pictures"+"/"+nodeName+".png", "0", "FinalNode")
        tab = []
        
        param = next((p for p in node["parameters"] if p["typeString"] == "Enum"), None)
        index = 0
        for enum in param["enum"]:
            nodeImage = "Examples/Example_"+nodeName+"_"+enum+".png"
            nodeImage = nodeImage.replace(" ", "_")
            paletteExample(baseDir+nodeImage, str(index), "Graph")
            tab.append((nodeImage, "Mode " + enum))
            index = index + 1
            
        appendTable(tab, 2, f)
        return
    
    
    Imogen.NewGraph("GraphFor"+nodeName)
    node = Imogen.AddNode(nodeName)
    
    if nodeName in exampleWithCubeMap:
        imageRead = Imogen.AddNode("ImageRead")
        setDefaultCubemap(imageRead)
        Imogen.Connect(imageRead, 0, node, 0)
        if nodeName == "CubeRadiance":
            Imogen.SetParameter(node, "Mode", "1")
        if nodeName == "CubemapView":
            Imogen.SetParameter(node, "Mode", "2")
        
    elif nodeName == "ChannelPacker":
        circle = Imogen.AddNode("Circle")
        square = Imogen.AddNode("Square")
        checker = Imogen.AddNode("Checker")
        color = Imogen.AddNode("Color")
        Imogen.Connect(circle, 0, node, 0)
        Imogen.Connect(square, 0, node, 1)
        Imogen.Connect(checker, 0, node, 2)
        Imogen.Connect(color, 0, node, 3)
        
    elif nodeName == "AO":
        perlin = Imogen.AddNode("PerlinNoise")
        Imogen.Connect(perlin, 0, node, 0)
        
    elif nodeName == "Ramp":
        perlin = Imogen.AddNode("PerlinNoise")
        gradient = Imogen.AddNode("GradientBuilder")
        Imogen.Connect(perlin, 0, node, 0)
        Imogen.Connect(gradient, 0, node, 1)
    
    elif nodeName == "Warp":
        circle = Imogen.AddNode("Circle")
        imageRead = Imogen.AddNode("ImageRead")
        Imogen.SetParameter(imageRead, "File name", "Media/Pictures/PartyCat.jpg")
        Imogen.SetParameter(circle, "T", "1.0")
        Imogen.SetParameter(node, "Mode", "1")
        Imogen.Connect(imageRead, 0, node, 0)
        Imogen.Connect(circle, 0, node, 1)

    elif nodeName == "ReactionDiffusion" or nodeName == "Disolve" or nodeName == "Transform" or nodeName == "Tile" or nodeName == "NormalMap":
        circle = Imogen.AddNode("Circle")
        Imogen.Connect(circle, 0, node, 0)
        
        if nodeName == "Transform":
            Imogen.SetParameter(node, "Scale", "5.0,5.0")
            Imogen.SetParameter(node, "Rotation", "45.0")
        elif nodeName == "Tile":
            Imogen.Connect(circle, 0, node, 1)
            Imogen.SetParameter(node, "Scale", "10.0")
            Imogen.SetParameter(node, "Offset 0", "0.5")
        elif nodeName == "NormalMap":
            Imogen.SetParameter(circle, "T", "1.0")
            
    elif nodeName == "NormalMapBlending":
        circle = Imogen.AddNode("Circle")
        Imogen.SetParameter(circle, "T", "1.0")
        ngon = Imogen.AddNode("NGon")
        normal1 = Imogen.AddNode("NormalMap")
        normal2 = Imogen.AddNode("NormalMap")
        Imogen.Connect(circle, 0, normal1, 0)
        Imogen.Connect(ngon, 0, normal2, 0)
        Imogen.Connect(normal1, 0, node, 0)
        Imogen.Connect(normal2, 0, node, 1)
        
    elif nodeName == "ImageRead" :
        Imogen.SetParameter(node, "File name", "Media/Pictures/PartyCat.jpg")

    elif nodeName == "EquirectConverter" :
        imageRead = Imogen.AddNode("ImageRead")
        view = Imogen.AddNode("CubemapView")
        Imogen.SetParameter(imageRead, "File name", "Media/EnvMaps/Equirect/studio022.hdr")
        Imogen.SetParameter(view, "Mode", "2")
        Imogen.Connect(imageRead, 0, node, 0)
        Imogen.Connect(node, 0, view, 0)

    elif nodeName == "PhysicalSky" :
        view = Imogen.AddNode("CubemapView")
        Imogen.SetParameter(view, "Mode", "2")
        Imogen.Connect(node, 0, view, 0)
        
    elif nodeName == "EdgeDetect" :
        voronoi = Imogen.AddNode("Voronoi")
        Imogen.Connect(voronoi, 0, node, 0)
        
    elif nodeName in exampleWithCatImage :
        imageRead = Imogen.AddNode("ImageRead")
        Imogen.SetParameter(imageRead, "File name", "Media/Pictures/PartyCat.jpg")
        Imogen.Connect(imageRead, 0, node, 0)

    elif nodeName == "SVG" :
        Imogen.SetParameter(node, "File name", "Media/Pictures/23.svg")
    
    finishGraph(f, nodeName, baseDir)
    
def generateDocumentation():
    startTime = datetime.datetime.now()
    
    metanodes = Imogen.GetMetaNodes()
    
    # directories
    baseDir = "Documentation/"
    directories = [baseDir, baseDir+"Pictures", baseDir+"Examples"]
    for directory in directories:
        try:
            os.mkdir(directory)
        except OSError:
            pass
        
    # sync jobs
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
            generateExample(nodeName, baseDir, f, node)
        appendHotKeys(f)
        
    Imogen.SetSynchronousEvaluation(False)
    endTime = datetime.datetime.now()
    Imogen.Log("Documentation generated in {}\n".format(endTime - startTime))


Imogen.RegisterPlugin("Generate documentation", "import Plugins.generateDocumentation as plg\nplg.generateDocumentation()") 
