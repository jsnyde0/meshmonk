#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>
#include "meshmonk/meshmonk.hpp"
#include "meshmonk/global.hpp" // For registration::NUM_FEATURES

namespace py = pybind11;

// Wrapper function for meshmonk::rigid_registration using data copying
void rigid_registration_wrapper(
    py::array_t<float, py::array::c_style | py::array::forcecast> floating_features_arr,
    py::array_t<float, py::array::c_style | py::array::forcecast> target_features_arr,
    py::object floating_faces_arr_obj, // py::array_t or py::none
    py::object target_faces_arr_obj,   // py::array_t or py::none
    py::object floating_flags_arr_obj, // py::array_t or py::none
    py::object target_flags_arr_obj,   // py::array_t or py::none
    py::array_t<float, py::array::c_style | py::array::forcecast> transformation_matrix_arr,
    size_t num_iterations = 20,
    bool correspondences_symmetric = true, size_t correspondences_num_neighbours = 5,
    float correspondences_flag_threshold = 0.99f, bool correspondences_equalize_push_pull = false,
    float inlier_kappa = 4.0f, bool inlier_use_orientation = true,
    bool use_scaling = false) {

    py::buffer_info ff_buf = floating_features_arr.request(true); 
    if (ff_buf.ptr == nullptr) throw std::runtime_error("Floating features buffer is null");
    if (ff_buf.ndim != 2 || ff_buf.shape[1] != registration::NUM_FEATURES) {
        throw std::runtime_error("Floating features NumPy array must be of shape (N, NUM_FEATURES)");
    }
    FeatureMat local_floating_features(ff_buf.shape[0], ff_buf.shape[1]);
    memcpy(local_floating_features.data(), ff_buf.ptr, ff_buf.size * sizeof(float));

    py::buffer_info tm_buf = transformation_matrix_arr.request(true); 
    if (tm_buf.ptr == nullptr) throw std::runtime_error("Transformation matrix buffer is null");
    if (tm_buf.ndim != 2 || tm_buf.shape[0] != 4 || tm_buf.shape[1] != 4) {
        throw std::runtime_error("Transformation matrix NumPy array must be of shape (4, 4)");
    }
    Mat4Float local_transformation_matrix; 
    memcpy(local_transformation_matrix.data(), tm_buf.ptr, tm_buf.size * sizeof(float));

    py::buffer_info target_ff_buf = target_features_arr.request(); 
    if (target_ff_buf.ptr == nullptr) throw std::runtime_error("Target features buffer is null");
    if (target_ff_buf.ndim != 2 || target_ff_buf.shape[1] != registration::NUM_FEATURES) {
        throw std::runtime_error("Target features NumPy array must be 2D and have NUM_FEATURES columns.");
    }
    Eigen::Map<const FeatureMat> target_features_map(static_cast<const float*>(target_ff_buf.ptr), target_ff_buf.shape[0], target_ff_buf.shape[1]);

    FacesMat floating_faces_map_data(0, 3);
    FacesMat target_faces_map_data(0, 3);
    VecDynFloat floating_flags_map_data(0);
    VecDynFloat target_flags_map_data(0);

    if (!floating_faces_arr_obj.is_none()) {
        auto floating_faces_arr = floating_faces_arr_obj.cast<py::array_t<int, py::array::c_style | py::array::forcecast>>();
        py::buffer_info f_faces_buf = floating_faces_arr.request();
        if (f_faces_buf.ptr != nullptr && f_faces_buf.size > 0) {
            if (f_faces_buf.ndim != 2 || f_faces_buf.shape[1] != 3) throw std::runtime_error("Floating faces NumPy array must be 2D and have 3 columns.");
            floating_faces_map_data = Eigen::Map<const FacesMat>(static_cast<const int*>(f_faces_buf.ptr), f_faces_buf.shape[0], f_faces_buf.shape[1]);
        }
    }

    if (!target_faces_arr_obj.is_none()) {
        auto target_faces_arr = target_faces_arr_obj.cast<py::array_t<int, py::array::c_style | py::array::forcecast>>();
        py::buffer_info t_faces_buf = target_faces_arr.request();
        if (t_faces_buf.ptr != nullptr && t_faces_buf.size > 0) {
            if (t_faces_buf.ndim != 2 || t_faces_buf.shape[1] != 3) throw std::runtime_error("Target faces NumPy array must be 2D and have 3 columns.");
            target_faces_map_data = Eigen::Map<const FacesMat>(static_cast<const int*>(t_faces_buf.ptr), t_faces_buf.shape[0], t_faces_buf.shape[1]);
        }
    }

    if (!floating_flags_arr_obj.is_none()) {
        auto floating_flags_arr = floating_flags_arr_obj.cast<py::array_t<float, py::array::c_style | py::array::forcecast>>();
        py::buffer_info f_flags_buf = floating_flags_arr.request();
        if (f_flags_buf.ptr != nullptr && f_flags_buf.size > 0) {
            if (f_flags_buf.ndim != 1 && (f_flags_buf.ndim == 2 && (f_flags_buf.shape[0] != 1 && f_flags_buf.shape[1] != 1))) throw std::runtime_error("Floating flags NumPy array must be 1-dimensional.");
            floating_flags_map_data = Eigen::Map<const VecDynFloat>(static_cast<const float*>(f_flags_buf.ptr), f_flags_buf.size);
        }
    }

    if (!target_flags_arr_obj.is_none()) {
        auto target_flags_arr = target_flags_arr_obj.cast<py::array_t<float, py::array::c_style | py::array::forcecast>>();
        py::buffer_info t_flags_buf = target_flags_arr.request();
        if (t_flags_buf.ptr != nullptr && t_flags_buf.size > 0) {
            if (t_flags_buf.ndim != 1 && (t_flags_buf.ndim == 2 && (t_flags_buf.shape[0] != 1 && t_flags_buf.shape[1] != 1))) throw std::runtime_error("Target flags NumPy array must be 1-dimensional.");
            target_flags_map_data = Eigen::Map<const VecDynFloat>(static_cast<const float*>(t_flags_buf.ptr), t_flags_buf.size);
        }
    }

    meshmonk::rigid_registration(
        local_floating_features, target_features_map,
        floating_faces_map_data, target_faces_map_data,
        floating_flags_map_data, target_flags_map_data,
        local_transformation_matrix, 
        num_iterations, correspondences_symmetric, correspondences_num_neighbours,
        correspondences_flag_threshold, correspondences_equalize_push_pull,
        inlier_kappa, inlier_use_orientation, use_scaling);

    // Copy back the modified floating features and transformation matrix
    memcpy(ff_buf.ptr, local_floating_features.data(), ff_buf.size * sizeof(float));
    memcpy(tm_buf.ptr, local_transformation_matrix.data(), tm_buf.size * sizeof(float));
}

