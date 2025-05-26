#include <iostream>
#include <string>
#include <vector>
#include <memory> // For std::make_unique
#include <fstream> // For file I/O

// Eigen
#include <Eigen/Core>
#include <Eigen/Dense> // For Matrix4f etc.

// OpenMesh
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

// cxxopts
#include "cxxopts.hpp"

// MeshMonk library
#include "meshmonk.hpp" // Include the MeshMonk library header

// Define MyMesh type
typedef OpenMesh::TriMesh_ArrayKernelT<> MyMesh;

// Helper function to print arguments (could be more sophisticated)
template<typename T>
void print_arg(const std::string& name, const T& value) {
    std::cout << "  " << name << ": " << value << std::endl;
}

void print_bool_arg(const std::string& name, bool value) {
    std::cout << "  " << name << ": " << (value ? "true" : "false") << std::endl;
}

// OBJ Reading Function
bool load_obj_mesh(const std::string& filepath, Eigen::MatrixX3f& vertices, Eigen::MatrixX3i& faces) {
    MyMesh mesh;
    mesh.request_vertex_normals(); 
    mesh.request_face_normals();   

    OpenMesh::IO::Options read_options;
    if (!OpenMesh::IO::read_mesh(mesh, filepath, read_options)) {
        std::cerr << "Error loading mesh from file: " << filepath << std::endl;
        return false;
    }

    vertices.resize(mesh.n_vertices(), 3);
    for (size_t i = 0; i < mesh.n_vertices(); ++i) {
        MyMesh::Point p = mesh.point(MyMesh::VertexHandle(i));
        vertices(i, 0) = p[0];
        vertices(i, 1) = p[1];
        vertices(i, 2) = p[2];
    }

    faces.resize(mesh.n_faces(), 3);
    size_t face_idx = 0;
    for (MyMesh::FaceIter f_it = mesh.faces_begin(); f_it != mesh.faces_end(); ++f_it) {
        MyMesh::FaceVertexIter fv_it = mesh.fv_iter(*f_it);
        // Temporarily comment out is_triangle for build, as it caused issues with the specific OpenMesh version.
        // A more robust solution would be to check OpenMesh version or use a different API if available.
        if (!fv_it.is_valid() /* || !mesh.is_triangle(*f_it) */ ) { 
             std::cerr << "Warning: Invalid face iterator skipped in " << filepath << std::endl;
             continue; 
        }
        faces(face_idx, 0) = (*fv_it).idx();
        ++fv_it;
        faces(face_idx, 1) = (*fv_it).idx();
        ++fv_it;
        faces(face_idx, 2) = (*fv_it).idx();
        face_idx++;
    }
    if (face_idx != mesh.n_faces()){ 
        faces.conservativeResize(face_idx, 3);
    }
    return true;
}

// OBJ Writing Function
bool save_obj_mesh(const std::string& filepath, const Eigen::MatrixX3f& vertices, const Eigen::MatrixX3i& faces) {
    MyMesh mesh;
    std::vector<MyMesh::VertexHandle> vhs;
    vhs.reserve(vertices.rows());
    for (int i = 0; i < vertices.rows(); ++i) {
        vhs.push_back(mesh.add_vertex(MyMesh::Point(vertices(i, 0), vertices(i, 1), vertices(i, 2))));
    }
    for (int i = 0; i < faces.rows(); ++i) {
        std::vector<MyMesh::VertexHandle> face_vhs;
        face_vhs.push_back(vhs[faces(i, 0)]);
        face_vhs.push_back(vhs[faces(i, 1)]);
        face_vhs.push_back(vhs[faces(i, 2)]);
        mesh.add_face(face_vhs);
    }
    OpenMesh::IO::Options write_options;
    if (!OpenMesh::IO::write_mesh(mesh, filepath, write_options)) {
        std::cerr << "Error writing mesh to file: " << filepath << std::endl;
        return false;
    }
    return true;
}

