cmake_minimum_required(VERSION 3.22)


project(eokas VERSION 1.0)


set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")


include("cmake/eokas.cmake")
include("cmake/eokas-base.cmake")
include("cmake/eokas-native.cmake")
include("cmake/eokas-gpu.cmake")
include("cmake/eokas-rose.cmake")
include("cmake/eokas-mesh.cmake")
include("cmake/eokas-app.cmake")
include("modules/elang/module-build.cmake")
include("cmake/eokas-imapp.cmake")
include("cmake/eokas-test-gpu.cmake")
