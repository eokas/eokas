
set(EOKAS_TARGET_NAME "gpu")
set(EOKAS_TARGET_DIR "${EOKAS_MODULES_DIR}/${EOKAS_TARGET_NAME}")

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
        "base"
)

if(APPLE)
    list(APPEND EOKAS_SOURCE_FILES
            "${EOKAS_TARGET_DIR}/Metal/metal.cpp"
    )
    list(APPEND EOKAS_LIBRARY_FILES
            "-framework Metal"
            "-framework MetalKit"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
            "-framework QuartzCore"
    )
elseif (WIN32)
    list(APPEND EOKAS_SOURCE_FILES
            "${EOKAS_TARGET_DIR}/DX12/dx12.cpp"
    )
    list(APPEND EOKAS_LIBRARY_FILES
            "dxguid.lib" "dxgi.lib" "d3d12.lib" "d3dcompiler.lib"
    )
    add_definitions(-D_MBCS)
else()
    list(APPEND EOKAS_SOURCE_FILES
            "${EOKAS_TARGET_DIR}/Vulkan/vulka.cpp"
    )
    list(APPEND EOKAS_LIBRARY_FILES
            "vulkan-1"
    )
endif()

message("EOKAS_HEADER_DIRS = ${EOKAS_HEADER_DIRS}")
message("EOKAS_LIBRARY_DIRS = ${EOKAS_LIBRARY_DIRS}")
message("EOKAS_HEADER_FILES = ${EOKAS_HEADER_FILES}")
message("EOKAS_SOURCE_FILES = ${EOKAS_SOURCE_FILES}")
message("EOKAS_LIBRARY_FILES = ${EOKAS_LIBRARY_FILES}")


add_library(${EOKAS_TARGET_NAME} STATIC ${EOKAS_HEADER_FILES} ${EOKAS_SOURCE_FILES})
target_include_directories(${EOKAS_TARGET_NAME} PRIVATE ${EOKAS_HEADER_DIRS})
target_link_directories(${EOKAS_TARGET_NAME} PRIVATE ${EOKAS_LIBRARY_DIRS})
target_link_libraries(${EOKAS_TARGET_NAME} ${EOKAS_LIBRARY_FILES})


install(FILES ${EOKAS_HEADER_FILES} DESTINATION include/${EOKAS_TARGET_NAME})
install(TARGETS ${EOKAS_TARGET_NAME} DESTINATION lib/${EOKAS_OS_NAME}/${CMAKE_BUILD_TYPE})
