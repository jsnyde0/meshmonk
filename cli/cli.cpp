#include <iostream>
#include <string>
#include <vector>
#include <fstream> // Added for std::ofstream

// Argument parsing
#include "cxxopts.hpp"

// OpenMesh
#include "OpenMesh/Core/IO/MeshIO.hh"
#include "OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh"

// Eigen
#include <Eigen/Dense>

// MeshMonk core library
// Assuming meshmonk.hpp and global.hpp are accessible via include paths
// set in the root CMakeLists.txt for meshmonk_shared
#include "meshmonk/meshmonk.hpp" // Relative to include path, e.g., project root
#include "meshmonk/global.hpp"   // Relative to include path

// Define a default mesh type for OpenMesh
struct CLIMeshTraits : public OpenMesh::DefaultTraits
{
  // Let OpenMesh store vertex normals
  VertexAttributes(OpenMesh::Attributes::Normal);
  // Let OpenMesh store face normals
  FaceAttributes(OpenMesh::Attributes::Normal);
};
typedef OpenMesh::TriMesh_ArrayKernelT<CLIMeshTraits> MyMesh;


// Placeholder for loading OBJ mesh
bool load_obj_mesh(const std::string& filename, 
                   Eigen::MatrixXd& V, 
                   Eigen::MatrixXi& F,
                   MyMesh& mesh) {
    // Read the mesh using OpenMesh
    if (!OpenMesh::IO::read_mesh(mesh, filename)) {
        std::cerr << "Error loading mesh from " << filename << std::endl;
        return false;
    }

    // Request and compute vertex normals if not already available
    mesh.request_vertex_normals();
    if (!mesh.has_vertex_normals()) {
        std::cerr << "Error: Could not request vertex normals for: " << filename << std::endl;
        // For MeshMonk, normals are often crucial. Let's consider this fatal for now.
        // If features are to be computed differently, this might change.
    }
    mesh.update_normals(); // Compute normals
    if (!mesh.has_vertex_normals()) {
        std::cerr << "Warning: Normals could not be computed for: " << filename << std::endl;
        // Proceeding without normals, FeatureMat will have zero normals.
    }

    // Populate V (vertices)
    V.resize(mesh.n_vertices(), 3);
    for (size_t i = 0; i < mesh.n_vertices(); ++i) {
        MyMesh::Point p = mesh.point(mesh.vertex_handle(i));
        V(i, 0) = p[0];
        V(i, 1) = p[1];
        V(i, 2) = p[2];
    }

    // Populate F (faces) - 0-indexed
    F.resize(mesh.n_faces(), 3);
    for (size_t i = 0; i < mesh.n_faces(); ++i) {
        MyMesh::FaceHandle fh = mesh.face_handle(i);
        int j = 0;
        for (MyMesh::FaceVertexIter fv_it = mesh.fv_iter(fh); fv_it.is_valid(); ++fv_it) {
            if (j < 3) { // Assuming triangular faces
                F(i, j) = fv_it->idx();
            }
            j++;
        }
        if (j != 3) {
            std::cerr << "Warning: Face " << i << " is not a triangle (or error in iteration)." << std::endl;
        }
    }
    return true;
}

// Placeholder for saving OBJ mesh
bool save_obj_mesh(const std::string& filename, 
                   const MyMesh& mesh_to_save) {
    // The V and F parameters are removed as mesh_to_save is the source of truth.
    std::cout << "Saving mesh to " << filename << std::endl;
    if (!OpenMesh::IO::write_mesh(mesh_to_save, filename)) {
        std::cerr << "Error saving mesh to " << filename << std::endl;
        return false;
    }
    std::cout << "Mesh saved successfully to " << filename << std::endl;
    return true;
}

#include <sstream> // Added for std::istringstream

// Placeholder for saving transformation matrix
bool save_transform_matrix(const std::string& filename, const Eigen::Matrix4f& transform) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open file to save transform matrix: " << filename << std::endl;
        return false;
    }
    outfile << transform << std::endl;
    outfile.close();
    std::cout << "Transformation matrix saved to " << filename << std::endl;
    return true;
}

