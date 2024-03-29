#!/usr/bin/env python

import os
import subprocess

for part in ["airframe", "left_flap", "left_elevator", "left_aileron", "right_flap", "right_elevator", "right_aileron", "rudder", "canopy"]:
    out_filename = "./stl/{}.stl".format(part)
    openscad_cmd = "openscad -o {} -Dpart=\"{}\" SAILR.scad ".format(out_filename, part)
    print(openscad_cmd)
    with open(out_filename, "w+") as f:
        subprocess.call(openscad_cmd.split())

subprocess.call("blender --python subdivide.py".split())
