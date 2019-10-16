set(CMAKE_CXX_STANDARD 17)

# set(VMCORE_SHARED_LIBRARY OFF)
# vm_external_module(
#   GIT_REPOSITORY https://github.com/cad420/VMCore
#   GIT_TAG        e5bd96e
# )
set(VMCORE_SHARED_LIBRARY OFF)
vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/VMat
  GIT_TAG        master
)
vm_external_module(
  GIT_REPOSITORY https://github.com/xlnx/koi
  GIT_TAG        external/cad420
)
# add_definitions(-DKOI_NO_LIBUV_REACTOR)

add_library(voxel_refine refiner.cc)
if (UNIX)
  target_link_libraries(voxel_refine pthread dl)
endif()
target_include_directories(voxel_refine PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(voxel_refine uv)
vm_target_dependency(voxel_refine VMUtils PUBLIC)
# vm_target_dependency(voxel_refine vmcore PUBLIC)
vm_target_dependency(voxel_refine koi PUBLIC)
vm_target_dependency(voxel_refine VMat PUBLIC)
# target_link_libraries(voxel_refine vmcore)
