import Imogen
import os

Imogen.Log("\nLoading plugins\n")

for module in os.listdir(os.path.dirname(__file__)):
    if module == '__init__.py' or module[-3:] != '.py':
        continue
    Imogen.Log("Loading {} \n".format(str(module[:-3])))
    __import__("Plugins."+module[:-3], locals(), globals())
del module

Imogen.Log("Plugins loaded\n")
