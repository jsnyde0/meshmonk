cmake_minimum_required(VERSION 3.10)
project(MeshMonkCLI)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# MeshMonk library
set(MESHMONK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../) # /app/
find_library(MESHMONK_LIBRARY NAMES meshmonk libmeshmonk
    HINTS 
        ${CMAKE_CURRENT_SOURCE_DIR}/../builds/Linux/
        ${CMAKE_CURRENT_SOURCE_DIR}/../builds/Ubuntu_16_04/  # Specific path for the provided library
        ${CMAKE_CURRENT_SOURCE_DIR}/../builds/Windows/ 
        ${CMAKE_CURRENT_SOURCE_DIR}/../builds/MacOS/)
if(NOT MESHMONK_LIBRARY)
    message(FATAL_ERROR "MeshMonk library (libmeshmonk.so) not found in specified HINTS paths.")
endif()

# Eigen3 library
find_package(Eigen3 REQUIRED NO_MODULE)

# OpenMesh library
find_package(OpenMesh REQUIRED)

# cxxopts library
# Assuming cxxopts was installed via CMake (sudo cmake --build . --target install from its build dir)
# which provides cxxopts-config.cmake
find_package(cxxopts CONFIG REQUIRED)

add_executable(meshmonk_cli cli.cpp)

# Include Directories
target_include_directories(meshmonk_cli PUBLIC
    ${MESHMONK_INCLUDE_DIR}             # For meshmonk.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../vendor # For nanoflann.hpp (used by meshmonk internals)
    ${OpenMesh_INCLUDE_DIRS}            # For OpenMesh headers
    ${EIGEN3_INCLUDE_DIR}               # For Eigen headers (though often header-only and found by compiler)
                                        # cxxopts include dir should be handled by its CMake config
)

# Link Libraries
# Order can be important: typically link specific dependencies first, then more general ones.
# meshmonk_cli depends on MESHMONK_LIBRARY.
# MESHMONK_LIBRARY itself depends on OpenMesh (implicitly).
# cli.cpp also directly uses OpenMesh, Eigen, cxxopts.
target_link_libraries(meshmonk_cli PUBLIC
    ${MESHMONK_LIBRARY}       # Link against the pre-built MeshMonk library
    ${OpenMesh_LIBRARIES}     # Link against OpenMesh libraries found (libOpenMeshCore.so, libOpenMeshTools.so)
    Eigen3::Eigen             # Link against Eigen
    cxxopts::cxxopts          # Link against cxxopts
)

# Set RPATH to find libmeshmonk.so at runtime if it's not in standard paths
# This helps the executable find libmeshmonk.so if it's in a custom location relative to the build/install.
# The origin is the location of the executable. $ORIGIN/../lib or $ORIGIN/../builds/Ubuntu_16_04 etc.
# For development, directly using the path from MESHMONK_LIBRARY might be okay.
# However, find_library returns the full path, so it should be linked with that full path.
# For runtime, if libmeshmonk.so is not in a standard system path, this might be needed.
# This is more for deployment/installation convenience.
# For now, direct linking via full path from MESHMONK_LIBRARY should be sufficient for build.
# If runtime errors about not finding libmeshmonk.so occur, RPATH settings might be needed.
# Example: SET_TARGET_PROPERTIES(meshmonk_cli PROPERTIES
# INSTALL_RPATH "$ORIGIN/../lib:${OpenMesh_LIBRARY_DIRS}") # Adjust path as needed
# BUILD_WITH_INSTALL_RPATH TRUE
# Or:
# file(RELATIVE_PATH MESHMONK_LIB_REL_PATH "${CMAKE_BINARY_DIR}" "${MESHMONK_LIBRARY}")
# SET_TARGET_PROPERTIES(meshmonk_cli PROPERTIES
#   INSTALL_RPATH "$ORIGIN;$ORIGIN/${MESHMONK_LIB_REL_PATH}/../../builds/Ubuntu_16_04") # Example
