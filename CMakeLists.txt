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

project(rticonnextdds_ros2_adapter
  VERSION 0.1.0
  LANGUAGES C
)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wimplicit-fallthrough)
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
  COMPONENTS     core )

set(CONNEXTDDS_DIR "${CONNEXTDDS_DIR}"
  CACHE PATH "Installation directory for Connext DDS" FORCE)
set(CONNEXTDDS_ARCH "${CONNEXTDDS_ARCH}"
  CACHE STRING "Build architecture for Connext DDS" FORCE)

find_program(rtiddsgen_bin
  NAMES rtiddsgen
  PATHS "${CONNEXTDDS_DIR}/bin"
)
if("${rtiddsgen_bin}" MATCHES "NOTFOUND")
  message(FATAL_ERROR "failed to find rtiddsgen command")
endif()

set(generated_code_dir  ${CMAKE_CURRENT_BINARY_DIR}/rtiddsgen)

set(generated_code_headers
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_model.h
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_modelSupport.h
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_modelPlugin.h)

set(generated_code_sources
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_model.c
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_modelSupport.c
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_modelPlugin.c)

set(generated_code ${generated_code_headers} ${generated_code_sources})

set(graph_idl ${CMAKE_CURRENT_SOURCE_DIR}/idl/${PROJECT_NAME}/${PROJECT_NAME}_model.idl)

add_custom_command(OUTPUT ${generated_code}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_code_dir}/${PROJECT_NAME}
  COMMAND ${rtiddsgen_bin} -language C
                           -replace
                           -d ${generated_code_dir}/${PROJECT_NAME}
                           -unboundedSupport
                           ${graph_idl}
  DEPENDS ${graph_idl})

add_library(${PROJECT_NAME} SHARED
  src/graph_impl.h
  src/graph_impl.c
  src/graph.c
  include/rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_graph.h
  ${generated_code})

target_compile_definitions(${PROJECT_NAME}
  PRIVATE 
    NDDS_USER_DLL_EXPORT)

target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${generated_code_dir}>"
  "$<INSTALL_INTERFACE:include>")

target_link_libraries(${PROJECT_NAME} PUBLIC RTIConnextDDS::c_api)

install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/${PROJECT_NAME}
  DESTINATION include)

install(FILES ${generated_code_headers}
  DESTINATION include/${PROJECT_NAME})

install(EXPORT export_${PROJECT_NAME}
  FILE ${PROJECT_NAME}Targets.cmake
  DESTINATION lib/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  VERSION "${CMAKE_PROJECT_VERSION}"
  COMPATIBILITY AnyNewerVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION lib/cmake/${PROJECT_NAME}
)

export(EXPORT export_${PROJECT_NAME}
  FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
