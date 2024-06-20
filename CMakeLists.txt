cmake_minimum_required(VERSION 3.24)

project(NativeSandbox)

set(TARGET_NAME ${PROJECT_NAME})

add_executable(${TARGET_NAME}
	"src/graphics/Collection.cpp"
	"src/text/Glyph.cpp"
	"src/text/TextLayout.cpp"
	"src/text/WOFF2.cpp"
	"src/utils/Compression.cpp"
	"src/utils/FilePack.cpp"
	"src/main.cpp"
)


target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
target_include_directories(${TARGET_NAME} PRIVATE ${HARFBUZZ_INCLUDE_DIRS})
target_link_directories(${TARGET_NAME} PRIVATE ${HARFBUZZ_LIBRARY_DIRS})
target_link_libraries(${TARGET_NAME} PRIVATE ${HARFBUZZ_LIBRARIES})

find_package(Freetype REQUIRED)
target_include_directories(${TARGET_NAME} PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} PRIVATE ${FREETYPE_LIBRARIES})

add_subdirectory("third-party")
target_link_libraries(${TARGET_NAME} PRIVATE
	ttf2mesh
	libtess2 clipper
	brotlienc brotlicommon
	woff2dec woff2common
)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Available build types" FORCE)

# Release build settings
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

