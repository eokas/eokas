
set(EOKAS_TARGET_NAME "test-gpu")
set(EOKAS_TARGET_DIR "${EOKAS_MODULES_DIR}/test-gpu")

message("============================================================================")
message("EOKAS_TARGET_NAME = ${EOKAS_TARGET_NAME}")
message("EOKAS_TARGET_DIR = ${EOKAS_TARGET_DIR}")

set(EOKAS_HEADER_DIRS
        "${EOKAS_MODULES_DIR}"
)

set(EOKAS_LIBRARY_DIRS
        "${EOKAS_PROJECT_DIR}/deps/lib/${EOKAS_OS_NAME}/${CMAKE_BUILD_TYPE}"
)

file(GLOB EOKAS_HEADER_FILES
        "${EOKAS_TARGET_DIR}/*.h"
)

file(GLOB EOKAS_SOURCE_FILES
        "${EOKAS_TARGET_DIR}/*.cpp"
        "${EOKAS_TARGET_DIR}/${EOKAS_OS_NAME}/*.cpp"
)

set(EOKAS_LIBRARY_FILES
        "base" "gpu"
)

message("EOKAS_HEADER_DIRS = ${EOKAS_HEADER_DIRS}")
message("EOKAS_LIBRARY_DIRS = ${EOKAS_LIBRARY_DIRS}")
message("EOKAS_HEADER_FILES = ${EOKAS_HEADER_FILES}")
message("EOKAS_SOURCE_FILES = ${EOKAS_SOURCE_FILES}")
message("EOKAS_LIBRARY_FILES = ${EOKAS_LIBRARY_FILES}")


add_executable(${EOKAS_TARGET_NAME} ${EOKAS_HEADER_FILES} ${EOKAS_SOURCE_FILES})
target_include_directories(${EOKAS_TARGET_NAME} PRIVATE ${EOKAS_HEADER_DIRS})
target_link_directories(${EOKAS_TARGET_NAME} PRIVATE ${EOKAS_LIBRARY_DIRS})
target_link_libraries(${EOKAS_TARGET_NAME} ${EOKAS_LIBRARY_FILES})
if(WIN32)
    set_target_properties(${EOKAS_TARGET_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif()


install(TARGETS ${EOKAS_TARGET_NAME} DESTINATION bin/${EOKAS_OS_NAME}/${CMAKE_BUILD_TYPE})