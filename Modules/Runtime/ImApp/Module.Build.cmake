
set(EOKAS_TARGET_NAME "ImApp")
set(EOKAS_TARGET_DIR "${EOKAS_RUNTIME_DIR}/ImApp")

message("EOKAS_TARGET_NAME = ${EOKAS_TARGET_NAME}")
message("EOKAS_TARGET_DIR = ${EOKAS_TARGET_DIR}")

set(EOKAS_HEADER_DIRS
        "${EOKAS_DEPS_DIR}/vulkan/include"
        "${EOKAS_DEPS_DIR}/glfw/include"
        "${EOKAS_DEPS_DIR}/FreeType2/include"

        "${EOKAS_RUNTIME_DIR}"

        "${EOKAS_TARGET_DIR}"
        "${EOKAS_TARGET_DIR}/imgui"
        "${EOKAS_TARGET_DIR}/imgui/${EOKAS_OS_NAME}"
        "${EOKAS_TARGET_DIR}/implot"
)

set(EOKAS_LIBRARY_DIRS
        "${EOKAS_DEPS_DIR}/vulkan/lib/${EOKAS_OS_NAME}"
        "${EOKAS_DEPS_DIR}/glfw/lib/${EOKAS_OS_NAME}"
        "${EOKAS_DEPS_DIR}/FreeType2/lib/${EOKAS_OS_NAME}"
)


file(GLOB EOKAS_HEADER_FILES
        "${EOKAS_TARGET_DIR}/*.h"
        "${EOKAS_TARGET_DIR}/imgui/*.h"
        "${EOKAS_TARGET_DIR}/implot/*.h"
        "${EOKAS_TARGET_DIR}/imwidgets/*.h"
        "${EOKAS_TARGET_DIR}/app/*.h"
)

file(GLOB EOKAS_SOURCE_FILES
        "${EOKAS_TARGET_DIR}/imgui/*.cpp"
        "${EOKAS_TARGET_DIR}/imgui/${EOKAS_OS_NAME}/*.cpp"
        "${EOKAS_TARGET_DIR}/imgui/${EOKAS_OS_NAME}/*.mm"
        "${EOKAS_TARGET_DIR}/implot/*.cpp"
        "${EOKAS_TARGET_DIR}/imwidgets/*.cpp"
        "${EOKAS_TARGET_DIR}/utils/*.cpp"
        "${EOKAS_TARGET_DIR}/app/*.cpp"
)

set(EOKAS_LIBRARY_FILES
    "Base" "glfw3" "freetype"
)

if(APPLE)
    list(APPEND EOKAS_SOURCE_FILES
            "${EOKAS_TARGET_DIR}/Main.mm"
    )
    list(APPEND EOKAS_LIBRARY_FILES
            "-framework Metal"
            "-framework MetalKit"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
            "-framework QuartzCore"
    )
else()
    list(APPEND EOKAS_SOURCE_FILES
            "${EOKAS_TARGET_DIR}/Main.cpp"
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

add_executable(${EOKAS_TARGET_NAME} ${EOKAS_HEADER_FILES} ${EOKAS_SOURCE_FILES})
target_include_directories(${EOKAS_TARGET_NAME} PRIVATE ${EOKAS_HEADER_DIRS})
target_link_directories(${EOKAS_TARGET_NAME} PRIVATE ${EOKAS_LIBRARY_DIRS})
target_link_libraries(${EOKAS_TARGET_NAME} ${EOKAS_LIBRARY_FILES})


install(TARGETS ${EOKAS_TARGET_NAME} DESTINATION bin/${EOKAS_OS_NAME}/${CMAKE_BUILD_TYPE})

