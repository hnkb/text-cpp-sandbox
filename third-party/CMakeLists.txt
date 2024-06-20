include (FetchContent)

FetchContent_Declare (
	brotli
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/brotli"
	URL "https://github.com/google/brotli/archive/refs/tags/v1.1.0.tar.gz"
	EXCLUDE_FROM_ALL)

FetchContent_Declare (
	woff2
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/woff2"
	URL "https://github.com/google/woff2/archive/refs/tags/v1.0.2.tar.gz"
	EXCLUDE_FROM_ALL)


### configure and include brotli

# no need to build brotli tests, let's suppress warnings about test data
set(BROTLI_DISABLE_TESTS ON CACHE BOOL "No need to configure brotli tests" FORCE)

FetchContent_MakeAvailable (brotli)


### configure and include woff2

# Pass these variables to WOFF2
if (TARGET brotlidec)
	get_target_property(BROTLI_INCLUDE_DIRS brotlidec INTERFACE_INCLUDE_DIRECTORIES)
	set(BROTLIDEC_INCLUDE_DIRS ${BROTLI_INCLUDE_DIRS} CACHE STRING "Brotli include directories")
	set(BROTLIDEC_LIBRARIES brotlidec CACHE STRING "Brotli libraries")
endif()
if (TARGET brotlienc)
	get_target_property(BROTLI_INCLUDE_DIRS brotlienc INTERFACE_INCLUDE_DIRECTORIES)
	set(BROTLIENC_INCLUDE_DIRS ${BROTLI_INCLUDE_DIRS} CACHE STRING "Brotli include directories")
	set(BROTLIENC_LIBRARIES brotlienc CACHE STRING "Brotli libraries")
endif()

# Force woff2 to build static libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

FetchContent_MakeAvailable (woff2)

# add the missing include directory to woff2 target
target_include_directories(woff2common INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/woff2/include")


### configure and include ttf2mesh

add_library (ttf2mesh STATIC "ttf2mesh/ttf2mesh.c")
target_include_directories(ttf2mesh PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ttf2mesh")
