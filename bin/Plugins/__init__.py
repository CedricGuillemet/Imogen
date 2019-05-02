import Imogen
import os
import sys
import importlib

Imogen.Log("\nLoading plugins\n")

for module in os.listdir(os.path.dirname(__file__)):
    if module == '__init__.py' or module[-3:] != '.py':
        continue
    Imogen.Log("Loading {} \n".format(str(module[:-3])))
    moduleName = "Plugins."+module[:-3]
    if moduleName in sys.modules:
        importlib.reload(sys.modules[moduleName])
    else:
        __import__(moduleName, locals(), globals())
del module

Imogen.Log("Plugins loaded\n")
