cmake_minimum_required(VERSION 3.22)


project(eokas VERSION 1.0)
include("cmake/eokas.cmake")


eokas_runtime_module(Base)
eokas_runtime_module(Native)
eokas_runtime_module(GPU)
eokas_runtime_module(Rose)
eokas_runtime_module(App)
eokas_runtime_module(Mesh)
eokas_runtime_module(Render)
eokas_runtime_module(UI)
eokas_runtime_module(FBX)
eokas_runtime_module(Elang)
eokas_runtime_module(ImApp)

include(CTest)
eokas_test_module(Test-Units)
eokas_test_module(Test-GPU)
eokas_test_module(Test-Render)
eokas_test_module(Test-UI)
