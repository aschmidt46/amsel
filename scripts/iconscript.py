from PIL import Image

f = open("../include/icon.h", 'w+')
f.write("#pragma once\n")
f.write("#include <cstdint>\n")

with Image.open("../resources/amsel.png") as im:
    h = 400
    w = 400
    c = 4
    vs = h*w*c
    f.write("unsigned int iconSize = " + str(vs) + ";")
    f.write("uint8_t icon["+str(vs)+"] = {")
    for y in range(0, h):
        for x in range(0, w):
            p = im.getpixel([x,y])
            for col in range(0,c):
                hx = hex(p[col])
                f.write(hx + ",")
    f.write("};\n")
