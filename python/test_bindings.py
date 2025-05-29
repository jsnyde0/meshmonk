import numpy as np
import meshmonk_python

print("Successfully imported 'meshmonk_python'")

# Check if the NUM_FEATURES attribute exists and print it
if hasattr(meshmonk_python, 'NUM_FEATURES'):
    NUM_FEATURES = meshmonk_python.NUM_FEATURES
    print(f"meshmonk_python.NUM_FEATURES = {NUM_FEATURES}")
else:
    print("Error: meshmonk_python.NUM_FEATURES not found!")
    exit(1)

N_POINTS = 100  # Number of points in the feature sets
# Transformation matrix for rigid is (NUM_FEATURES+1)x(NUM_FEATURES+1) if NUM_FEATURES is 3 (for 3D points)
# However, MeshMonk's rigid_registration expects a 4x4 matrix for 3D data (compatibility with graphics transformations)
# Since NUM_FEATURES can be generic (e.g. 6), this needs clarification.
# For now, assuming the C++ side handles the matrix appropriately if it's 3D data + other features.
# The C++ error for rigid_registration said transformation_matrix must be 4x4.
# This implies rigid registration primarily works on the first 3 features as 3D coordinates.
# Let's ensure NUM_FEATURES >= 3 for this test if a 4x4 matrix is hardcoded.
# The current NUM_FEATURES is 6. The C++ wrapper enforces a 4x4 matrix. This seems like a mismatch
# if we are to transform all NUM_FEATURES features.
# I will proceed with NUM_FEATURES = 3 for rigid test for now to match 4x4 matrix.
# Or, I need to adjust the C++ wrapper for transformation_matrix for rigid if it should handle generic NUM_FEATURES.
# Given the error "Transformation matrix NumPy array must be of shape (4, 4)", I must provide a 4x4 matrix.
# This means for rigid registration, we are implicitly assuming the first 3 cols of features are XYZ.

# 1. Test rigid_registration
print("\nTesting rigid_registration...")
# To match the 4x4 transformation_matrix, let's assume features are XYZ + others
# The rigid transformation will likely only apply to XYZ.
target_features_rigid = np.random.rand(N_POINTS, NUM_FEATURES).astype(np.float32)
floating_features_rigid = target_features_rigid + np.random.rand(N_POINTS, NUM_FEATURES).astype(np.float32) * 0.1
transformation_matrix_rigid = np.eye(4, dtype=np.float32) # Rigid transform is 4x4

# Make copies for checking modification
floating_features_rigid_orig = np.copy(floating_features_rigid)
transformation_matrix_rigid_orig = np.copy(transformation_matrix_rigid)

