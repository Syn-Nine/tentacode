cmake_minimum_required(VERSION 3.16.0)

option(USE_RAYLIB "Enable Raylib support" ON)

# Project declaration
project(Tentacode VERSION 2024.9.0 DESCRIPTION "Tentacode Game Dev Library" LANGUAGES CXX)

# Enable export of compile commands (for tools like clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Tentacode version info
set(TT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(TT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(TT_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(TT_VERSION_STRING "${TT_VERSION_MAJOR}.${TT_VERSION_MINOR}.${TT_VERSION_PATCH}")

message(STATUS ${TT_VERSION_STRING})

# Configure version file
configure_file(${PROJECT_SOURCE_DIR}/include/tentacode/TentacodeVersion.hh.in
               ${PROJECT_BINARY_DIR}/tentacode/TentacodeVersion.hh @ONLY)

# Add the library target
# If Raylib is disabled, define NO_RAYLIB
if(NOT USE_RAYLIB)
    add_compile_definitions(NO_RAYLIB)
endif()
add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} 
    PRIVATE
    src/Literal.cpp
    src/Parser.cpp
    src/Scanner.cpp
)

##########################################################
# Link and build  
##########################################################

add_executable(tentacode_exe src/interp.cpp)

# add dependencies
include(cmake/CPM.cmake)

CPMAddPackage("gh:raysan5/raylib#5.0")

# Link the library to the executable
target_link_libraries(tentacode_exe PRIVATE ${PROJECT_NAME} raylib)

# Include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE ${raylib_SOURCE_DIR}/src
)

# Compiler options
target_compile_options(${PROJECT_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wshadow -Wno-unused>
        # $<$<CXX_COMPILER_ID:GNU>:-Werror >
)


# Set executable output directories 
set_target_properties(tentacode_exe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)