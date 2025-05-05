
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


function(eokas_test_setup TEST_LIB_NAME)
    message("------------------------------------------------------------------------")
    message("Test> Build Test Set '${TEST_LIB_NAME}' Start...")
    include(CTest)

    if(${BUILD_TESTING})
        set(TEST_EXEC_NAME test-${TEST_LIB_NAME})
        message("Test> Build: ${TEST_EXEC_NAME}")
        message("Test> Link: ${TEST_LIB_NAME}; ${ARGN}")

        file(GLOB TEST_ENGINE_FILES ./test/engine/*.cpp)
        file(GLOB TEST_CASES_FILES ./test/cases-${TEST_LIB_NAME}/*.cpp)
        add_executable(${TEST_EXEC_NAME} ${TEST_ENGINE_FILES} ${TEST_CASES_FILES})
        target_include_directories(${TEST_EXEC_NAME} PRIVATE ${EOKAS_SOURCE_DIR})
        target_link_libraries(${TEST_EXEC_NAME} PRIVATE ${TEST_LIB_NAME} ${ARGN})

        foreach (FILE_NAME ${TEST_CASES_FILES})
            get_filename_component (TEST_CASE_NAME ${FILE_NAME} NAME_WE)
            add_test (NAME ${TEST_CASE_NAME} COMMAND ${TEST_EXEC_NAME} ${TEST_CASE_NAME})
            message("Test> Case: ${TEST_CASE_NAME} => ${FILE_NAME}")
        endforeach ()
    endif()

    message("Test> Build Test Set '${TEST_LIB_NAME}' Done.")
endfunction()
