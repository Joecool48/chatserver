inc_dir = "include"
src_dir = "src"
build_dir = "bin"
Export('inc_dir')
Export('build_dir')
Export('src_dir')

SConscript(src_dir + "/SConscript", variant_dir = build_dir, duplicate = 0)

