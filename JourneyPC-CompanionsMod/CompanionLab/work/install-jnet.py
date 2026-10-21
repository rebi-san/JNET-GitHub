import argparse
from pathlib import Path
import re
import runpy
import sys


parser = argparse.ArgumentParser(description="Install a prepared JNET release into approved test bottles.")
parser.add_argument("version")
parser.add_argument("bottle", choices=("devishaz", "rebishaz", "smooviepuffs", "all"))
args = parser.parse_args()
if re.fullmatch(r"0\.[0-9]+", args.version) is None:
    parser.error("Expected a JNET version such as 0.243")
work = Path(__file__).resolve().parent
installer = work / ("install-" + args.version + ".py")
if installer.is_symlink() or not installer.is_file() or installer.resolve().parent != work:
    parser.error("No local installer prepared for this version")
sys.argv = [str(installer), args.bottle]
runpy.run_path(str(installer), run_name="__main__")
