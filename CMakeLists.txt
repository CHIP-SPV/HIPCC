cmake_minimum_required(VERSION 3.16.3)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project (hipcc.bin)
add_executable(hipcc.bin src/hipBin.cpp)

project (hipconfig.bin)
add_executable(hipconfig.bin src/hipBin.cpp)

# If not  building as a standalone, put the binary in /bin
if(DEFINED HIPCC_BUILD_PATH)
  message(STATUS "HIPCC: HIPCC_BUILD_PATH was provided. hipcc.bin and hipconfig.bin will be placed in ${HIPCC_BUILD_PATH}")
  set_target_properties(hipcc.bin hipconfig.bin PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${HIPCC_BUILD_PATH})
endif()

set(HIP_VERSION_MAJOR 4 PARENT_SCOPE)
set(HIP_VERSION_MINOR 4 PARENT_SCOPE)
set(HIP_VERSION_PATCH 4 PARENT_SCOPE)
