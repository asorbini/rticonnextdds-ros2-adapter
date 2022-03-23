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
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 14)

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

set(generated_code_c_headers
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_model.h
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_modelSupport.h
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_modelPlugin.h)

set(generated_code_c_sources
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_model.c
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_modelSupport.c
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_modelPlugin.c)

set(generated_code_c ${generated_code_c_headers} ${generated_code_c_sources})

set(generated_code_cpp_headers
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_model.hpp
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_modelPlugin.hpp)

set(generated_code_cpp_sources
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_model.cxx
  ${generated_code_dir}/${PROJECT_NAME}/${PROJECT_NAME}_modelPlugin.cxx)

set(generated_code_cpp ${generated_code_cpp_headers} ${generated_code_cpp_sources})

set(graph_idl ${CMAKE_CURRENT_SOURCE_DIR}/idl/${PROJECT_NAME}/${PROJECT_NAME}_model.idl)

add_custom_command(OUTPUT ${generated_code_c}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_code_dir}/${PROJECT_NAME}
  COMMAND ${rtiddsgen_bin} -language C
                           -replace
                           -d ${generated_code_dir}/${PROJECT_NAME}
                           -unboundedSupport
                           ${graph_idl}
  DEPENDS ${graph_idl})

add_custom_command(OUTPUT ${generated_code_cpp}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_code_dir}/${PROJECT_NAME}
  COMMAND ${rtiddsgen_bin} -language C++11
                           -replace
                           -d ${generated_code_dir}/${PROJECT_NAME}
                           -unboundedSupport
                           ${graph_idl}
  DEPENDS ${graph_idl})

add_library(${PROJECT_NAME}_c SHARED
  src/adapter_c/graph_impl.h
  src/adapter_c/graph_impl.c
  src/adapter_c/graph.c
  include/rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_c.h
  include/rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_dll.h
  ${generated_code_c})

target_compile_definitions(${PROJECT_NAME}_c
  PRIVATE 
    NDDS_USER_DLL_EXPORT)

target_include_directories(${PROJECT_NAME}_c PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${generated_code_dir}>"
  "$<INSTALL_INTERFACE:include>")

target_link_libraries(${PROJECT_NAME}_c PUBLIC RTIConnextDDS::c_api)

install(TARGETS ${PROJECT_NAME}_c
  EXPORT export_${PROJECT_NAME}_c
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

add_library(${PROJECT_NAME}_cpp SHARED
  src/adapter_cpp/graph.cpp
  include/rticonnextdds_ros2_adapter/rticonnextdds_ros2_adapter_cpp.hpp
  ${generated_code_cpp})

target_compile_definitions(${PROJECT_NAME}_cpp
  PRIVATE 
    NDDS_USER_DLL_EXPORT)

target_include_directories(${PROJECT_NAME}_cpp PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${generated_code_dir}>"
  "$<INSTALL_INTERFACE:include>")

target_link_libraries(${PROJECT_NAME}_cpp PUBLIC
  RTIConnextDDS::cpp2_api
  ${PROJECT_NAME}_c)

install(TARGETS ${PROJECT_NAME}_cpp
  EXPORT export_${PROJECT_NAME}_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/${PROJECT_NAME}
  DESTINATION include)

install(FILES ${generated_code_headers}
  DESTINATION include/${PROJECT_NAME})

install(EXPORT export_${PROJECT_NAME}_c
  FILE ${PROJECT_NAME}_cTargets.cmake
  DESTINATION lib/cmake/${PROJECT_NAME}
)

install(EXPORT export_${PROJECT_NAME}_cpp
  FILE ${PROJECT_NAME}_cppTargets.cmake
  DESTINATION lib/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_cConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_cConfig.cmake"
  INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_cConfigVersion.cmake"
  VERSION "${CMAKE_PROJECT_VERSION}"
  COMPATIBILITY AnyNewerVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_cConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_cConfigVersion.cmake
  DESTINATION lib/cmake/${PROJECT_NAME}
)

export(EXPORT export_${PROJECT_NAME}_c
  FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_cTargets.cmake"
)


configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_cppConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_cppConfig.cmake"
  INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_cppConfigVersion.cmake"
  VERSION "${CMAKE_PROJECT_VERSION}"
  COMPATIBILITY AnyNewerVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_cppConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_cppConfigVersion.cmake
  DESTINATION lib/cmake/${PROJECT_NAME}
)

export(EXPORT export_${PROJECT_NAME}_cpp
  FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_cppTargets.cmake"
)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(examples/hello_c_adapter)
endif()