void nonrigid_registration_wrapper(
    py::array_t<float, py::array::c_style | py::array::forcecast> floating_features_arr,
    py::array_t<float, py::array::c_style | py::array::forcecast> target_features_arr,
    py::object floating_faces_arr_obj, // py::array_t or py::none
    py::object target_faces_arr_obj,   // py::array_t or py::none
    py::object floating_flags_arr_obj, // py::array_t or py::none
    py::object target_flags_arr_obj,   // py::array_t or py::none
    size_t num_iterations = 60,
    bool correspondences_symmetric = true, size_t correspondences_num_neighbours = 5,
    float correspondences_flag_threshold = 0.99f, bool correspondences_equalize_push_pull = false,
    float inlier_kappa = 4.0f, bool inlier_use_orientation = true,
    float transform_sigma = 3.0f,
    size_t transform_num_viscous_iterations_start = 50, size_t transform_num_viscous_iterations_end = 1,
    size_t transform_num_elastic_iterations_start = 50, size_t transform_num_elastic_iterations_end = 1) {

    py::buffer_info ff_buf = floating_features_arr.request(true);
    if (ff_buf.ptr == nullptr) throw std::runtime_error("Floating features buffer is null");
    if (ff_buf.ndim != 2 || ff_buf.shape[1] != registration::NUM_FEATURES) {
        throw std::runtime_error("Floating features NumPy array must be of shape (N, NUM_FEATURES)");
    }
    FeatureMat local_floating_features(ff_buf.shape[0], ff_buf.shape[1]);
    memcpy(local_floating_features.data(), ff_buf.ptr, ff_buf.size * sizeof(float));

    py::buffer_info target_ff_buf = target_features_arr.request();
    if (target_ff_buf.ptr == nullptr) throw std::runtime_error("Target features buffer is null");
    if (target_ff_buf.ndim != 2 || target_ff_buf.shape[1] != registration::NUM_FEATURES) {
        throw std::runtime_error("Target features NumPy array must be 2D and have NUM_FEATURES columns.");
    }
    Eigen::Map<const FeatureMat> target_features_map(static_cast<const float*>(target_ff_buf.ptr), target_ff_buf.shape[0], target_ff_buf.shape[1]);

    FacesMat floating_faces_map_data(0, 3);
    FacesMat target_faces_map_data(0, 3);
    VecDynFloat floating_flags_map_data(0);
    VecDynFloat target_flags_map_data(0);

    if (!floating_faces_arr_obj.is_none()) {
        auto floating_faces_arr = floating_faces_arr_obj.cast<py::array_t<int, py::array::c_style | py::array::forcecast>>();
        py::buffer_info f_faces_buf = floating_faces_arr.request();
        if (f_faces_buf.ptr != nullptr && f_faces_buf.size > 0) {
            if (f_faces_buf.ndim != 2 || f_faces_buf.shape[1] != 3) throw std::runtime_error("Floating faces NumPy array must be 2D and have 3 columns.");
            floating_faces_map_data = Eigen::Map<const FacesMat>(static_cast<const int*>(f_faces_buf.ptr), f_faces_buf.shape[0], f_faces_buf.shape[1]);
        }
    }

    if (!target_faces_arr_obj.is_none()) {
        auto target_faces_arr = target_faces_arr_obj.cast<py::array_t<int, py::array::c_style | py::array::forcecast>>();
        py::buffer_info t_faces_buf = target_faces_arr.request();
        if (t_faces_buf.ptr != nullptr && t_faces_buf.size > 0) {
            if (t_faces_buf.ndim != 2 || t_faces_buf.shape[1] != 3) throw std::runtime_error("Target faces NumPy array must be 2D and have 3 columns.");
            target_faces_map_data = Eigen::Map<const FacesMat>(static_cast<const int*>(t_faces_buf.ptr), t_faces_buf.shape[0], t_faces_buf.shape[1]);
        }
    }

    if (!floating_flags_arr_obj.is_none()) {
        auto floating_flags_arr = floating_flags_arr_obj.cast<py::array_t<float, py::array::c_style | py::array::forcecast>>();
        py::buffer_info f_flags_buf = floating_flags_arr.request();
        if (f_flags_buf.ptr != nullptr && f_flags_buf.size > 0) {
            if (f_flags_buf.ndim != 1 && (f_flags_buf.ndim == 2 && (f_flags_buf.shape[0] != 1 && f_flags_buf.shape[1] != 1))) throw std::runtime_error("Floating flags NumPy array must be 1-dimensional.");
            floating_flags_map_data = Eigen::Map<const VecDynFloat>(static_cast<const float*>(f_flags_buf.ptr), f_flags_buf.size);
        }
    }

    if (!target_flags_arr_obj.is_none()) {
        auto target_flags_arr = target_flags_arr_obj.cast<py::array_t<float, py::array::c_style | py::array::forcecast>>();
        py::buffer_info t_flags_buf = target_flags_arr.request();
        if (t_flags_buf.ptr != nullptr && t_flags_buf.size > 0) {
            if (t_flags_buf.ndim != 1 && (t_flags_buf.ndim == 2 && (t_flags_buf.shape[0] != 1 && t_flags_buf.shape[1] != 1))) throw std::runtime_error("Target flags NumPy array must be 1-dimensional.");
            target_flags_map_data = Eigen::Map<const VecDynFloat>(static_cast<const float*>(t_flags_buf.ptr), t_flags_buf.size);
        }
    }
    
    meshmonk::nonrigid_registration(
        local_floating_features, target_features_map,
        floating_faces_map_data, target_faces_map_data,
        floating_flags_map_data, target_flags_map_data,
        num_iterations, correspondences_symmetric, correspondences_num_neighbours,
        correspondences_flag_threshold, correspondences_equalize_push_pull,
        inlier_kappa, inlier_use_orientation, transform_sigma,
        transform_num_viscous_iterations_start, transform_num_viscous_iterations_end,
        transform_num_elastic_iterations_start, transform_num_elastic_iterations_end);

    memcpy(ff_buf.ptr, local_floating_features.data(), ff_buf.size * sizeof(float));
}

