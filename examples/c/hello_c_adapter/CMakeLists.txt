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
cmake_minimum_required(VERSION 3.11)

project(hello_c_adapter C)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wimplicit-fallthrough)
endif()

if(NOT TARGET rticonnextdds_ros2_adapter_c)
  find_package(rticonnextdds_ros2_adapter_c CONFIG REQUIRED)
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

find_program(rtiddsgen_bin
  NAMES rtiddsgen
  PATHS "${CONNEXTDDS_DIR}/bin"
)

set(generated_code_dir  ${CMAKE_CURRENT_BINARY_DIR}/rtiddsgen)

set(generated_code_c_headers
  ${generated_code_dir}/String.h
  ${generated_code_dir}/StringSupport.h
  ${generated_code_dir}/StringPlugin.h)

set(generated_code_c_sources
  ${generated_code_dir}/String.c
  ${generated_code_dir}/StringSupport.c
  ${generated_code_dir}/StringPlugin.c)

set(generated_code_c ${generated_code_c_headers} ${generated_code_c_sources})

set(idl_file ${CMAKE_CURRENT_SOURCE_DIR}/String.idl)

add_custom_command(OUTPUT ${generated_code_c}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_code_dir}
  COMMAND ${rtiddsgen_bin} -language C
                           -replace
                           -d ${generated_code_dir}
                           -unboundedSupport
                           ${idl_file}
  DEPENDS ${idl_file})


add_executable(${PROJECT_NAME}
  hello_c_adapter.c
  ${generated_code_c}
)
target_link_libraries(${PROJECT_NAME} rticonnextdds_ros2_adapter_c)
target_include_directories(${PROJECT_NAME}
  PRIVATE
    "${generated_code_dir}"
)

install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
