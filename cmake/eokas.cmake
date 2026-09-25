
message("========================================================================")
message("== EOKAS BUILD SETTINGS")
message("========================================================================")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")


cmake_host_system_information(RESULT EOKAS_OS_NAME QUERY OS_NAME)
message("EOKAS_OS_NNAME = " ${EOKAS_OS_NAME})


set(EOKAS_PROJECT_NAME "${PROJECT_NAME}")
set(EOKAS_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(EOKAS_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules")
set(EOKAS_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")
set(EOKAS_BINARY_DIR "${PROJECT_BINARY_DIR}")
message("EOKAS_PROJECT_NAME = ${EOKAS_PROJECT_NAME}")
message("EOKAS_PROJECT_DIR = ${EOKAS_PROJECT_DIR}")
message("EOKAS_MODULES_DIR = ${EOKAS_MODULES_DIR}")
message("EOKAS_BINARY_DIR = ${EOKAS_BINARY_DIR}")


macro(eokas_module MODULE_NAME)
    message("========================================================================")
    message("== EOKAS MODULE: ${MODULE_NAME}")
    message("========================================================================")
    include("${EOKAS_MODULES_DIR}/${MODULE_NAME}/module-build.cmake")
endmacro()

macro(eokas_test MODULE_NAME)
    message("========================================================================")
    message("== EOKAS TEST: ${MODULE_NAME}")
    message("========================================================================")
    include("${EOKAS_TEST_DIR}/${MODULE_NAME}/module-build.cmake")
endmacro()
