import pandas as pd
from io import StringIO

file_content = ""
with open("./resources/table.txt") as file:
    file_content = file.read()


table = pd.read_html(StringIO(file_content))[0]

wfile = open("./resources/output.txt", "w")

def has_numbers(inputString):
    return any(char.isdigit() for char in inputString)

def translatemode(str):
    if(str=="d,x"): return "ADDR_ZERO_PAGE_INDEXED_X"
    if(str=="d,y"): return "ADDR_ZERO_PAGE_INDEXED_Y"
    if(str=="a,x"): return "ADDR_ABSOLUTE_INDEXED_X"
    if(str=="a,y"): return "ADDR_ABSOLUTE_INDEXED_Y"
    if(str=="(d,x)"): return "ADDR_INDEXED_INDIRECT_X"
    if(str=="(d),y"): return "ADDR_INDEXED_INDIRECT_Y"
    if(str=="A"): return "ADDR_ACCUMULATOR"
    if(str=="#i"): return "ADDR_IMMEDIATE"
    if(str=="d"): return "ADDR_ZERO_PAGE"
    if(str=="a"): return "ADDR_ABSOLUTE"
    if(str=="*+d"): return "ADDR_RELATIVE"
    if(str=="(a)"): return "ADDR_INDIRECT"
    print("UNBEKANNTER MODUS: "+str)

opcode = 0
for index,row in table.iterrows():
    for val in row:
        vals = val.split(" ")
        if(has_numbers(vals[0])): continue
        if(len(vals)>2) : print("PROBLEM") 
        entry = "{&"
        if(len(vals)<2): mode = "ADDR_IMPLICIT"
        else: mode = translatemode(vals[1])
        hstr = hex(opcode).upper()
        hstr = hstr[2:]
        if(len(hstr)==1): hstr = "0"+hstr
        entry = entry+vals[0]+", "+mode+", 0},"
        tablen = len(entry) // 4
        tabs = ""
        for i in range(tablen, 10): tabs = tabs+"\t"
        entry = entry+tabs+"// $"+hstr+"\n"
        wfile.write(entry)
        opcode += 1


wfile.close()