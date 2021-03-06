set(CMAKE_CXX_STANDARD 17)

vm_external_module(
  GIT_REPOSITORY https://github.com/cad420/cuda-fx
  GIT_TAG        master
)

file(GLOB VARCH_ARCHIVE_SRC *.cc)

add_subdirectory(backends)
get_directory_property(BE_SRC DIRECTORY backends DEFINITION SOURCES)

set(VARCH_ARCHIVE_SRC ${VARCH_ARCHIVE_SRC} ${BE_SRC})

cuda_add_library(voxel_archive ${VARCH_ARCHIVE_SRC})

target_link_libraries(voxel_archive ${CUDA_CUDA_LIBRARY})
if (UNIX)
target_link_libraries(voxel_archive pthread dl)
endif()
# target_include_directories(varch 
#   ${CUDA_INCLUDE_DIRS}
# )
vm_target_dependency(voxel_archive cudafx PUBLIC)
vm_target_dependency(voxel_archive VMUtils PUBLIC)
vm_target_dependency(voxel_archive vmcore PRIVATE)
if (UNIX)
  target_link_libraries(voxel_archive pthread dl)
endif()
target_link_libraries(voxel_archive vmcore)

if(MSVC)
  if(CMAKE_CL_64)
    target_link_libraries(voxel_archive
      ${PROJECT_SOURCE_DIR}/lib/x64/nvcuvid.lib
    )
  else()
    target_link_libraries(voxel_archive
      ${PROJECT_SOURCE_DIR}/lib/Win32/nvcuvid.lib
    )
  endif()
else()
  target_link_libraries(voxel_archive
  	${PROJECT_SOURCE_DIR}/lib/linux/stubs/x86_64/libnvidia-encode.so
  )
endif()
if (OPENH264_FOUND)
  target_link_libraries(voxel_archive openh264)
endif()

target_include_directories(voxel_archive PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