// Helper function to load corresponding points from a text file
// Each line: x y z nx ny nz
bool load_corresponding_points(const std::string& filename, FeatureMat& points) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open file for corresponding points: " << filename << std::endl;
        return false;
    }

    std::vector<std::vector<float>> data_rows;
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::vector<float> row_values;
        float val;
        int count = 0;
        while (iss >> val) {
            row_values.push_back(val);
            count++;
        }
        if (count == 6) { // Expecting 6 values: x, y, z, nx, ny, nz
            data_rows.push_back(row_values);
        } else if (count > 0) { // Non-empty line but not 6 values
            std::cerr << "Warning: Skipping malformed line in " << filename << ": " << line << " (expected 6 float values)" << std::endl;
        }
    }
    infile.close();

    if (data_rows.empty()) {
        std::cerr << "Error: No valid data found in corresponding points file: " << filename << std::endl;
        return false;
    }

    points.resize(data_rows.size(), 6);
    for (size_t i = 0; i < data_rows.size(); ++i) {
        for (size_t j = 0; j < 6; ++j) {
            points(i, j) = data_rows[i][j];
        }
    }
    std::cout << "Loaded " << points.rows() << " corresponding points from " << filename << std::endl;
    return true;
}

// Helper function to load inlier weights from a text file
// Each line: float_value
bool load_inlier_weights(const std::string& filename, VecDynFloat& weights) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open file for inlier weights: " << filename << std::endl;
        return false;
    }

    std::vector<float> weight_values;
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        float val;
        if (iss >> val) {
            // Check if there's more data on the line, which would be an error
            char remaining;
            if (iss >> remaining) {
                 std::cerr << "Warning: Malformed line in " << filename << ": " << line << " (expected single float). Using first value." << std::endl;
            }
            weight_values.push_back(val);
        } else if (!line.empty()){ // Non-empty line but not a float
            std::cerr << "Warning: Skipping non-float line in " << filename << ": " << line << std::endl;
        }
    }
    infile.close();

    if (weight_values.empty()) {
        std::cerr << "Error: No valid data found in inlier weights file: " << filename << std::endl;
        return false;
    }

    weights.resize(weight_values.size());
    for (size_t i = 0; i < weight_values.size(); ++i) {
        weights(i) = weight_values[i];
    }
    std::cout << "Loaded " << weights.size() << " inlier weights from " << filename << std::endl;
    return true;
}