# Create dummy faces and flags for rigid registration
num_dummy_faces_rigid = N_POINTS // 3 if N_POINTS >= 3 else 0
if num_dummy_faces_rigid * 3 > N_POINTS :
    num_dummy_faces_rigid = (N_POINTS // 3) -1 if (N_POINTS // 3) > 0 else 0

if N_POINTS >= 3 and num_dummy_faces_rigid > 0:
    dummy_faces_rigid_arr = np.array([[i, i+1, i+2] for i in range(0, num_dummy_faces_rigid * 3, 3)], dtype=np.int32)
else:
    dummy_faces_rigid_arr = np.empty((0,3), dtype=np.int32)

dummy_flags_rigid_arr = np.ones(N_POINTS, dtype=np.float32) # Example: all flags = 1.0

try:
    meshmonk_python.rigid_registration(
        floating_features_rigid, # (N, NUM_FEATURES)
        target_features_rigid,   # (N, NUM_FEATURES)
        floating_faces=dummy_faces_rigid_arr,
        target_faces=dummy_faces_rigid_arr, # Using same dummy faces for target
        floating_flags=dummy_flags_rigid_arr,
        target_flags=dummy_flags_rigid_arr, # Using same dummy flags for target
        transformation_matrix=transformation_matrix_rigid, # (4,4)
        num_iterations=10
    )
    print("rigid_registration called successfully.")

    # The first 3 columns of floating_features_rigid should be modified if NUM_FEATURES >= 3
    if not np.array_equal(floating_features_rigid, floating_features_rigid_orig):
        print("floating_features_rigid was modified by rigid_registration (as expected).")
    else:
        print("WARNING: floating_features_rigid was NOT modified by rigid_registration.")

    if not np.array_equal(transformation_matrix_rigid, transformation_matrix_rigid_orig):
        print("transformation_matrix_rigid was modified by rigid_registration (as expected).")
    else:
        print("WARNING: transformation_matrix_rigid was NOT modified by rigid_registration.")

except Exception as e:
    print(f"Error during rigid_registration: {e}")

# 2. Test nonrigid_registration
print("\nTesting nonrigid_registration...")
target_features_nonrigid = np.random.rand(N_POINTS, NUM_FEATURES).astype(np.float32)
floating_features_nonrigid = target_features_nonrigid + np.random.rand(N_POINTS, NUM_FEATURES).astype(np.float32) * 0.1

# Make copies for checking modification
floating_features_nonrigid_orig = np.copy(floating_features_nonrigid)

# Create dummy faces and flags for nonrigid registration
num_dummy_faces_nonrigid = N_POINTS // 3 if N_POINTS >= 3 else 0
if num_dummy_faces_nonrigid * 3 > N_POINTS:
    num_dummy_faces_nonrigid = (N_POINTS // 3) -1 if (N_POINTS // 3) > 0 else 0

if N_POINTS >= 3 and num_dummy_faces_nonrigid > 0:
    dummy_faces_nonrigid_arr = np.array([[i, i+1, i+2] for i in range(0, num_dummy_faces_nonrigid * 3, 3)], dtype=np.int32)
else:
    dummy_faces_nonrigid_arr = np.empty((0,3), dtype=np.int32)

dummy_flags_nonrigid_arr = np.ones(N_POINTS, dtype=np.float32)

try:
    meshmonk_python.nonrigid_registration(
        floating_features_nonrigid,
        target_features_nonrigid,
        floating_faces=dummy_faces_nonrigid_arr,
        target_faces=dummy_faces_nonrigid_arr, # Using same dummy faces
        floating_flags=dummy_flags_nonrigid_arr,
        target_flags=dummy_flags_nonrigid_arr, # Using same dummy flags
        num_iterations=10,
        correspondences_num_neighbours=5 # Example of overriding a default
    )
    print("nonrigid_registration called successfully.")

    if not np.array_equal(floating_features_nonrigid, floating_features_nonrigid_orig):
        print("floating_features_nonrigid was modified by nonrigid_registration (as expected).")
    else:
        print("WARNING: floating_features_nonrigid was NOT modified by nonrigid_registration.")

except Exception as e:
    print(f"Error during nonrigid_registration: {e}")

# 3. Test pyramid_registration
print("\nTesting pyramid_registration...")
target_features_pyramid = np.random.rand(N_POINTS, NUM_FEATURES).astype(np.float32)
floating_features_pyramid = target_features_pyramid + np.random.rand(N_POINTS, NUM_FEATURES).astype(np.float32) * 0.1

# Pyramid registration requires faces. Create dummy faces.
# Number of faces: ensure it's less than N_POINTS / 3.
num_dummy_faces = N_POINTS // 3
if num_dummy_faces * 3 > N_POINTS :
    num_dummy_faces = (N_POINTS // 3) -1 if (N_POINTS // 3) > 0 else 0


if N_POINTS >= 3 and num_dummy_faces > 0:
    floating_faces_pyramid_arr = np.array([[i, i+1, i+2] for i in range(0, num_dummy_faces * 3, 3)], dtype=np.int32)
    target_faces_pyramid_arr = np.copy(floating_faces_pyramid_arr)
else: # Not enough points to make even one face
    floating_faces_pyramid_arr = np.empty((0,3), dtype=np.int32)
    target_faces_pyramid_arr = np.empty((0,3), dtype=np.int32)


# Make copies for checking modification
floating_features_pyramid_orig = np.copy(floating_features_pyramid)

# Dummy flags for pyramid registration
dummy_flags_pyramid_arr = np.ones(N_POINTS, dtype=np.float32)

try:
    # Faces are mandatory for pyramid in this binding. Flags are now also provided.
    meshmonk_python.pyramid_registration(
        floating_features_pyramid,
        target_features_pyramid,
        floating_faces_pyramid_arr,
        target_faces_pyramid_arr,
        floating_flags=dummy_flags_pyramid_arr,
        target_flags=dummy_flags_pyramid_arr, # Using same dummy flags
        num_iterations=10, 
        num_pyramid_layers=2
    )
    print("pyramid_registration called successfully.")

    if not np.array_equal(floating_features_pyramid, floating_features_pyramid_orig):
        print("floating_features_pyramid was modified by pyramid_registration (as expected).")
    else:
        print("WARNING: floating_features_pyramid was NOT modified by pyramid_registration.")

except Exception as e:
    print(f"Error during pyramid_registration: {e}")

print("\nTest script finished.")
