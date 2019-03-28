import Imogen

def buildAll():

	libGraphs = Imogen.GetLibraryGraphs()
	for graphName in libGraphs:
		graph = Imogen.GetGraph(graphName)
		graph.Build()

	Imogen.Log("Build Done!\n")


Imogen.RegisterPlugin("Build All Materials", "import Plugins.buildAll as plg\nplg.buildAll()") 