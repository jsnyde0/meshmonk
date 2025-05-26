cmake_minimum_required(VERSION 3.10)
project(MeshMonkProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Set PIC for all targets

# --- ExternalProject for OpenMesh ---
include(ExternalProject)

ExternalProject_Add(openmesh_project
    SOURCE_DIR /tmp/OpenMesh-8.1-verified # Use the verified OpenMesh 8.1 source
    #--Configure step-------------
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/openmesh_install
               -DCMAKE_BUILD_TYPE=Release
               -DBUILD_SHARED_LIBS=OFF # Build OpenMesh as a static library
               -DOPENMESH_BUILD_APPS=OFF
               -DOPENMESH_BUILD_EXAMPLES=OFF
               -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
    #--Build step-----------------
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    #--Install step---------------
    INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install --config Release
    #--Output logging-------------
    LOG_DOWNLOAD OFF # Source is local
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_INSTALL ON
)

# --- Make OpenMesh Headers and Libraries Available (Static Build) ---
# Create an imported target for static OpenMesh
add_library(OpenMesh::Core STATIC IMPORTED GLOBAL) 
set_property(TARGET OpenMesh::Core PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/openmesh_install/lib/libOpenMeshCore.a)
# For OpenMesh 8.1, static libraries might have an 'S' suffix, like libOpenMeshCoreS.a
# The worker might need to check the actual generated library name in openmesh_install/lib/
# and adjust IMPORTED_LOCATION if this fails.

add_library(OpenMesh::Tools STATIC IMPORTED GLOBAL)
set_property(TARGET OpenMesh::Tools PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/openmesh_install/lib/libOpenMeshTools.a)

set(OPENMESH_INCLUDE_DIR ${CMAKE_BINARY_DIR}/openmesh_install/include CACHE INTERNAL "OpenMesh include directory")

# --- Eigen3 ---
find_package(Eigen3 REQUIRED NO_MODULE)

# --- meshmonk_shared Library Definition ---
# Collect source files for libmeshmonk
file(GLOB MESHMONK_SRC_FILES 
    "src/*.cpp"
)
set(MESHMONK_ROOT_SRC_FILES
    meshmonk.cpp
)

add_library(meshmonk_shared SHARED ${MESHMONK_SRC_FILES} ${MESHMONK_ROOT_SRC_FILES})

# Include Directories for meshmonk_shared
target_include_directories(meshmonk_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}          # For meshmonk.hpp, global.hpp in root
    ${CMAKE_CURRENT_SOURCE_DIR}/src      # For headers in src/
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor   # For nanoflann.hpp
    ${EIGEN3_INCLUDE_DIR}
    ${OPENMESH_INCLUDE_DIR}              # Use the include dir from the installed OpenMesh
)

# Link meshmonk_shared against OpenMesh and Eigen3
target_link_libraries(meshmonk_shared PUBLIC
    OpenMesh::Core
    OpenMesh::Tools
    Eigen3::Eigen
)

# Set properties for output name
set_target_properties(meshmonk_shared PROPERTIES
    OUTPUT_NAME "meshmonk" # To get libmeshmonk.so
)

# Add dependency for meshmonk_shared on openmesh_project
# This ensures OpenMesh is built and installed before meshmonk_shared tries to link against it.
add_dependencies(meshmonk_shared openmesh_project)

# --- CLI Placeholder ---
# add_subdirectory(cli) # This will be done in a subsequent step
message(STATUS "Root CMakeLists.txt configured using ExternalProject for OpenMesh 8.1.")
message(STATUS "meshmonk_shared library definition is in place.")

# --- Installation (Optional, but good practice) ---
install(TARGETS meshmonk_shared
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES meshmonk.hpp global.hpp
    DESTINATION include/meshmonk
)
install(DIRECTORY src/
    DESTINATION include/meshmonk/src
    FILES_MATCHING PATTERN "*.hpp"
)

message(STATUS "MeshMonkProject (libmeshmonk_shared) configured.")
