cmake_minimum_required(VERSION 3.13)

project(mflang VERSION 1.0.0)

set(CMAKE_BUILD_TYPE Debug)

# Set the C++ standard (e.g., 11, 14, 17, 20, 23, 26)
set(CMAKE_CXX_STANDARD 17)
# Ensure the standard is a requirement; if not supported, CMake will error
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Optional: Disable compiler-specific extensions (like GNU extensions)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} main.cc)

add_subdirectory(klds)

target_link_libraries(${PROJECT_NAME} "klds")