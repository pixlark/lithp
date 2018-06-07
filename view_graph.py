import os
import subprocess

with open("graph.viz", "r") as f:
	graph_repr = f.read()

subprocess.run(["dot", "-Tpng", "-o", "graph.png"], input=graph_repr.encode())

subprocess.run(["feh", "graph.png"])
