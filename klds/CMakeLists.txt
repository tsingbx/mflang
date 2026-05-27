cmake_minimum_required(VERSION 3.13)
project(klds VERSION 1.0.0)
# Set the C++ standard (e.g., 11, 14, 17, 20, 23, 26)
set(CMAKE_CXX_STANDARD 17)
# Ensure the standard is a requirement; if not supported, CMake will error
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Optional: Disable compiler-specific extensions (like GNU extensions)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${PROJECT_NAME} ast.cc lexer.cc parser.cc repl.cc)

find_package(LLVM REQUIRED CONFIG)

target_include_directories(${PROJECT_NAME} INTERFACE ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs core support native)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${llvm_libs})

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "ast.h;lexer.h;parser.h;repl.h")
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        )