void pyramid_registration_wrapper(
    py::array_t<float, py::array::c_style | py::array::forcecast> floating_features_arr, // Mandatory
    py::array_t<float, py::array::c_style | py::array::forcecast> target_features_arr,   // Mandatory
    py::array_t<int, py::array::c_style | py::array::forcecast> floating_faces_arr,     // Mandatory for pyramid
    py::array_t<int, py::array::c_style | py::array::forcecast> target_faces_arr,       // Mandatory for pyramid
    py::object floating_flags_arr_obj, // py::array_t or py::none
    py::object target_flags_arr_obj,   // py::array_t or py::none
    size_t num_iterations = 60, size_t num_pyramid_layers = 3,
    float downsample_float_start = 90.0f, float downsample_target_start = 90.0f,
    float downsample_float_end = 0.0f, float downsample_target_end = 0.0f,
    bool correspondences_symmetric = true, size_t correspondences_num_neighbours = 5,
    float correspondences_flag_threshold = 0.99f, bool correspondences_equalize_push_pull = false,
    float inlier_kappa = 4.0f, bool inlier_use_orientation = true,
    float transform_sigma = 3.0f,
    size_t transform_num_viscous_iterations_start = 50, size_t transform_num_viscous_iterations_end = 1,
    size_t transform_num_elastic_iterations_start = 50, size_t transform_num_elastic_iterations_end = 1) {

    py::buffer_info ff_buf = floating_features_arr.request(true);
    if (ff_buf.ptr == nullptr) throw std::runtime_error("Floating features buffer is null");
    if (ff_buf.ndim != 2 || ff_buf.shape[1] != registration::NUM_FEATURES) {
        throw std::runtime_error("Floating features NumPy array must be of shape (N, NUM_FEATURES)");
    }
    FeatureMat local_floating_features(ff_buf.shape[0], ff_buf.shape[1]);
    memcpy(local_floating_features.data(), ff_buf.ptr, ff_buf.size * sizeof(float));

    py::buffer_info target_ff_buf = target_features_arr.request();
    if (target_ff_buf.ptr == nullptr) throw std::runtime_error("Target features buffer is null");
    if (target_ff_buf.ndim != 2 || target_ff_buf.shape[1] != registration::NUM_FEATURES) {
        throw std::runtime_error("Target features NumPy array must be 2D and have NUM_FEATURES columns.");
    }
    Eigen::Map<const FeatureMat> target_features_map(static_cast<const float*>(target_ff_buf.ptr), target_ff_buf.shape[0], target_ff_buf.shape[1]);
    
    // For pyramid, faces are mandatory from Python, so we expect valid arrays.
    py::buffer_info f_faces_buf = floating_faces_arr.request();
    if (f_faces_buf.ptr == nullptr || f_faces_buf.size == 0) throw std::runtime_error("Floating faces buffer is null or empty for pyramid registration.");
    if (f_faces_buf.ndim != 2 || f_faces_buf.shape[1] != 3) {
        throw std::runtime_error("Floating faces NumPy array must be 2D and have 3 columns for pyramid.");
    }
    Eigen::Map<const FacesMat> floating_faces_map(static_cast<const int*>(f_faces_buf.ptr), f_faces_buf.shape[0], f_faces_buf.shape[1]);

    py::buffer_info t_faces_buf = target_faces_arr.request();
    if (t_faces_buf.ptr == nullptr || t_faces_buf.size == 0) throw std::runtime_error("Target faces buffer is null or empty for pyramid registration.");
    if (t_faces_buf.ndim != 2 || t_faces_buf.shape[1] != 3) {
        throw std::runtime_error("Target faces NumPy array must be 2D and have 3 columns for pyramid.");
    }
    Eigen::Map<const FacesMat> target_faces_map(static_cast<const int*>(t_faces_buf.ptr), t_faces_buf.shape[0], t_faces_buf.shape[1]);

    VecDynFloat floating_flags_map_data(0);
    VecDynFloat target_flags_map_data(0);

    if (!floating_flags_arr_obj.is_none()) {
        auto floating_flags_arr = floating_flags_arr_obj.cast<py::array_t<float, py::array::c_style | py::array::forcecast>>();
        py::buffer_info f_flags_buf = floating_flags_arr.request();
        if (f_flags_buf.ptr != nullptr && f_flags_buf.size > 0) {
            if (f_flags_buf.ndim != 1 && (f_flags_buf.ndim == 2 && (f_flags_buf.shape[0] != 1 && f_flags_buf.shape[1] != 1))) throw std::runtime_error("Floating flags NumPy array must be 1-dimensional.");
            floating_flags_map_data = Eigen::Map<const VecDynFloat>(static_cast<const float*>(f_flags_buf.ptr), f_flags_buf.size);
        }
    }

    if (!target_flags_arr_obj.is_none()) {
        auto target_flags_arr = target_flags_arr_obj.cast<py::array_t<float, py::array::c_style | py::array::forcecast>>();
        py::buffer_info t_flags_buf = target_flags_arr.request();
        if (t_flags_buf.ptr != nullptr && t_flags_buf.size > 0) {
             if (t_flags_buf.ndim != 1 && (t_flags_buf.ndim == 2 && (t_flags_buf.shape[0] != 1 && t_flags_buf.shape[1] != 1))) throw std::runtime_error("Target flags NumPy array must be 1-dimensional.");
            target_flags_map_data = Eigen::Map<const VecDynFloat>(static_cast<const float*>(t_flags_buf.ptr), t_flags_buf.size);
        }
    }
    
    meshmonk::pyramid_registration(
        local_floating_features, target_features_map,
        floating_faces_map, target_faces_map, // These are correct as direct maps for pyramid
        floating_flags_map_data, target_flags_map_data, // These use _data
        num_iterations, num_pyramid_layers,
        downsample_float_start, downsample_target_start,
        downsample_float_end, downsample_target_end,
        correspondences_symmetric, correspondences_num_neighbours,
        correspondences_flag_threshold, correspondences_equalize_push_pull,
        inlier_kappa, inlier_use_orientation, transform_sigma,
        transform_num_viscous_iterations_start, transform_num_viscous_iterations_end,
        transform_num_elastic_iterations_start, transform_num_elastic_iterations_end);

    memcpy(ff_buf.ptr, local_floating_features.data(), ff_buf.size * sizeof(float));
}


