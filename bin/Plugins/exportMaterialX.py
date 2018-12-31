import Imogen
import xml.etree.ElementTree as ET

materialXParamTypes = [
    'float',
    'float2',
    'float3',
    'float4',
    'color',
    'int',
    'int2',
    'ramp',
    'angle',
    'angle2',
    'angle3',
    'angle4',
    'enum',
    'structure', # not in use
    'filename',
    'filename',
    'forceEvaluate', # not in use
    'bool',
    'ramp4',
    'camera']

def exportMaterialX():
	mat = ET.Element('materialx')  
	mat.set('version','1.36')  

	libGraphs = Imogen.GetLibraryGraphs()
	for graphName in libGraphs:
		matGraph = ET.SubElement(mat, 'nodegraph')
		matGraph.set('name', graphName)    
		graph = Imogen.GetGraph(graphName)
		nodes = graph.GetEvaluationList() 
		
		# for naming nodes
		typeCount = {} 
		nodeNames = []
		
		# first pass to gather all node names
		for node in nodes:
			t = node.GetType()
			if not t in typeCount:
				typeCount[t] = 0
			typeCount[t] = typeCount[t] + 1
			nodeNames.append(t + str(typeCount[t]))

		# second pass : export all
		nodeIndex = 0
		for node in nodes:
			t = node.GetType()
			nodeItem = ET.SubElement(matGraph, t)
			nodeItem.set('name', nodeNames[nodeIndex])
			nodeItem.set('type', 'color4')
			nodeIndex = nodeIndex + 1
			
			# export parameters
			parameters = node.GetParameters()
			for parameter in parameters:
				paramItem = ET.SubElement(nodeItem, 'parameter')
				paramItem.set('name',parameter["name"])  			
				paramItem.set('type',materialXParamTypes[parameter["type"]])
				paramItem.set('value',str(parameter["value"]))
				
			# export connection infos
			inputIndex = 0
			inputs = node.GetInputs()
			for input in inputs:
				inputItem = ET.SubElement(nodeItem, 'input')
				inputItem.set('name', 'in'+str(inputIndex))  			
				inputItem.set('type', 'color4') # as the only output is color4 ...
				inputItem.set('nodename', nodeNames[input["nodeIndex"]])
				inputIndex = inputIndex + 1

			
	# create a new XML file with the results
	mydata = ET.tostring(mat)
	myfile = open("materialx_test.xml", "wb")  
	myfile.write(mydata)
	myfile.close()

	Imogen.Log("\nDone!\n")
	
Imogen.RegisterPlugin("Export MaterialX", "Plugins.exportMaterialX.exportMaterialX();") 