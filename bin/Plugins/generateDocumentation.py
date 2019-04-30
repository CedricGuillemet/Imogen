import Imogen
import os

def generateExample(nodeName, baseDir):
	if nodeName == "ImageRead" :
		Imogen.NewGraph("GraphFor"+nodeName)
		imageRead = Imogen.AddNode(nodeName)
		Imogen.SetParameter(imageRead, "File name", "Media/Pictures/PartyCat.jpg")
		Imogen.AutoLayout()
		Imogen.Render()
		Imogen.Render()
		Imogen.Render()
		Imogen.Render()
		Imogen.Render()
		Imogen.CaptureScreen(baseDir+"Examples"+"/Example_"+nodeName+".png")
		Imogen.DeleteGraph()
		
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
		Imogen.Render()
		Imogen.Render()
		Imogen.Render()
		Imogen.CaptureScreen(baseDir+"Pictures"+"/"+nodeName+".png")
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
			generateExample(nodeName, baseDir)
			
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