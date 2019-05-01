import Imogen
import os


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
		
		f.write("Action    | Description         | Hot key | b\n");
		f.write("-|-|-|-\n");

		for index in range(0, 13):
			blendImage = baseDir+"Examples"+"/Example_"+nodeName+"_"+str(index)+".png"
			blendExample(blendImage, str(index), "Graph")
			f.write("![node picture]("+blendImage+")")
			if (index % 2) == 1:
				f.write("\n")
			else:
				f.write("|")
		f.write("\n")
		
def generateDocumentation():
	baseDir = "Documentation/"
	
	metanodes = Imogen.GetMetaNodes()
	try:
		os.mkdir(baseDir)
	except OSError:
		pass
	Imogen.SetSynchronousEvaluation(True)
	for node in metanodes:
		nodeName = node["name"];
		nodeCategory = node.get("category","None")
		
		try:
			os.mkdir(baseDir+"Pictures")
		except OSError:
			pass
			
		try:
			os.mkdir(baseDir+"Examples")
		except OSError:
			pass
		
		Imogen.NewGraph("GraphFor"+nodeName)
		Imogen.AddNode(nodeName)
		Imogen.AutoLayout()
		Imogen.Render()
		Imogen.Render()
		Imogen.CaptureScreen(baseDir+"Pictures"+"/"+nodeName+".png", "Graph")
		Imogen.DeleteGraph()

		with open(baseDir+nodeCategory+".md", "a") as f:
			f.write("# "+nodeName+"\n")
			f.write("![node picture](./Pictures/"+nodeName+".png)\n\n")
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
			
	with open(baseDir + "HotKeys.md", "w") as f:
		f.write("# Default Hot Keys\n");
		f.write("\n");
		f.write("Action    | Description         | Hot key\n");
		f.write("----------|---------------------|------------------\n");
		hotkeys = Imogen.GetHotKeys()
		for h in hotkeys:
			f.write(h["name"]+"|"+h["description"]+"|"+h["keys"]+"\n")
		f.write("\n");
		
	Imogen.SetSynchronousEvaluation(False)
	Imogen.Log("Documentation generated!\n")
	


Imogen.RegisterPlugin("Generate documentation", "import Plugins.generateDocumentation as plg\nplg.generateDocumentation()") 