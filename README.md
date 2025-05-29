# MeshMonk: High-Performance 3D Mesh Registration

MeshMonk is a C++ library designed for efficient 3D mesh registration. It provides functionalities for rigid, non-rigid, and pyramid-based registration approaches. This repository is currently undergoing modernization. Phase 1, focusing on a CMake build system for the core library, C++ examples, command-line interface, and Python bindings, is complete.

## Overview

The core of MeshMonk is a C++ library that can be integrated into your own projects.
Currently, it can be used:

*   **As a C++ Library:** Link against `libmeshmonk_shared` in your C++ applications. A C++ example (`MeshMonkExample`) demonstrating library usage is buildable via CMake.
*   **As a Command-Line Tool:** Use `meshmonk_cli` to perform registrations directly.
*   **From Python:** Import the `meshmonk_python` module to access registration functionalities.

Future phases will focus on:
*   Re-integrating MATLAB MEX bindings.

This document primarily focuses on building the core MeshMonk C++ library, its C++ example, the command-line interface, and the Python bindings using CMake.

## Repository Structure

The repository has been reorganized for clarity and modularity:

*   `CMakeLists.txt`: Root CMake file for the project.
*   `library/`: Contains the core MeshMonk library.
    *   `CMakeLists.txt`: CMake file for building `libmeshmonk_shared`.
    *   `include/meshmonk/`: Public API headers for the library (e.g., `meshmonk.hpp`).
    *   `src/`: Internal source code and private headers for the library.
    *   `examples/`: C++ example code demonstrating library usage.
        *   `CMakeLists.txt`: CMake file for building the C++ example(s).
*   `cli/`: Contains the source code for the command-line interface.
    *   `CMakeLists.txt`: CMake file for building `meshmonk_cli`.
    *   `cli.cpp`: Main source file for the CLI.
*   `python/`: Contains the C++ source code for Python bindings and a test script.
    *   `CMakeLists.txt`: CMake file for building the `meshmonk_python` module.
    *   `meshmonk_bindings.cpp`: Pybind11 C++ binding code.
    *   `test_bindings.py`: Python script to test the bindings.
*   `vendor/`: Contains third-party dependencies.
    *   `OpenMesh-11.0.0/`: Source for the OpenMesh library, built statically.
    *   `nanoflann.hpp`: Header-only library for KD-tree search.
*   `demo/`: Contains example mesh data for use with the CLI.
*   `docs/`: Contains legacy documentation.

## Building MeshMonk (Core Library, C++ Example, CLI & Python Bindings)

This method compiles the core shared library (`libmeshmonk_shared`), a C++ example (`MeshMonkExample`), the command-line tool (`meshmonk_cli`), and the Python module (`meshmonk_python`).

### Prerequisites

Ensure you have the following dependencies installed on your system:

1.  **C++14 Compiler:** A C++ compiler supporting C++14 (e.g., GCC >= 5 or Clang >= 3.4).
    *   Ubuntu: `sudo apt update && sudo apt install build-essential g++`
2.  **CMake:** Version 3.10 or higher.
    *   Ubuntu: `sudo apt install cmake`
3.  **Python (for Python bindings):** Python 3.6+ development libraries.
    *   Ubuntu: `sudo apt install python3-dev` (e.g., `python3.12-dev` for Python 3.12)
4.  **NumPy (for Python bindings runtime):**
    *   Ubuntu: `sudo apt install python3-numpy`
5.  **Eigen3, cxxopts, pybind11:** These are handled automatically by CMake using `FetchContent` and do not require separate system installations for the build process.

### Compilation Steps

