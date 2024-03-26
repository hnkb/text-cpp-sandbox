cmake_minimum_required(VERSION 3.21)

project(NativeSandbox)

set(TARGET_NAME ${PROJECT_NAME})

add_executable(${TARGET_NAME}
	"src/main.cpp"
)


target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
