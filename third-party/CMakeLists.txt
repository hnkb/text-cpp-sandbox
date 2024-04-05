add_library (ttf2mesh STATIC "ttf2mesh/ttf2mesh.c")
target_include_directories(ttf2mesh PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ttf2mesh")

file(GLOB LIBTESS2_SOURCES "libtess2-1.0.2/Source/*.c")
add_library (libtess2 STATIC ${LIBTESS2_SOURCES})
target_include_directories(libtess2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libtess2-1.0.2/Include")
