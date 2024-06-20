cmake_minimum_required(VERSION 3.24)

project(NativeSandbox)

set(TARGET_NAME ${PROJECT_NAME})

add_executable(${TARGET_NAME}
	"src/text/Glyph_ttf2mesh.cpp"
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

add_subdirectory("third-party")
target_link_libraries(${TARGET_NAME} PRIVATE
	ttf2mesh
	brotlienc brotlicommon
	woff2dec woff2common
)
