cmake_minimum_required(VERSION 3.10)
project(meshmonk_python LANGUAGES CXX)

# Add pybind11
# pybind11_add_module will take care of most pybind11 configurations
# pybind11 should be available from FetchContent in the parent scope.

# Explicitly set Python version for pybind11
set(PYBIND11_PYTHON_VERSION 3.12)
set(PYTHON_EXECUTABLE /usr/bin/python3.12)

# Define the Python extension module
# The first argument is the name of the module
# The second argument is the source file for the bindings
pybind11_add_module(meshmonk_python meshmonk_bindings.cpp)

# Link against meshmonk_shared
# This assumes meshmonk_shared is a target defined elsewhere (e.g., in the parent scope's library build)
target_link_libraries(meshmonk_python PRIVATE meshmonk_shared)

# Include directories
# Add include directories needed for meshmonk_bindings.cpp
# This typically includes the include directory of meshmonk and Eigen (if used in bindings)
target_include_directories(meshmonk_python PRIVATE
    ${CMAKE_SOURCE_DIR}/library/include # For meshmonk headers
    ${Eigen3_INCLUDE_DIRS}             # For Eigen headers
    ${CMAKE_SOURCE_DIR}/vendor/OpenMesh-11.0.0/src # For OpenMesh headers
    # pybind11_INCLUDE_DIRS should be handled by pybind11_add_module
)

# Optional: Set C++ standard if not inherited or needs to be specific
# set_target_properties(meshmonk_python PROPERTIES CXX_STANDARD 14)

# Installation (optional, but good practice)
# This tells CMake where to install the Python module
# Adjust the destination path as needed (e.g., site-packages or a custom location)
# You might need to set PYTHON_EXTENSION_MODULE_INSTALL_DIR
# Example:
# install(TARGETS meshmonk_python DESTINATION your_python_packages_path)
