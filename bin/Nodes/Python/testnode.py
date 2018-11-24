import imo
        
def test(a, b, e):
    res = imo.add(a,b)
    res2 = imo.add2(a, b)
    #e = imo.accessor_api()
    print ("result = {}".format(res))
    print ("result2 = {}".format(res2))
    print ("target is {}".format(e["target"]))
    print ("inputs is {}".format(e["inputs"]))
    img = imo.GetImage()
    #print (img)
    #imo.SaveImage(img)
    #print (imo.add)
    #help(imo)
#    return res​
	
