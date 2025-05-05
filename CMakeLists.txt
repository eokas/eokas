cmake_minimum_required(VERSION 3.22)


project(eokas VERSION 1.0)
include("cmake/eokas.cmake")


eokas_module(base)
eokas_module(native)
eokas_module(gpu)
eokas_module(rose)
eokas_module(app)
eokas_module(mesh)
eokas_module(fbx)
eokas_module(elang)
eokas_module(imapp)
eokas_module(test-gpu)
