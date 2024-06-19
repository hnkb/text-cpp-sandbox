cmake_minimum_required(VERSION 3.21)

project(NativeSandbox)

set(TARGET_NAME ${PROJECT_NAME})

add_executable(${TARGET_NAME}
	"src/text/TextLayout.cpp"
	"src/main.cpp"
)


target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)


find_package(PkgConfig REQUIRED)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
target_include_directories(${TARGET_NAME} PRIVATE ${HARFBUZZ_INCLUDE_DIRS})
target_link_directories(${TARGET_NAME} PRIVATE ${HARFBUZZ_LIBRARY_DIRS})
target_link_libraries(${TARGET_NAME} PRIVATE ${HARFBUZZ_LIBRARIES})
