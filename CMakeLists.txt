# Copyright 2020 Real-Time Innovations, Inc.
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

project(rticonnextdds_ros2_adapter C)

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
  COMPONENTS     core )

find_program(rtiddsgen_bin
  NAMES rtiddsgen
  PATHS "${CONNEXTDDS_DIR}/bin"
)
if("${rtiddsgen_bin}" MATCHES "NOTFOUND")
  message(FATAL_ERROR "failed to find rtiddsgen command")
endif()

set(generated_code_dir  ${CMAKE_CURRENT_BINARY_DIR}/rtiddsgen)

set(generated_code_sources
  ${generated_code_dir}/rmw_dds_common/msg/Graph.h
  ${generated_code_dir}/rmw_dds_common/msg/Graph.c
  ${generated_code_dir}/rmw_dds_common/msg/GraphSupport.h
  ${generated_code_dir}/rmw_dds_common/msg/GraphSupport.c
  ${generated_code_dir}/rmw_dds_common/msg/GraphPlugin.h
  ${generated_code_dir}/rmw_dds_common/msg/GraphPlugin.c)

set(graph_idl ${CMAKE_CURRENT_SOURCE_DIR}/idl/rmw_dds_common/msg/Graph.idl)

add_custom_command(OUTPUT ${generated_code_sources}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_code_dir}/rmw_dds_common/msg
  COMMAND ${rtiddsgen_bin} -language C
                           -replace
                           -d ${generated_code_dir}/rmw_dds_common/msg
                           -unboundedSupport
                           ${graph_idl}
  DEPENDS ${graph_idl})

add_library(${PROJECT_NAME} SHARED
  src/graph_impl.h
  src/graph_impl.c
  src/graph.c
  include/rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_graph.h
  ${generated_code_sources})

target_compile_definitions(${PROJECT_NAME}
  PRIVATE "RTICONNEXTDDS_ROS2_ADAPTER_BUILDING_LIBRARY")

target_include_directories(${PROJECT_NAME} PRIVATE
  ${generated_code_dir})

target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")

target_link_libraries(${PROJECT_NAME} PUBLIC RTIConnextDDS::c_api)

install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
