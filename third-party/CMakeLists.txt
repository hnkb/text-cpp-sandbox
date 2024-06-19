include (FetchContent)

FetchContent_Declare (
	brotli
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/brotli"
	URL "https://github.com/google/brotli/archive/refs/tags/v1.1.0.tar.gz"
	EXCLUDE_FROM_ALL)


### configure and include brotli

# no need to build brotli tests, let's suppress warnings about test data
set(BROTLI_DISABLE_TESTS ON CACHE BOOL "No need to configure brotli tests" FORCE)

FetchContent_MakeAvailable (brotli)


### configure and include ttf2mesh

add_library (ttf2mesh STATIC "ttf2mesh/ttf2mesh.c")
target_include_directories(ttf2mesh PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ttf2mesh")
