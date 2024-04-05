cmake_minimum_required(VERSION 3.21)

project(TextTest)

add_executable(TextTest
	"src/TextLayout.cpp"
	"src/main.cpp"
)


target_compile_features(TextTest PRIVATE cxx_std_17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
target_include_directories(TextTest PRIVATE ${HARFBUZZ_INCLUDE_DIRS})
target_link_directories(TextTest PRIVATE ${HARFBUZZ_LIBRARY_DIRS})
target_link_libraries(TextTest PRIVATE ${HARFBUZZ_LIBRARIES})

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})
target_link_libraries(TextTest PRIVATE ${FREETYPE_LIBRARIES})

add_subdirectory("third-party")

target_link_libraries(TextTest PRIVATE ttf2mesh libtess2)