1.  **Clone the Repository (if you haven't already):**
    ```bash
    # Example using HTTPS:
    git clone https://github.com/TheWebMonks/meshmonk.git # Adjust if using SSH or a fork
    cd meshmonk
    ```

2.  **Create a Build Directory:**
    It's standard practice to build outside the source directory.
    ```bash
    rm -rf build # Clean previous build if it exists
    mkdir build
    cd build
    ```

3.  **Run CMake:**
    This command configures the project and generates the necessary build files. It will download dependencies (Eigen3, cxxopts, pybind11 via FetchContent) and prepare to compile the vendored OpenMesh library, the `meshmonk_shared` library, the C++ example, the `meshmonk_cli` executable, and the `meshmonk_python` module.
    ```bash
    # Basic command:
    cmake ..
    # If building Python bindings and CMake doesn't find your desired Python version, specify it:
    # cmake .. -DPYTHON_EXECUTABLE=/usr/bin/python3.12 
    ```
    If you encounter errors, please ensure your CMake, C++ compiler, and Python development libraries are correctly installed and meet the version requirements.

4.  **Compile the Project:**
    This will build all targets. Using `-j$(nproc)` utilizes all available CPU cores for a faster build on Linux.
    ```bash
    make -j$(nproc)
    # Alternatively, you can use:
    # cmake --build . -- -j$(nproc)
    ```

### Build Outputs

Upon successful compilation, you will find the key outputs in your `build/` directory (or its subdirectories):

*   **Core Shared Library:** `library/libmeshmonk_shared.so` (on Linux), `library/libmeshmonk_shared.dylib` (on macOS), or `library/meshmonk_shared.dll` (on Windows). (Path relative to `build/` directory)
*   **C++ Example Executable:** `library/examples/MeshMonkExample`. (Path relative to `build/` directory)
*   **Command-Line Executable:** `cli/meshmonk_cli`. (Path relative to `build/` directory)
*   **Python Module:** `python/meshmonk_python.cpython-<python_version>-<arch>.so` (e.g., `meshmonk_python.cpython-312-x86_64-linux-gnu.so` on Linux with Python 3.12). (Path relative to `build/` directory)

## Using the `meshmonk_cli` Command-Line Tool
(Content from existing README - unchanged)
...

## Using MeshMonk as a C++ Library
(Content from existing README - unchanged)
...

## Using MeshMonk from Python

Python bindings are available for MeshMonk, allowing you to use its registration functionalities directly within Python scripts.

### Building the Python Module

The Python module (`meshmonk_python`) is built as part of the standard CMake build process described in the "Building MeshMonk" section.
- `pybind11` is fetched automatically by CMake using `FetchContent`.
- Ensure you have Python development libraries installed (e.g., `python3-dev`).
- Basic build steps (from the project root, assuming CMake and a C++ compiler are set up):
  ```bash
  mkdir build
  cd build
  cmake .. 
  # Optionally specify your Python executable if needed:
  # cmake .. -DPYTHON_EXECUTABLE=/path/to/your/python
  make -j$(nproc)
  ```
- The compiled Python module (e.g., `meshmonk_python.cpython-312-x86_64-linux-gnu.so`) will be located in the `build/python/` directory.

### Using the Module

To use the module in Python:
1.  Add the directory containing the compiled module to your Python path.
2.  Import the module.

```python
import sys
import numpy as np # NumPy is a dependency for data exchange

# Add the build directory (or the specific python subdir) to sys.path
# Adjust this path based on where your 'build' directory is relative to your script
sys.path.append('./build/python') 
# If your script is in 'meshmonk/python' and build is '../build':
# sys.path.append('../build/python') 

try:
    import meshmonk_python
except ModuleNotFoundError:
    print("Error: Could not import meshmonk_python.")
    print("Ensure the path to the compiled module (e.g., build/python/) is in sys.path.")
    print(f"Current sys.path: {sys.path}")
    exit(1)

# Accessing NUM_FEATURES
# This constant indicates the number of features per vertex the MeshMonk library was compiled with.
# Your NumPy arrays for feature data should match this dimension.
print(f"MeshMonk NUM_FEATURES: {meshmonk_python.NUM_FEATURES}")
```

### Basic Usage Example: `rigid_registration`

This example demonstrates how to call the `rigid_registration` function.
**Important:** The MeshMonk C++ library functions, even when operating on point clouds, internally expect mesh structures (faces) and per-point flags. For stability, it's recommended to provide dummy data for these if you are working primarily with point cloud features.

```python
import numpy as np
import meshmonk_python # Assuming it's importable as per above

# 1. Setup: Prepare data
num_floating_pts = 50
num_target_pts = 60
num_feat = meshmonk_python.NUM_FEATURES # Should be 6 for this build

# Features (random data for example)
# Use np.float32 as this is what the wrappers expect
floating_features = np.random.rand(num_floating_pts, num_feat).astype(np.float32)
target_features = np.random.rand(num_target_pts, num_feat).astype(np.float32)

# Faces (dummy topology - required by the underlying C++ library)
# Create simple, non-overlapping triangles if enough points exist.
def create_dummy_faces(num_pts):
    if num_pts < 3:
        return np.empty((0, 3), dtype=np.int32)
    num_faces = num_pts // 3
    faces = np.array([[i * 3, i * 3 + 1, i * 3 + 2] for i in range(num_faces)], dtype=np.int32)
    return faces

floating_faces = create_dummy_faces(num_floating_pts)
target_faces = create_dummy_faces(num_target_pts)

# Flags (dummy data - e.g., all ones)
# Use np.float32
floating_flags = np.ones(num_floating_pts, dtype=np.float32)
target_flags = np.ones(num_target_pts, dtype=np.float32)

# Transformation matrix (4x4 for rigid registration)
# Initialize to identity for this example
transformation_matrix = np.eye(4, dtype=np.float32)

# Store originals for comparison
original_floating_features_subset = np.copy(floating_features[:2, :3]) # First 2 points, first 3 features
original_transformation_matrix = np.copy(transformation_matrix)

print("Original floating_features (first 2 points, XYZ):\n", original_floating_features_subset)
print("Original transformation_matrix:\n", original_transformation_matrix)

# 2. Call rigid registration
try:
    meshmonk_python.rigid_registration(
        floating_features, 
        target_features,
        floating_faces=floating_faces, # Provide dummy faces
        target_faces=target_faces,     # Provide dummy faces
        floating_flags=floating_flags, # Provide dummy flags
        target_flags=target_flags,     # Provide dummy flags
        transformation_matrix=transformation_matrix,
        num_iterations=10 # Example of an optional argument
        # Other optional arguments like correspondences_symmetric, etc., can be set here
    )
    print("\nrigid_registration called successfully.")
except Exception as e:
    print(f"\nError during rigid_registration: {e}")

# 3. Check results (data is modified in-place)
print("Modified floating_features (first 2 points, XYZ):\n", floating_features[:2, :3])
print("Modified transformation_matrix:\n", transformation_matrix)

# Further examples for nonrigid_registration and pyramid_registration would follow a similar pattern.
# Remember to provide dummy faces and flags for them as well.
# For nonrigid_registration, the 'transformation_matrix' is not an output; 
# 'floating_features' are deformed in place.
# For pyramid_registration, faces are mandatory arguments in the binding.
```

### Available Functions

The following registration functions are available in the `meshmonk_python` module:

*   **`rigid_registration(...)`**:
    Performs rigid registration between a floating and a target feature set.
    Modifies the input `floating_features` (first 3 columns representing XYZ coordinates) and `transformation_matrix` in-place.
    Requires `floating_features`, `target_features`, and `transformation_matrix`.
    `floating_faces`, `target_faces`, `floating_flags`, `target_flags` are optional but recommended to be provided as dummy data for stability. Many other parameters are optional and mirror the C++ API defaults.

*   **`nonrigid_registration(...)`**:
    Performs non-rigid registration.
    Modifies the input `floating_features` in-place.
    Requires `floating_features` and `target_features`.
    `floating_faces`, `target_faces`, `floating_flags`, `target_flags` are optional but recommended. Other parameters are optional.

*   **`pyramid_registration(...)`**:
    Performs pyramid-based non-rigid registration.
    Modifies the input `floating_features` in-place.
    Requires `floating_features`, `target_features`, `floating_faces`, and `target_faces`.
    `floating_flags` and `target_flags` are optional but recommended. Other parameters are optional.

Please refer to the C++ function signatures in `meshmonk/meshmonk.hpp` or the binding code in `python/meshmonk_bindings.cpp` for detailed parameter lists and their default values.

## Using MeshMonk from MATLAB

(Content from existing README - mostly unchanged, except for removal of the "Python support is planned" line)
...
To use MeshMonk's functionalities within MATLAB, you first need to build the core shared library using CMake and then compile the MEX functions.

### 1. Build `libmeshmonk_shared` via CMake

Follow the main build instructions in this README to configure and build MeshMonk using CMake. This will produce the `libmeshmonk_shared` library (e.g., `libmeshmonk_shared.so` on Linux, `libmeshmonk_shared.dylib` on macOS, or `meshmonk_shared.dll` and `meshmonk_shared.lib` on Windows). This library is typically found in the `build/library/` directory (or `build/bin/` for the DLL on Windows, depending on CMake configuration).

### 2. Compile MEX Functions

Once `libmeshmonk_shared` is built, you can compile the MEX functions:

1.  Open MATLAB.
2.  Navigate MATLAB's current directory to the `matlab/` directory within the MeshMonk project (e.g., `cd path/to/meshmonk/matlab`).
3.  Run the appropriate MEX compilation script:
    *   On **Linux or macOS**: Execute `mex_all` in the MATLAB command window.
    *   On **Windows**: Execute `mex_windows_all` in the MATLAB command window.

    ```matlab
    % For Linux/macOS
    mex_all
    
    % For Windows
    % mex_windows_all
    ```

    These scripts will compile all necessary MEX functions and place them in the current directory (`matlab/`).

**Notes for MEX Compilation:**
*   The MEX scripts (`mex_all.m` and `mex_windows_all.m`) assume that the MeshMonk project root is one level above the `matlab/` directory (i.e., `meshmonkRoot = '..';`).
*   The scripts expect to find Eigen3 headers in a directory fetched by CMake (typically `build/_deps/eigen-src/`). If you have Eigen3 installed in a custom system location, you may need to modify the `eigenIncludeDir` variable at the beginning of the respective `mex_*.m` script.
*   Similarly, OpenMesh headers are expected to be found within the CMake build directory (`build/vendor/OpenMesh-11.0.0/_build/src/`).

### 3. MATLAB Runtime Environment Setup

For MATLAB to find `libmeshmonk_shared` and the compiled MEX functions at runtime:

*   **MEX Functions:** The demo scripts in the `demo/` directory are configured to find MEX files if they are located in the `matlab/` directory (due to `addpath(fullfile('..', 'matlab'))`). Ensure your compiled MEX files (`.mexa64`, `.mexw64`, etc.) are in `matlab/`.

*   **`libmeshmonk_shared` Library:**
    *   **Linux:** Add the directory containing `libmeshmonk_shared.so` (e.g., `path/to/meshmonk/build/library/`) to your `LD_LIBRARY_PATH` environment variable before launching MATLAB.
      ```bash
      export LD_LIBRARY_PATH=/path/to/meshmonk/build/library:$LD_LIBRARY_PATH
      matlab
      ```
      For a persistent setting, add this line to your `~/.bashrc` or `~/.zshrc`.

    *   **macOS:** Add the directory containing `libmeshmonk_shared.dylib` (e.g., `path/to/meshmonk/build/library/`) to your `DYLD_LIBRARY_PATH` environment variable.
      ```bash
      export DYLD_LIBRARY_PATH=/path/to/meshmonk/build/library:$DYLD_LIBRARY_PATH
      matlab
      ```
      Alternatively, you can create a `startup.m` file in your MATLAB user path (typically `~/Documents/MATLAB/`) and add:
      ```matlab
      addpath('path/to/meshmonk/build/library'); 
      % Or, more robustly for .dylib, consider a symbolic link or install_name_tool adjustments post-build.
      % However, DYLD_LIBRARY_PATH is the most common method for .dylib files not in standard locations.
      ```
      *(Note: `addpath` in `startup.m` is generally for M-files and MEX-files, not typically for `.dylib` dependencies directly, but MATLAB might pick them up if they are in its search path. `DYLD_LIBRARY_PATH` is more reliable for shared libraries.)*

    *   **Windows:** Ensure the directory containing `meshmonk_shared.dll` (e.g., `path/to/meshmonk/build/library/` or `path/to/meshmonk/build/bin/`) is in your system's `PATH` environment variable. Alternatively, you can add this directory to MATLAB's path using a `startup.m` file in your MATLAB user path (typically `Documents\MATLAB\`):
      ```matlab
      addpath('C:\path\to\meshmonk\build\library'); % Or wherever the .dll is
      ```

### 4. Running Demo Scripts

The demo scripts (e.g., `test_compute_correspondences.m`) are located in the `demo/` directory.
1.  Open MATLAB.
2.  Ensure your runtime environment is set up as described above.
3.  Navigate MATLAB's current directory to `path/to/meshmonk/demo/`.
4.  Open and run any of the `test_*.m` scripts. For example:
    ```matlab
    test_compute_correspondences
    ```

## Documentation

*   **Legacy Build Instructions:** For older, manual build instructions (not using the primary CMake system described above), you can refer to files in the `docs/` directory. Please note these are largely outdated for the current build process.
    *   [Ubuntu (Legacy)](docs/ubuntu.md)
    *   [OSX (Legacy)](docs/osx.md)
    *   [Windows (Legacy)](docs/windows.md)
