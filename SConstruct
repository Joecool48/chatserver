import os
import glob
import atexit
inc_dir = "include"
src_dir = "src"
build_dir = "bin"
Export('inc_dir')
Export('build_dir')
Export('src_dir')
def cleanupObj():
    for obj in glob.glob(src_dir + "/" + "*.o") + glob.glob(build_dir + "/" + "*.o"):
        os.remove(obj)
atexit.register(cleanupObj) 
SConscript(src_dir + "/SConscript", variant_dir = build_dir, duplicate = 0)


