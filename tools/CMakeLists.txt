# add_executable(voprod voprod.cc)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
)

vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/cppfs
  GIT_TAG        master
)

if(VARCH_BUILD_ARCHIVER)
  cuda_add_executable(archiver archiver.cc)
  vm_target_dependency(archiver voxel_archive PRIVATE)
  vm_target_dependency(archiver cppfs PRIVATE)
endif()

if(VARCH_BUILD_UNARCHIVER)
  cuda_add_executable(archive-info archive-info.cc)
  vm_target_dependency(archive-info voxel_unarchive PRIVATE)
  vm_target_dependency(archive-info cppfs PRIVATE)
  
  cuda_add_executable(archive-thumbnailer archive-thumbnailer.cc)
  vm_target_dependency(archive-thumbnailer voxel_unarchive PRIVATE)
  vm_target_dependency(archive-thumbnailer cppfs PRIVATE)

  cuda_add_executable(meta-builder meta-builder.cc)
  vm_target_dependency(meta-builder voxel_unarchive PRIVATE)
  vm_target_dependency(meta-builder cppfs PRIVATE)
endif()

add_executable(downsampler downsampler.cc)
vm_target_dependency(downsampler vmcore PRIVATE)
vm_target_dependency(downsampler VMat PRIVATE)
vm_target_dependency(downsampler VMUtils PRIVATE)
vm_target_dependency(downsampler cppfs PRIVATE)