int main(int argc, char* argv[]) {
    cxxopts::Options options("meshmonk_cli", "Command-line interface for MeshMonk registration tasks.");

    options.add_options()
        ("h,help", "Print usage");

    options.add_options("Global")
        ("command", "The command to execute (pyramid_reg, rigid_reg, compute_rigid_transform)", cxxopts::value<std::string>())
        // Positional arguments - these will need careful handling depending on the command
        ("arg1", "First positional argument (e.g., source_mesh, floating_mesh)", cxxopts::value<std::string>())
        ("arg2", "Second positional argument (e.g., target_mesh, corresponding_points)", cxxopts::value<std::string>())
        ("arg3", "Third positional argument (e.g., output_mesh, inlier_weights)", cxxopts::value<std::string>())
        ("arg4", "Fourth positional argument (e.g., output_mesh for compute_rigid_transform)", cxxopts::value<std::string>()->default_value("")); // Optional for some commands

    options.parse_positional({"command", "arg1", "arg2", "arg3", "arg4"});
    options.positional_help("<command> <arg1> <arg2> <arg3> [arg4]");


    // Command-specific options
    // Pyramid Registration options
    options.add_options("pyramid_reg")
        ("num_iterations", "Number of iterations per level", cxxopts::value<size_t>()->default_value("90")) // MATLAB: 90
        ("smoothness", "Smoothness term weight (transformSigma)", cxxopts::value<float>()->default_value("3.0f")) // MATLAB: 3.0
        ("num_pyramid_layers", "Number of pyramid layers", cxxopts::value<size_t>()->default_value("3")) // MATLAB: 3
        ("ds_float_start", "Downsample start % for float mesh", cxxopts::value<float>()->default_value("50.0f")) // MATLAB: 50
        ("ds_target_start", "Downsample start % for target mesh", cxxopts::value<float>()->default_value("70.0f")) // MATLAB: 70
        ("ds_float_end", "Downsample end % for float mesh", cxxopts::value<float>()->default_value("0.0f")) // MATLAB: 0.0
        ("ds_target_end", "Downsample end % for target mesh", cxxopts::value<float>()->default_value("0.0f")) // MATLAB: 0.0
        ("correspondences_symmetric", "Use symmetric correspondences", cxxopts::value<bool>()->default_value("true")) // MATLAB: true
        ("correspondences_num_neighbours", "Num neighbours for correspondences", cxxopts::value<size_t>()->default_value("3")) // MATLAB: 3
        ("correspondences_flag_threshold", "Flag threshold for correspondences", cxxopts::value<float>()->default_value("0.999f")) // MATLAB: 0.999
        ("correspondences_equalize_push_pull", "Equalize push/pull for correspondences", cxxopts::value<bool>()->default_value("false")) // MATLAB: false
        ("inlier_kappa", "Kappa value for inlier detection", cxxopts::value<float>()->default_value("12.0f")) // MATLAB: 12.0
        ("inlier_use_orientation", "Use orientation for inlier detection", cxxopts::value<bool>()->default_value("true")) // MATLAB: true
        ("transform_num_viscous_iterations_start", "Num viscous iterations (start)", cxxopts::value<size_t>()->default_value("90")) // MATLAB: 90 (numIterations)
        ("transform_num_viscous_iterations_end", "Num viscous iterations (end)", cxxopts::value<size_t>()->default_value("1")) // MATLAB: 1
        ("transform_num_elastic_iterations_start", "Num elastic iterations (start)", cxxopts::value<size_t>()->default_value("90")) // MATLAB: 90 (numIterations)
        ("transform_num_elastic_iterations_end", "Num elastic iterations (end)", cxxopts::value<size_t>()->default_value("1")); // MATLAB: 1

    // Rigid Registration options
    options.add_options("rigid_reg")
        ("transform_output", "Output file for the 4x4 transformation matrix (TXT)", cxxopts::value<std::string>()->default_value(""))
        ("rigid_num_iterations", "Number of iterations for rigid registration", cxxopts::value<size_t>()->default_value("80")) // MATLAB: 80
        ("rigid_correspondences_symmetric", "Use symmetric correspondences (rigid)", cxxopts::value<bool>()->default_value("true")) // MATLAB: true
        ("rigid_correspondences_num_neighbours", "Num neighbours for correspondences (rigid)", cxxopts::value<size_t>()->default_value("3")) // MATLAB: 3
        ("rigid_correspondences_flag_threshold", "Flag threshold for correspondences (rigid)", cxxopts::value<float>()->default_value("0.9f")) // MATLAB: 0.9
        ("rigid_correspondences_equalize_push_pull", "Equalize push/pull for correspondences (rigid)", cxxopts::value<bool>()->default_value("false")) // MATLAB: false
        ("rigid_inlier_kappa", "Kappa value for inlier detection (rigid)", cxxopts::value<float>()->default_value("12.0f")) // MATLAB: 12.0
        ("rigid_inlier_use_orientation", "Use orientation for inlier detection (rigid)", cxxopts::value<bool>()->default_value("true")) // MATLAB: true
        ("rigid_use_scaling", "Enable scaling in rigid registration", cxxopts::value<bool>()->default_value("false")); // MATLAB: false

    // Compute Rigid Transform options
    options.add_options("compute_rigid_transform")
        ("crt_transform_output", "Output file for the 4x4 transformation matrix (TXT) for compute_rigid_transform", cxxopts::value<std::string>()->default_value(""))
        ("crt_use_scaling", "Enable scaling in computation for compute_rigid_transform", cxxopts::value<bool>()->default_value("false"));

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help({"", "Global", "pyramid_reg", "rigid_reg", "compute_rigid_transform"}) << std::endl;
        return 0;
    }

    // Basic check for command
    if (!result.count("command")) {
        std::cerr << "Error: Missing command argument." << std::endl;
        std::cout << options.help({"", "Global"}) << std::endl;
        return 1;
    }
    std::string command = result["command"].as<std::string>();

    // Argument validation based on command
    std::string arg1_val, arg2_val, arg3_val, arg4_val;
    if (command == "pyramid_reg" || command == "rigid_reg") {
        if (!result.count("arg1") || !result.count("arg2") || !result.count("arg3")) {
            std::cerr << "Error: Missing arguments for " << command << ": <source_mesh> <target_mesh> <output_mesh>" << std::endl;
            std::cout << options.help({"", "Global", command}) << std::endl;
            return 1;
        }
        arg1_val = result["arg1"].as<std::string>(); // source
        arg2_val = result["arg2"].as<std::string>(); // target
        arg3_val = result["arg3"].as<std::string>(); // output
        std::cout << "Command: " << command << std::endl;
        std::cout << "Source: " << arg1_val << std::endl;
        std::cout << "Target: " << arg2_val << std::endl;
        std::cout << "Output: " << arg3_val << std::endl;
    } else if (command == "compute_rigid_transform") {
        if (!result.count("arg1") || !result.count("arg2") || !result.count("arg3") || !result.count("arg4")) {
            std::cerr << "Error: Missing arguments for " << command << ": <floating_mesh> <corresponding_points> <inlier_weights> <output_mesh>" << std::endl;
            std::cout << options.help({"", "Global", command}) << std::endl;
            return 1;
        }
        arg1_val = result["arg1"].as<std::string>(); // floating_mesh
        arg2_val = result["arg2"].as<std::string>(); // corresponding_points
        arg3_val = result["arg3"].as<std::string>(); // inlier_weights
        arg4_val = result["arg4"].as<std::string>(); // output_mesh for compute_rigid_transform
        std::cout << "Command: " << command << std::endl;
        std::cout << "Floating Mesh: " << arg1_val << std::endl;
        std::cout << "Corresponding Points: " << arg2_val << std::endl;
        std::cout << "Inlier Weights: " << arg3_val << std::endl;
        std::cout << "Output Mesh: " << arg4_val << std::endl;
    } else {
        std::cerr << "Error: Unknown command: " << command << std::endl;
        std::cout << options.help({"", "Global", "pyramid_reg", "rigid_reg", "compute_rigid_transform"}) << std::endl;
        return 1;
    }


    // Eigen matrices for mesh data (double for OpenMesh compatibility, then cast to float for MeshMonk)
    Eigen::MatrixXd source_V_d, target_V_d; // For pyramid_reg and rigid_reg
    Eigen::MatrixXd floating_V_d; // For compute_rigid_transform
    Eigen::MatrixXi source_F_i, target_F_i;
    
    // OpenMesh containers
    MyMesh source_mesh, target_mesh, result_mesh, floating_mesh; // Added floating_mesh
    Eigen::MatrixXi floating_F_i; // source_F_i and target_F_i declared above

    // Prepare data based on command
    FeatureMat source_features, target_features; // For pyramid_reg and rigid_reg
    FacesMat source_faces_mat, target_faces_mat; // For pyramid_reg and rigid_reg
    VecDynFloat source_flags, target_flags;     // For pyramid_reg and rigid_reg

    FeatureMat floating_features; // For compute_rigid_transform
    // corresponding_points and inlier_weights will be loaded from files for compute_rigid_transform

    if (command == "pyramid_reg" || command == "rigid_reg") {
        // Load source and target meshes
        std::cout << "Loading source mesh: " << arg1_val << std::endl;
        if (!load_obj_mesh(arg1_val, source_V_d, source_F_i, source_mesh)) {
            // load_obj_mesh prints specific error
            return 1;
        }
        std::cout << "Loading target mesh: " << arg2_val << std::endl;
        if (!load_obj_mesh(arg2_val, target_V_d, target_F_i, target_mesh)) {
            // load_obj_mesh prints specific error
            return 1;
        }

        // Initialize result mesh with source data for these commands
        result_mesh = source_mesh;

        // Prepare data for MeshMonk
        source_faces_mat = source_F_i.cast<int>();
        target_faces_mat = target_F_i.cast<int>();

        source_features.resize(source_mesh.n_vertices(), registration::NUM_FEATURES);
        for (size_t i = 0; i < source_mesh.n_vertices(); ++i) {
            MyMesh::Point p = source_mesh.point(source_mesh.vertex_handle(i));
            source_features(i, 0) = static_cast<float>(p[0]);
            source_features(i, 1) = static_cast<float>(p[1]);
            source_features(i, 2) = static_cast<float>(p[2]);
            if (source_mesh.has_vertex_normals()) {
                MyMesh::Normal n = source_mesh.normal(source_mesh.vertex_handle(i));
                source_features(i, 3) = static_cast<float>(n[0]);
                source_features(i, 4) = static_cast<float>(n[1]);
                source_features(i, 5) = static_cast<float>(n[2]);
            } else {
                source_features(i, 3) = 0.0f; source_features(i, 4) = 0.0f; source_features(i, 5) = 0.0f;
            }
        }

        target_features.resize(target_mesh.n_vertices(), registration::NUM_FEATURES);
        for (size_t i = 0; i < target_mesh.n_vertices(); ++i) {
            MyMesh::Point p = target_mesh.point(target_mesh.vertex_handle(i));
            target_features(i, 0) = static_cast<float>(p[0]);
            target_features(i, 1) = static_cast<float>(p[1]);
            target_features(i, 2) = static_cast<float>(p[2]);
            if (target_mesh.has_vertex_normals()) {
                MyMesh::Normal n = target_mesh.normal(target_mesh.vertex_handle(i));
                target_features(i, 3) = static_cast<float>(n[0]);
                target_features(i, 4) = static_cast<float>(n[1]);
                target_features(i, 5) = static_cast<float>(n[2]);
            } else {
                target_features(i, 3) = 0.0f; target_features(i, 4) = 0.0f; target_features(i, 5) = 0.0f;
            }
        }
        source_flags = VecDynFloat::Ones(source_mesh.n_vertices());
        target_flags = VecDynFloat::Ones(target_mesh.n_vertices());
    } else if (command == "compute_rigid_transform") {
        // Load floating mesh
        std::cout << "Loading floating mesh: " << arg1_val << std::endl;
        if (!load_obj_mesh(arg1_val, floating_V_d, floating_F_i, floating_mesh)) {
            // load_obj_mesh prints specific error
            return 1;
        }
        // Initialize result mesh with floating data for this command
        result_mesh = floating_mesh;

        floating_features.resize(floating_mesh.n_vertices(), registration::NUM_FEATURES);
        for (size_t i = 0; i < floating_mesh.n_vertices(); ++i) {
            MyMesh::Point p = floating_mesh.point(floating_mesh.vertex_handle(i));
            floating_features(i, 0) = static_cast<float>(p[0]);
            floating_features(i, 1) = static_cast<float>(p[1]);
            floating_features(i, 2) = static_cast<float>(p[2]);
            // Normals might not be used directly by compute_rigid_transform, but populate if available
            if (floating_mesh.has_vertex_normals()) {
                MyMesh::Normal n = floating_mesh.normal(floating_mesh.vertex_handle(i));
                floating_features(i, 3) = static_cast<float>(n[0]);
                floating_features(i, 4) = static_cast<float>(n[1]);
                floating_features(i, 5) = static_cast<float>(n[2]);
            } else {
                floating_features(i, 3) = 0.0f; floating_features(i, 4) = 0.0f; floating_features(i, 5) = 0.0f;
            }
        }
        // Corresponding points and inlier weights (arg2_val, arg3_val) need to be loaded here.
        // This will require new helper functions, similar to load_obj_mesh.
        // For now, just printing them.
        std::cout << "Path to corresponding points: " << arg2_val << std::endl;
        std::cout << "Path to inlier weights: " << arg3_val << std::endl;
    }


    if (command == "pyramid_reg") {
        std::cout << "Executing pyramid registration..." << std::endl;
        
        meshmonk::pyramid_registration(
            source_features, target_features, // These are populated for pyramid_reg
            source_faces_mat, target_faces_mat, // These are populated for pyramid_reg
            source_flags, target_flags, // These are populated for pyramid_reg
            result["num_iterations"].as<size_t>(),
            result["num_pyramid_layers"].as<size_t>(),
            result["ds_float_start"].as<float>(),
            result["ds_target_start"].as<float>(),
            result["ds_float_end"].as<float>(),
            result["ds_target_end"].as<float>(),
            result["correspondences_symmetric"].as<bool>(),
            result["correspondences_num_neighbours"].as<size_t>(),
            result["correspondences_flag_threshold"].as<float>(),
            result["correspondences_equalize_push_pull"].as<bool>(),
            result["inlier_kappa"].as<float>(),
            result["inlier_use_orientation"].as<bool>(),
            result["smoothness"].as<float>(), // transformSigma
            result["transform_num_viscous_iterations_start"].as<size_t>(),
            result["transform_num_viscous_iterations_end"].as<size_t>(),
            result["transform_num_elastic_iterations_start"].as<size_t>(),
            result["transform_num_elastic_iterations_end"].as<size_t>()
        );
        
        // Update result_mesh from the modified source_features (positions only)
        for (size_t i = 0; i < result_mesh.n_vertices(); ++i) {
            result_mesh.set_point(result_mesh.vertex_handle(i),
                                  MyMesh::Point(source_features(i, 0),
                                                source_features(i, 1),
                                                source_features(i, 2)));
        }
        result_mesh.request_vertex_normals(); // Request normals
        result_mesh.update_normals(); // Recompute normals after deformation

        std::cout << "Pyramid registration complete." << std::endl;

    } else if (command == "rigid_reg") {
        std::cout << "Executing rigid registration..." << std::endl;
        // Accessing option directly as per cxxopts behavior for matched command group
        std::string transform_output_file_opt = result["transform_output"].as<std::string>();
        Eigen::Matrix4f transform_matrix = Eigen::Matrix4f::Identity();

        meshmonk::rigid_registration(
            source_features, target_features, // These are populated for rigid_reg
            source_faces_mat, target_faces_mat, // These are populated for rigid_reg
            source_flags, target_flags, // These are populated for rigid_reg
            transform_matrix, // This is modified by rigid_registration
            result["rigid_num_iterations"].as<size_t>(),
            result["rigid_correspondences_symmetric"].as<bool>(),
            result["rigid_correspondences_num_neighbours"].as<size_t>(),
            result["rigid_correspondences_flag_threshold"].as<float>(),
            result["rigid_correspondences_equalize_push_pull"].as<bool>(),
            result["rigid_inlier_kappa"].as<float>(),
            result["rigid_inlier_use_orientation"].as<bool>(),
            result["rigid_use_scaling"].as<bool>() // This option is from rigid_reg group
        );
        
        // Update result_mesh from the modified source_features (positions only)
        for (size_t i = 0; i < result_mesh.n_vertices(); ++i) {
            result_mesh.set_point(result_mesh.vertex_handle(i),
                                  MyMesh::Point(source_features(i, 0),
                                                source_features(i, 1),
                                                source_features(i, 2)));
        }
        result_mesh.request_vertex_normals(); // Request normals
        result_mesh.update_normals(); // Recompute normals after transformation

        std::cout << "Rigid registration complete." << std::endl;

        if (!transform_output_file_opt.empty()) {
            save_transform_matrix(transform_output_file_opt, transform_matrix);
        }
    } else if (command == "compute_rigid_transform") {
        std::cout << "Executing compute_rigid_transform..." << std::endl;
        // Accessing options directly as per cxxopts behavior for matched command group
        std::string transform_output_file_crt = result["crt_transform_output"].as<std::string>();
        bool use_scaling_crt = result["crt_use_scaling"].as<bool>();
        Eigen::Matrix4f transform_matrix_crt = Eigen::Matrix4f::Identity();

        // Load corresponding points and inlier weights
        FeatureMat corresponding_points_mat;
        if (!load_corresponding_points(arg2_val, corresponding_points_mat)) {
            // load_corresponding_points prints specific error
            return 1;
        }

        VecDynFloat inlier_weights_vec;
        if (!load_inlier_weights(arg3_val, inlier_weights_vec)) {
            // load_inlier_weights prints specific error
            return 1;
        }

        // Validate sizes
        if (floating_features.rows() != corresponding_points_mat.rows() ||
            floating_features.rows() != inlier_weights_vec.size()) {
            std::cerr << "Error: Mismatch in number of elements:" << std::endl;
            std::cerr << "  Floating mesh vertices: " << floating_features.rows() << std::endl;
            std::cerr << "  Corresponding points: " << corresponding_points_mat.rows() << std::endl;
            std::cerr << "  Inlier weights: " << inlier_weights_vec.size() << std::endl;
            std::cerr << "These counts must all be equal." << std::endl;
            return 1;
        }

        // Call MeshMonk's compute_rigid_transformation function
        // Assuming the signature:
        // void compute_rigid_transformation(
        //      const FeatureMat& floating_mesh_features, // Input: Nx6 features of floating mesh (pos+normal)
        //      const FeatureMat& corresponding_target_points, // Input: Nx6 features of targets (pos+normal)
        //      const VecDynFloat& inlier_weights,        // Input: Nx1 weights for each correspondence
        //      Eigen::Matrix4f& transformation_matrix, // Output: 4x4 transformation
        //      bool use_scaling);                       // Input: flag to allow scaling

        meshmonk::compute_rigid_transformation(
            floating_features,          // All 6 features (pos+normals) of the floating mesh
            corresponding_points_mat,   // All 6 features (pos+normals) of the target points
            inlier_weights_vec,
            transform_matrix_crt,       // This will be updated by the function
            use_scaling_crt
        );

        std::cout << "Compute rigid transform executed." << std::endl;
        std::cout << "  Use scaling: " << (use_scaling_crt ? "true" : "false") << std::endl;
        std::cout << "Computed Transformation Matrix:" << std::endl << transform_matrix_crt << std::endl;

        // Apply the computed transformation to the result_mesh vertices
        for (MyMesh::VertexIter v_it = result_mesh.vertices_begin(); v_it != result_mesh.vertices_end(); ++v_it) {
            MyMesh::Point p = result_mesh.point(*v_it);
            Eigen::Vector4f P_homogeneous(p[0], p[1], p[2], 1.0f);
            Eigen::Vector4f P_transformed_homogeneous = transform_matrix_crt * P_homogeneous;
            result_mesh.set_point(*v_it, MyMesh::Point(P_transformed_homogeneous[0], P_transformed_homogeneous[1], P_transformed_homogeneous[2]));
        }

        if (result_mesh.has_vertex_normals()) { // Check if normals exist before trying to update
            result_mesh.request_vertex_normals(); // Ensure they are available for update
            result_mesh.update_normals(); // Recompute normals after transformation
            std::cout << "Vertex normals updated." << std::endl;
        } else {
            std::cout << "No vertex normals to update on result mesh." << std::endl;
        }


        if (!transform_output_file_crt.empty()) {
            save_transform_matrix(transform_output_file_crt, transform_matrix_crt);
        }
        std::cout << "Compute rigid transform complete." << std::endl;
        // Output mesh for this command is arg4_val
        std::cout << "Saving output mesh to " << arg4_val << "..." << std::endl;
        if (!save_obj_mesh(arg4_val, result_mesh)) {
            // save_obj_mesh prints specific error
            return 1;
        }
        std::cout << "MeshMonk CLI execution finished for compute_rigid_transform." << std::endl;
        return 0; // Exit early as this command has its own output handling.
    }


    // Common output saving for pyramid_reg and rigid_reg (uses arg3_val as output_file)
    std::cout << "Saving output mesh to " << arg3_val << "..." << std::endl;
    if (!save_obj_mesh(arg3_val, result_mesh)) {
         // save_obj_mesh prints specific error
        return 1;
    }

    std::cout << "MeshMonk CLI execution finished." << std::endl;

    return 0;
}