#!/bin/python

import os
import sys
import subprocess

ws = sys.argv[1]
dirs = []
for (dirpath, dirnames, filenames) in os.walk(ws):
    dirs.extend(dirnames)
    break
dirs = [d for d in dirs if d.endswith("_output")]
rel_path = lambda d : ws+"/"+d
output = lambda d : rel_path(d)+"/output"
views = len(os.listdir(output(dirs[0])))//2
for i in range(views):
    subprocess.call(["mkdir", "-p", rel_path("mesh/view"+str(i))])
    subprocess.call(["mkdir", "-p", rel_path("strands/view"+str(i))])

subprocess.call(["mkdir", "-p", rel_path("ply")])

for d in dirs:
    files = []
    dir = output(d)
    for (dirpath, dirnames, filenames) in os.walk(dir):
        files.extend(filenames)
        break
    for f in files:
        if f.endswith(".ply"):
            subprocess.call(["mv", dir+"/"+f, rel_path("ply")])
            files.remove(f)
    files.sort()
    for i in range(views):
        subprocess.call(
                ["mv", dir+"/"+files[i*2], rel_path("mesh/view"+str(i))])
        subprocess.call(
                ["mv", dir+"/"+files[i*2+1], rel_path("strands/view"+str(i))])
    subprocess.call(["rmdir" , output(d)])
    subprocess.call(["rmdir" , rel_path(d)])
