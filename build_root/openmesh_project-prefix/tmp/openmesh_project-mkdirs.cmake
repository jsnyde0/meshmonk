# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/tmp/OpenMesh-src"
  "/app/build_root/openmesh_project-prefix/src/openmesh_project-build"
  "/app/build_root/openmesh_project-prefix"
  "/app/build_root/openmesh_project-prefix/tmp"
  "/app/build_root/openmesh_project-prefix/src/openmesh_project-stamp"
  "/app/build_root/openmesh_project-prefix/src"
  "/app/build_root/openmesh_project-prefix/src/openmesh_project-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/app/build_root/openmesh_project-prefix/src/openmesh_project-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/app/build_root/openmesh_project-prefix/src/openmesh_project-stamp${cfgdir}") # cfgdir has leading slash
endif()