PYBIND11_MODULE(meshmonk_python, m) {
    m.doc() = "Python bindings for MeshMonk";

    m.attr("NUM_FEATURES") = py::int_(registration::NUM_FEATURES);

    m.def("rigid_registration", &rigid_registration_wrapper,
        py::arg("floating_features"), 
        py::arg("target_features"),
        py::arg("floating_faces") = py::none(), 
        py::arg("target_faces") = py::none(),
        py::arg("floating_flags") = py::none(), 
        py::arg("target_flags") = py::none(),
        py::arg("transformation_matrix"),
        py::arg("num_iterations") = 20,
        py::arg("correspondences_symmetric") = true, py::arg("correspondences_num_neighbours") = 5,
        py::arg("correspondences_flag_threshold") = 0.99f, py::arg("correspondences_equalize_push_pull") = false,
        py::arg("inlier_kappa") = 4.0f, py::arg("inlier_use_orientation") = true,
        py::arg("use_scaling") = false,
        "Wrapper for MeshMonk rigid_registration that operates on NumPy arrays in-place via copying.");

    m.def("nonrigid_registration", &nonrigid_registration_wrapper,
        py::arg("floating_features"), 
        py::arg("target_features"),
        py::arg("floating_faces") = py::none(), 
        py::arg("target_faces") = py::none(),
        py::arg("floating_flags") = py::none(), 
        py::arg("target_flags") = py::none(),
        py::arg("num_iterations") = 60,
        py::arg("correspondences_symmetric") = true, py::arg("correspondences_num_neighbours") = 5,
        py::arg("correspondences_flag_threshold") = 0.99f, py::arg("correspondences_equalize_push_pull") = false,
        py::arg("inlier_kappa") = 4.0f, py::arg("inlier_use_orientation") = true,
        py::arg("transform_sigma") = 3.0f,
        py::arg("transform_num_viscous_iterations_start") = 50, py::arg("transform_num_viscous_iterations_end") = 1,
        py::arg("transform_num_elastic_iterations_start") = 50, py::arg("transform_num_elastic_iterations_end") = 1,
        "Wrapper for MeshMonk nonrigid_registration that operates on NumPy arrays in-place via copying.");

    m.def("pyramid_registration", &pyramid_registration_wrapper,
        py::arg("floating_features"), // Mandatory
        py::arg("target_features"),   // Mandatory
        py::arg("floating_faces"),    // Mandatory
        py::arg("target_faces"),      // Mandatory
        py::arg("floating_flags") = py::none(), 
        py::arg("target_flags") = py::none(),
        py::arg("num_iterations") = 60, py::arg("num_pyramid_layers") = 3,
        py::arg("downsample_float_start") = 90.0f, py::arg("downsample_target_start") = 90.0f,
        py::arg("downsample_float_end") = 0.0f, py::arg("downsample_target_end") = 0.0f,
        py::arg("correspondences_symmetric") = true, py::arg("correspondences_num_neighbours") = 5,
        py::arg("correspondences_flag_threshold") = 0.99f, py::arg("correspondences_equalize_push_pull") = false,
        py::arg("inlier_kappa") = 4.0f, py::arg("inlier_use_orientation") = true,
        py::arg("transform_sigma") = 3.0f,
        py::arg("transform_num_viscous_iterations_start") = 50, py::arg("transform_num_viscous_iterations_end") = 1,
        py::arg("transform_num_elastic_iterations_start") = 50, py::arg("transform_num_elastic_iterations_end") = 1,
        "Wrapper for MeshMonk pyramid_registration that operates on NumPy arrays in-place via copying.");
    

    py::register_exception_translator([](std::exception_ptr p) {
        try {
            if (p) std::rethrow_exception(p);
        } catch (const std::exception &e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
        }
    });
}
