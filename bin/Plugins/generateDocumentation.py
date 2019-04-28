import Imogen

def generateDocumentation():

	with open("documentation.md", "w") as f:
		metanodes = Imogen.GetMetaNodes()
		
		for node in metanodes:
			f.write("# "+node["name"]+"\n")
			f.write("Category : "+node["category"]+"\n")
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

		f.write("# Hot Keys\n");
		f.write("\n");
		f.write("Action    | Description         | Hot key\n");
		f.write("----------|---------------------|------------------\n");
		hotkeys = Imogen.GetHotKeys()
		for h in hotkeys:
			f.write(h["name"]+"|"+h["description"]+"|"+h["keys"]+"\n")
		f.write("\n");
	Imogen.Log("Documentation generated!\n")


Imogen.RegisterPlugin("Generate documentation", "import Plugins.generateDocumentation as plg\nplg.generateDocumentation()") 