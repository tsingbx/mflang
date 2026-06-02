cmake_minimum_required(VERSION 3.13)

project(mflang VERSION 1.0.0)
# Set the C++ standard (e.g., 11, 14, 17, 20, 23, 26)
set(CMAKE_CXX_STANDARD 17)
# Ensure the standard is a requirement; if not supported, CMake will error
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Optional: Disable compiler-specific extensions (like GNU extensions)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(${PROJECT_NAME} codegen.cc)

target_link_libraries(${PROJECT_NAME} PRIVATE llvm)