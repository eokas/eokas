
set(EOKAS_TARGET_NAME "MainEditor")
set(EOKAS_TARGET_DIR "${EOKAS_TOOLKIT_DIR}/${EOKAS_TARGET_NAME}")

message("EOKAS_TARGET_NAME = ${EOKAS_TARGET_NAME}")
message("EOKAS_TARGET_DIR = ${EOKAS_TARGET_DIR}")

set(EOKAS_HEADER_DIRS
        "${EOKAS_RUNTIME_DIR}"
        "${EOKAS_TOOLKIT_DIR}"
        "${EOKAS_TARGET_DIR}"
)

set(EOKAS_LIBRARY_DIRS
        "${EOKAS_DEPS_DIR}/lib/${EOKAS_OS_NAME}/${CMAKE_BUILD_TYPE}"
        "${EOKAS_DEPS_DIR}/FreeType2/lib/${EOKAS_OS_NAME}"
)


file(GLOB EOKAS_HEADER_FILES
        "${EOKAS_TARGET_DIR}/*.h"
)

file(GLOB EOKAS_SOURCE_FILES
        "${EOKAS_TARGET_DIR}/*.cpp"
        "${EOKAS_TARGET_DIR}/${EOKAS_OS_NAME}/*.cpp"
)

# freetype.lib references optional PNG/gzip/bzip2/brotli symbols that are not vendored.
if(WIN32)
    list(APPEND EOKAS_SOURCE_FILES "${EOKAS_PROJECT_DIR}/Test/Test-UI/FreetypeDepsStubs.cpp")
endif()

set(EOKAS_LIBRARY_FILES
        "ToolBase" "Base" "Native" "GPU" "Mesh" "Render" "UI" "freetype"
)

if(WIN32)
    list(APPEND EOKAS_LIBRARY_FILES "user32.lib" "dwmapi.lib" "dxguid.lib" "dxgi.lib" "d3d12.lib" "d3dcompiler.lib")
endif()

message("EOKAS_HEADER_DIRS = ${EOKAS_HEADER_DIRS}")
message("EOKAS_LIBRARY_DIRS = ${EOKAS_LIBRARY_DIRS}")
message("EOKAS_HEADER_FILES = ${EOKAS_HEADER_FILES}")
message("EOKAS_SOURCE_FILES = ${EOKAS_SOURCE_FILES}")
message("EOKAS_LIBRARY_FILES = ${EOKAS_LIBRARY_FILES}")

add_executable(${EOKAS_TARGET_NAME} ${EOKAS_HEADER_FILES} ${EOKAS_SOURCE_FILES})
target_include_directories(${EOKAS_TARGET_NAME} PRIVATE ${EOKAS_HEADER_DIRS})
target_link_directories(${EOKAS_TARGET_NAME} PRIVATE ${EOKAS_LIBRARY_DIRS})
target_link_libraries(${EOKAS_TARGET_NAME} ${EOKAS_LIBRARY_FILES})


install(TARGETS ${EOKAS_TARGET_NAME} DESTINATION bin/${EOKAS_OS_NAME}/${CMAKE_BUILD_TYPE})
