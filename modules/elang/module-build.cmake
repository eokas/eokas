
set(EOKAS_TARGET_NAME "elang")
set(EOKAS_TARGET_DIR "${EOKAS_MODULES_DIR}/${EOKAS_TARGET_NAME}")

message("EOKAS_TARGET_NAME = ${EOKAS_TARGET_NAME}")
message("EOKAS_TARGET_DIR = ${EOKAS_TARGET_DIR}")


set(EOKAS_HEADER_DIRS
        "${EOKAS_MODULES_DIR}"
)

set(EOKAS_LIBRARY_DIRS
)

set(EOKAS_LIBRARY_FILES
        base
)

file(GLOB EOKAS_SOURCE_FILES
        "${EOKAS_TARGET_DIR}/src/app/app.cpp"
        "${EOKAS_TARGET_DIR}/src/app/parser.cpp"
        "${EOKAS_TARGET_DIR}/src/app/scanner.cpp"
        "${EOKAS_TARGET_DIR}/src/sema/*.cpp"
        "${EOKAS_TARGET_DIR}/src/cpp/cpp-backend.cpp"
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
