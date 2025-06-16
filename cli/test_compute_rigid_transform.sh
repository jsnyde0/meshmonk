#!/bin/bash

# Script to test the compute_rigid_transform command of meshmonk_cli
# using a simple, verifiable test case.

# --- Configuration ---
set -e # Exit immediately if a command exits with a non-zero status.
set -o pipefail # The return value of a pipeline is the status of the last command to exit with a non-zero status.

# --- Directories and Paths ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
EXECUTABLE="$BUILD_DIR/cli/meshmonk_cli"
TEST_DATA_BASE_DIR="$SCRIPT_DIR/test_data"
TEST_DATA_DIR="$TEST_DATA_BASE_DIR/rigid_transform"

# --- Input Files (8-point cube for pure translation) ---
INPUT_MESH_OBJ="$TEST_DATA_DIR/input_mesh.obj"
INPUT_FEATURES="$TEST_DATA_DIR/corresponding_features.txt"
INPUT_WEIGHTS="$TEST_DATA_DIR/inlier_weights.txt"

# --- Expected Output Files (For verification) ---
EXPECTED_TRANSFORM="$TEST_DATA_DIR/expected_transform.txt"

# --- Generated Output Files (Will be created by this script) ---
OUTPUT_MESH_OBJ="$TEST_DATA_DIR/output_mesh_generated.obj"
OUTPUT_TRANSFORM="$TEST_DATA_DIR/output_transform_generated.txt"

# --- Helper Functions ---
fail() {
    echo "🔴 Test failed: $1"
    exit 1
}

# Function to compare two files containing floating-point numbers with a tolerance.
# This version is robust to differences in whitespace and line breaks.
compare_matrices() {
    local file1="$1"
    local file2="$2"
    local tolerance="$3"

    # Read all numbers from both files into arrays, ignoring whitespace and newlines.
    local arr1=($(cat "$file1"))
    local arr2=($(cat "$file2"))

    # Check if the number of elements is the same
    if [ "${#arr1[@]}" -ne "${#arr2[@]}" ]; then
        echo "Files have a different number of numeric values."
        echo "  File 1 (${#arr1[@]} values): ${arr1[*]}"
        echo "  File 2 (${#arr2[@]} values): ${arr2[*]}"
        return 1
    fi

    # Compare elements one by one
    for i in "${!arr1[@]}"; do
        # Use awk for robust floating point comparison
        local result=$(awk -v v1="${arr1[$i]}" -v v2="${arr2[$i]}" -v tol="$tolerance" '
            BEGIN {
                diff = v1 - v2;
                if (diff < 0) diff = -diff;
                if (diff > tol) {
                    exit 1; # Fails if difference is greater than tolerance
                }
                exit 0; # Succeeds otherwise
            }
        ')
        if [ $? -ne 0 ]; then
            echo "Mismatch at element $((i+1)):"
            echo "  Expected: ${arr1[$i]}"
            echo "  Got:      ${arr2[$i]}"
            return 1
        fi
    done

    return 0
}


# --- Main Script ---
echo "▶️  Starting test for 'compute_rigid_transform'..."

# 1. Clean up previous output files
echo "  - Cleaning up old output files..."
rm -f "$OUTPUT_MESH_OBJ" "$OUTPUT_TRANSFORM"

# 2. Check for required executable and files
echo "  - Verifying that required files exist..."
if [ ! -f "$EXECUTABLE" ]; then
    fail "Executable not found at $EXECUTABLE. Please build the project first."
fi
if [ ! -f "$INPUT_MESH_OBJ" ] || [ ! -f "$INPUT_FEATURES" ] || [ ! -f "$INPUT_WEIGHTS" ]; then
    fail "One or more test data files not found in $TEST_DATA_DIR. Please generate them first."
fi
echo "  - All required files found."

# 3. Run the CLI command with POSITIONAL arguments
echo "  - Running 'meshmonk_cli compute_rigid_transform'..."
"$EXECUTABLE" compute_rigid_transform \
    "$INPUT_MESH_OBJ" \
    "$INPUT_FEATURES" \
    "$INPUT_WEIGHTS" \
    "$OUTPUT_MESH_OBJ" \
    --crt_transform_output "$OUTPUT_TRANSFORM"

# Check exit code
CLI_EXIT_CODE=$?
if [ $CLI_EXIT_CODE -ne 0 ]; then
    fail "meshmonk_cli command failed with exit code $CLI_EXIT_CODE."
fi
echo "  - CLI command executed successfully."

# 4. Check that output files were created
echo "  - Checking for generated output files..."
if [ ! -s "$OUTPUT_MESH_OBJ" ] || [ ! -s "$OUTPUT_TRANSFORM" ]; then
    fail "One or both output files ($OUTPUT_MESH_OBJ, $OUTPUT_TRANSFORM) were not created or are empty."
fi
echo "  - Output files were created successfully."

# 5. Verify the output against expected results using the robust comparison
echo "  - Verifying generated transformation matrix..."
if ! compare_matrices "$EXPECTED_TRANSFORM" "$OUTPUT_TRANSFORM" "1e-5"; then
    fail "Verification failed. The generated transformation matrix does not match the expected one."
fi
echo "  - Transformation matrix is correct."

# --- Success ---
echo "✅ All checks passed. Test successful!"
exit 0