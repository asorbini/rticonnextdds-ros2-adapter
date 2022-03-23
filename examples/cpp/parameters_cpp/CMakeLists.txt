# Copyright 2022 Real-Time Innovations, Inc. (RTI)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required(VERSION 3.5)

project(parameters_cpp CXX)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wimplicit-fallthrough)
endif()

if(NOT TARGET rticonnextdds_ros2_adapter_cpp)
  find_package(rticonnextdds_ros2_adapter_cpp CONFIG REQUIRED)
endif()

if(NOT DEFINED CONNEXTDDS_DIR)
  if(NOT "$ENV{CONNEXTDDS_DIR}" STREQUAL "")
      file(TO_CMAKE_PATH "$ENV{CONNEXTDDS_DIR}" connextdds_dir)
  elseif(DEFINED NDDSHOME)
      set(connextdds_dir          "${NDDSHOME}")
  elseif(NOT "$ENV{NDDSHOME}" STREQUAL "")
      file(TO_CMAKE_PATH "$ENV{NDDSHOME}" connextdds_dir)
  endif()
  if("${connextdds_dir}" STREQUAL "")
      message(WARNING "no CONNEXTDDS_DIR nor NDDSHOME specified")
  endif()
  set(CONNEXTDDS_DIR "${connextdds_dir}")
endif()


list(APPEND CMAKE_MODULE_PATH
  "${CONNEXTDDS_DIR}/resource/cmake")

set(BUILD_SHARED_LIBS ON)

find_package(RTIConnextDDS "5.3.1" REQUIRED
  COMPONENTS     core messaging_api)

find_program(rtiddsgen_bin
  NAMES rtiddsgen
  PATHS "${CONNEXTDDS_DIR}/bin"
)

set(generated_code_dir  ${CMAKE_CURRENT_BINARY_DIR}/rtiddsgen)

set(generated_code_headers
  ${generated_code_dir}/Parameters.hpp
  ${generated_code_dir}/ParametersPlugin.hpp)

set(generated_code_sources
  ${generated_code_dir}/Parameters.cxx
  ${generated_code_dir}/ParametersPlugin.cxx)

set(generated_code ${generated_code_headers} ${generated_code_sources})

set(idl_file ${CMAKE_CURRENT_SOURCE_DIR}/Parameters.idl)

add_custom_command(OUTPUT ${generated_code}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_code_dir}
  COMMAND ${rtiddsgen_bin} -language C++11
                           -replace
                           -d ${generated_code_dir}
                           -unboundedSupport
                           ${idl_file}
  DEPENDS ${idl_file})


add_executable(parameters_client_cpp
  parameters_client_cpp.cpp
  ${generated_code}
)
target_link_libraries(parameters_client_cpp
  rticonnextdds_ros2_adapter_cpp
  RTIConnextDDS::messaging_cpp2_api
)
target_include_directories(parameters_client_cpp
  PRIVATE
    "${generated_code_dir}"
)

install(TARGETS parameters_client_cpp
  EXPORT export_parameters_client_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

add_executable(parameters_service_cpp
  parameters_service_cpp.cpp
  ${generated_code}
)
target_link_libraries(parameters_service_cpp
  rticonnextdds_ros2_adapter_cpp
  RTIConnextDDS::messaging_cpp2_api
)
target_include_directories(parameters_service_cpp
  PRIVATE
    "${generated_code_dir}"
)

install(TARGETS parameters_service_cpp
  EXPORT export_parameters_service_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
