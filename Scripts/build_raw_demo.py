"""Builds only the Demo_RawMaterials map. In the open editor's Output Log (Python):

    py "C:/Users/merts/Downloads/MyProject2/Scripts/build_raw_demo.py"
"""
import importlib
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_greybox  # noqa: E402  (does not build anything on import)

# The editor keeps modules between runs: pick up edits to the builder.
build_greybox = importlib.reload(build_greybox)

try:
    build_greybox.build_raw_demo_map()
finally:
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build_greybox.log'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(build_greybox.LOG))
