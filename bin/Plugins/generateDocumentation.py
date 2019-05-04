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
    Imogen.DeleteGraph()
    
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
    if nodeName == "ImageRead" :
        Imogen.NewGraph("GraphFor"+nodeName)
        imageRead = Imogen.AddNode(nodeName)
        Imogen.SetParameter(imageRead, "File name", "Media/Pictures/PartyCat.jpg")
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
    elif nodeName == "SVG" :
        Imogen.NewGraph("GraphFor"+nodeName)
        imageRead = Imogen.AddNode(nodeName)
        Imogen.SetParameter(imageRead, "File name", "Media/Pictures/23.svg")
        saveScreen(baseDir+"Examples"+"/Example_"+nodeName+".png", "Graph")
    elif nodeName == "Blend" :
        blendExample(baseDir+"Pictures"+"/"+nodeName+".png", "0", "FinalNode")
        
        tab = []
        for index in range(0, 13):
            blendImage = "Examples/Example_"+nodeName+"_"+str(index)+".png"
            blendExample(baseDir+blendImage, str(index), "Graph")
            tab.append((blendImage, "blend enum " + str(index)))
            
        appendTable(tab, 3, f)

        
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
            categories[nodeCategory] = []
            Imogen.NewGraph("GraphFor"+nodeName)
            Imogen.AddNode(nodeName)
            Imogen.AutoLayout()
            Imogen.Render()
            Imogen.Render()
            Imogen.CaptureScreen(baseDir+"Pictures"+"/"+nodeName+".png", "Graph")
            Imogen.DeleteGraph()
        
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