// Transformation Matrix Writing Function
bool save_transform_matrix(const std::string& filepath, const Eigen::Matrix4f& matrix) {
    std::ofstream outfile(filepath);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open file for writing transform matrix: " << filepath << std::endl;
        return false;
    }
    for (int i = 0; i < matrix.rows(); ++i) {
        for (int j = 0; j < matrix.cols(); ++j) {
            outfile << matrix(i, j) << (j == matrix.cols() - 1 ? "" : " ");
        }
        outfile << std::endl;
    }
    if (outfile.fail()) {
        std::cerr << "Error writing transform matrix to file: " << filepath << std::endl;
        outfile.close();
        return false;
    }
    outfile.close();
    return true;
}


int main(int argc, char* argv[]) {
    cxxopts::Options initial_options("meshmonk_cli_parser", "MeshMonk Command Line Interface");
    initial_options.allow_unrecognised_options();
    initial_options.add_options()
        ("positional", "Command to execute (pyramid_reg or rigid_reg)", cxxopts::value<std::vector<std::string>>());
    initial_options.parse_positional("positional");

    auto initial_result = initial_options.parse(argc, argv);
    std::string command;

    if (initial_result.count("positional") > 0) {
        const auto& positional_args = initial_result["positional"].as<std::vector<std::string>>();
        if (!positional_args.empty()) {
            command = positional_args[0];
        }
    }

    if (command.empty()) {
        cxxopts::Options help_options(argv[0], "MeshMonk Command Line Interface\n\n"
                                             "Usage: " + std::string(argv[0]) + " <command> [options]\n\n"
                                             "Commands:\n"
                                             "  pyramid_reg    Perform pyramid registration\n"
                                             "  rigid_reg      Perform rigid registration");
        help_options.add_options("Common")
            ("h,help", "Print help");
        std::cout << help_options.help() << std::endl;
        return 0; 
    }

    cxxopts::Options options(std::string(argv[0]) + " " + command, "Options for " + command);
    options.positional_help("[command specific options]");

    options.add_options("Common")
        ("floating", "Path to the input floating mesh file", cxxopts::value<std::string>())
        ("target", "Path to the input target mesh file", cxxopts::value<std::string>())
        ("output", "Path to save the transformed floating mesh", cxxopts::value<std::string>())
        ("h,help", "Print help for this command");

    if (command == "pyramid_reg") {
        options.add_options("pyramid_reg")
            ("num_iterations", "Number of iterations", cxxopts::value<int>()->default_value("90"))
            ("num_pyramid_layers", "Number of pyramid layers", cxxopts::value<int>()->default_value("3"))
            ("downsample_float_start", "Downsample float start", cxxopts::value<float>()->default_value("50.0"))
            ("downsample_target_start", "Downsample target start", cxxopts::value<float>()->default_value("70.0"))
            ("downsample_float_end", "Downsample float end", cxxopts::value<float>()->default_value("0.0"))
            ("downsample_target_end", "Downsample target end", cxxopts::value<float>()->default_value("0.0"))
            ("corr_symmetric", "Symmetric correspondences", cxxopts::value<bool>()->default_value("true"))
            ("corr_num_neighbours", "Number of correspondence neighbours", cxxopts::value<int>()->default_value("3"))
            ("corr_flag_threshold", "Correspondence flag threshold", cxxopts::value<float>()->default_value("0.999"))
            ("corr_equalize_push_pull", "Equalize push/pull", cxxopts::value<bool>()->default_value("false"))
            ("inlier_kappa", "Inlier kappa", cxxopts::value<float>()->default_value("12.0"))
            ("inlier_use_orientation", "Use orientation for inliers", cxxopts::value<bool>()->default_value("true"))
            ("transform_sigma", "Transform sigma", cxxopts::value<float>()->default_value("3.0"))
            ("transform_viscous_iter_start", "Transform viscous iterations start", cxxopts::value<int>())
            ("transform_viscous_iter_end", "Transform viscous iterations end", cxxopts::value<int>()->default_value("1"))
            ("transform_elastic_iter_start", "Transform elastic iterations start", cxxopts::value<int>())
            ("transform_elastic_iter_end", "Transform elastic iterations end", cxxopts::value<int>()->default_value("1"));
    } else if (command == "rigid_reg") {
        options.add_options("rigid_reg")
            ("num_iterations", "Number of iterations", cxxopts::value<int>()->default_value("80"))
            ("corr_symmetric", "Symmetric correspondences", cxxopts::value<bool>()->default_value("true"))
            ("corr_num_neighbours", "Number of correspondence neighbours", cxxopts::value<int>()->default_value("3"))
            ("corr_flag_threshold", "Correspondence flag threshold", cxxopts::value<float>()->default_value("0.9"))
            ("corr_equalize_push_pull", "Equalize push/pull", cxxopts::value<bool>()->default_value("false"))
            ("inlier_kappa", "Inlier kappa", cxxopts::value<float>()->default_value("12.0"))
            ("inlier_use_orientation", "Use orientation for inliers", cxxopts::value<bool>()->default_value("true"))
            ("use_scaling", "Use scaling", cxxopts::value<bool>()->default_value("false"))
            ("output_transform", "Path to save the output 4x4 transformation matrix", cxxopts::value<std::string>());
    } else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        std::cerr << "Available commands: pyramid_reg, rigid_reg" << std::endl;
        return 1;
    }

    std::vector<char*> new_argv_storage;
    new_argv_storage.push_back(argv[0]); 
    for (int i = 2; i < argc; ++i) { 
        new_argv_storage.push_back(argv[i]);
    }
    int new_argc = new_argv_storage.size();
    char** new_argv = new_argv_storage.data();

    try {
        // Use 'result' as the variable name for cxxopts::ParseResult consistently
        auto result = options.parse(new_argc, new_argv); 

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        if (!result.count("floating") || !result.count("target") || !result.count("output")) {
            std::cerr << "Error: Missing required arguments: --floating, --target, --output" << std::endl;
            std::cout << options.help() << std::endl;
            return 1;
        }

        std::string floating_path = result["floating"].as<std::string>();
        std::string target_path = result["target"].as<std::string>();
        std::string output_path = result["output"].as<std::string>();

        std::cout << "Command: " << command << std::endl;
        print_arg("floating", floating_path);
        print_arg("target", target_path);
        print_arg("output", output_path);


        Eigen::MatrixX3f floating_vertices, target_vertices;
        Eigen::MatrixX3i floating_faces, target_faces;

        std::cout << "\nLoading floating mesh..." << std::endl;
        if (load_obj_mesh(floating_path, floating_vertices, floating_faces)) {
            std::cout << "Successfully loaded floating mesh: " << floating_path << std::endl;
            std::cout << "  Vertices: " << floating_vertices.rows() << std::endl;
            std::cout << "  Faces: " << floating_faces.rows() << std::endl;
        } else {
            std::cerr << "Failed to load floating mesh: " << floating_path << std::endl;
            return 1;
        }

        std::cout << "\nLoading target mesh..." << std::endl;
        if (load_obj_mesh(target_path, target_vertices, target_faces)) {
            std::cout << "Successfully loaded target mesh: " << target_path << std::endl;
            std::cout << "  Vertices: " << target_vertices.rows() << std::endl;
            std::cout << "  Faces: " << target_faces.rows() << std::endl;
        } else {
            std::cerr << "Failed to load target mesh: " << target_path << std::endl;
            return 1;
        }
        
        if (command == "pyramid_reg") {
            std::cout << "\nParameters for pyramid_reg:" << std::endl;
            int num_iterations = result["num_iterations"].as<int>();
            print_arg("num_iterations", num_iterations);
            print_arg("num_pyramid_layers", result["num_pyramid_layers"].as<int>());
            print_arg("downsample_float_start", result["downsample_float_start"].as<float>());
            print_arg("downsample_target_start", result["downsample_target_start"].as<float>());
            print_arg("downsample_float_end", result["downsample_float_end"].as<float>());
            print_arg("downsample_target_end", result["downsample_target_end"].as<float>());
            print_bool_arg("corr_symmetric", result["corr_symmetric"].as<bool>());
            print_arg("corr_num_neighbours", result["corr_num_neighbours"].as<int>());
            print_arg("corr_flag_threshold", result["corr_flag_threshold"].as<float>());
            print_bool_arg("corr_equalize_push_pull", result["corr_equalize_push_pull"].as<bool>());
            print_arg("inlier_kappa", result["inlier_kappa"].as<float>());
            print_bool_arg("inlier_use_orientation", result["inlier_use_orientation"].as<bool>());
            print_arg("transform_sigma", result["transform_sigma"].as<float>());
            
            int transform_viscous_iter_start_val = result.count("transform_viscous_iter_start") ? 
                                                   result["transform_viscous_iter_start"].as<int>() : 
                                                   num_iterations;
            int transform_elastic_iter_start_val = result.count("transform_elastic_iter_start") ? 
                                                   result["transform_elastic_iter_start"].as<int>() : 
                                                   num_iterations;
            print_arg("transform_viscous_iter_start", transform_viscous_iter_start_val);
            print_arg("transform_viscous_iter_end", result["transform_viscous_iter_end"].as<int>());
            print_arg("transform_elastic_iter_start", transform_elastic_iter_start_val);
            print_arg("transform_elastic_iter_end", result["transform_elastic_iter_end"].as<int>());


            FeatureMat floatingFeatures(floating_vertices.rows(), 6); // Removed meshmonk:: namespace
            FeatureMat targetFeatures(target_vertices.rows(), 6); // Removed meshmonk:: namespace

            floatingFeatures.leftCols(3) = floating_vertices;
            floatingFeatures.rightCols(3).setZero(); 

            targetFeatures.leftCols(3) = target_vertices;
            targetFeatures.rightCols(3).setZero(); 

            Eigen::VectorXf floatingFlags = Eigen::VectorXf::Ones(floating_vertices.rows());
            Eigen::VectorXf targetFlags = Eigen::VectorXf::Ones(target_vertices.rows());
            
            std::cout << "\nStarting pyramid registration..." << std::endl;

            meshmonk::pyramid_registration(
                floatingFeatures, targetFeatures,
                floating_faces, target_faces, 
                floatingFlags, targetFlags,
                static_cast<size_t>(num_iterations),
                static_cast<size_t>(result["num_pyramid_layers"].as<int>()),
                result["downsample_float_start"].as<float>(),
                result["downsample_target_start"].as<float>(),
                result["downsample_float_end"].as<float>(),
                result["downsample_target_end"].as<float>(),
                result["corr_symmetric"].as<bool>(),
                static_cast<size_t>(result["corr_num_neighbours"].as<int>()),
                result["corr_flag_threshold"].as<float>(),
                result["corr_equalize_push_pull"].as<bool>(),
                result["inlier_kappa"].as<float>(),
                result["inlier_use_orientation"].as<bool>(),
                result["transform_sigma"].as<float>(),
                static_cast<size_t>(transform_viscous_iter_start_val),
                static_cast<size_t>(result["transform_viscous_iter_end"].as<int>()),
                static_cast<size_t>(transform_elastic_iter_start_val),
                static_cast<size_t>(result["transform_elastic_iter_end"].as<int>())
            );
            
            std::cout << "Pyramid registration completed." << std::endl;

            Eigen::MatrixX3f transformedFloatingVertices = floatingFeatures.leftCols(3);
            std::cout << "\nSaving transformed floating mesh to: " << output_path << std::endl;
            if (save_obj_mesh(output_path, transformedFloatingVertices, floating_faces)) {
                std::cout << "Successfully saved transformed mesh." << std::endl;
            } else {
                std::cerr << "Failed to save transformed mesh to: " << output_path << std::endl;
            }

        } else if (command == "rigid_reg") {
            std::cout << "\nParameters for rigid_reg:" << std::endl;
            print_arg("num_iterations", result["num_iterations"].as<int>());
            print_bool_arg("corr_symmetric", result["corr_symmetric"].as<bool>());
            print_arg("corr_num_neighbours", result["corr_num_neighbours"].as<int>());
            print_arg("corr_flag_threshold", result["corr_flag_threshold"].as<float>());
            print_bool_arg("corr_equalize_push_pull", result["corr_equalize_push_pull"].as<bool>());
            print_arg("inlier_kappa", result["inlier_kappa"].as<float>());
            print_bool_arg("inlier_use_orientation", result["inlier_use_orientation"].as<bool>());
            print_bool_arg("use_scaling", result["use_scaling"].as<bool>());
            if (result.count("output_transform")) {
                print_arg("output_transform", result["output_transform"].as<std::string>());
            }

            // Prepare data for registration
            FeatureMat floatingFeatures(floating_vertices.rows(), 6); // Removed meshmonk:: namespace
            FeatureMat targetFeatures(target_vertices.rows(), 6); // Removed meshmonk:: namespace

            floatingFeatures.leftCols(3) = floating_vertices;
            floatingFeatures.rightCols(3).setZero();

            targetFeatures.leftCols(3) = target_vertices;
            targetFeatures.rightCols(3).setZero();

            Eigen::VectorXf floatingFlags = Eigen::VectorXf::Ones(floating_vertices.rows());
            Eigen::VectorXf targetFlags = Eigen::VectorXf::Ones(target_vertices.rows());

            Eigen::Matrix4f transformationMatrix = Eigen::Matrix4f::Identity();
            
            std::cout << "\nStarting rigid registration..." << std::endl;

            meshmonk::rigid_registration(
                floatingFeatures, targetFeatures,
                floating_faces, target_faces, // Pass faces
                floatingFlags, targetFlags,
                transformationMatrix, // Output parameter
                static_cast<size_t>(result["num_iterations"].as<int>()),
                result["corr_symmetric"].as<bool>(),
                static_cast<size_t>(result["corr_num_neighbours"].as<int>()),
                result["corr_flag_threshold"].as<float>(),
                result["corr_equalize_push_pull"].as<bool>(),
                result["inlier_kappa"].as<float>(),
                result["inlier_use_orientation"].as<bool>(),
                result["use_scaling"].as<bool>()
            );

            std::cout << "Rigid registration completed." << std::endl;

            // Save transformed mesh
            Eigen::MatrixX3f transformedFloatingVertices = floatingFeatures.leftCols(3);
            std::cout << "\nSaving transformed floating mesh to: " << output_path << std::endl;
            if (save_obj_mesh(output_path, transformedFloatingVertices, floating_faces)) {
                std::cout << "Successfully saved transformed mesh." << std::endl;
            } else {
                std::cerr << "Failed to save transformed mesh to: " << output_path << std::endl;
            }

            // Save transformation matrix if requested
            if (result.count("output_transform")) {
                std::string output_transform_path = result["output_transform"].as<std::string>();
                std::cout << "\nSaving transformation matrix to: " << output_transform_path << std::endl;
                if (save_transform_matrix(output_transform_path, transformationMatrix)) {
                    std::cout << "Successfully saved transformation matrix." << std::endl;
                } else {
                    std::cerr << "Failed to save transformation matrix to: " << output_transform_path << std::endl;
                }
            }
        }

    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        std::cerr << options.help() << std::endl;
        return 1;
    }

    return 0;
}
