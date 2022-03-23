cmake_minimum_required(VERSION 3.5)

project(rticonnextdds_ros2_adapter_test C CXX)

if(NOT TARGET rticonnextdds_ros2_adapter_c)
  find_package(rticonnextdds_ros2_adapter_c CONFIG REQUIRED)
endif()

# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 14)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

include(GoogleTest)

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

set(graph_test_idl ${CMAKE_CURRENT_SOURCE_DIR}/../idl/rticonnextdds_ros2_adapter/${PROJECT_NAME}_model.idl)

add_custom_command(OUTPUT ${generated_code_c}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_code_dir}/${PROJECT_NAME}
  COMMAND ${rtiddsgen_bin} -language C
                           -replace
                           -d ${generated_code_dir}/${PROJECT_NAME}
                           -unboundedSupport
                           ${graph_test_idl}
  DEPENDS ${graph_test_idl})

add_custom_command(OUTPUT ${generated_code_cpp}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${generated_code_dir}/${PROJECT_NAME}
  COMMAND ${rtiddsgen_bin} -language C++11
                           -replace
                           -d ${generated_code_dir}/${PROJECT_NAME}
                           -unboundedSupport
                           ${graph_test_idl}
  DEPENDS ${graph_test_idl})

add_library(${PROJECT_NAME}_c_model SHARED
  ${generated_code_c}
)
target_link_libraries(${PROJECT_NAME}_c_model RTIConnextDDS::c_api)
target_compile_definitions(${PROJECT_NAME}_c_model PRIVATE NDDS_USER_DLL_EXPORT)
target_include_directories(${PROJECT_NAME}_c_model PUBLIC
  "$<BUILD_INTERFACE:${generated_code_dir}>"
  "$<INSTALL_INTERFACE:include>")

add_library(${PROJECT_NAME}_cpp_model SHARED
  ${generated_code_cpp}
)
target_link_libraries(${PROJECT_NAME}_cpp_model RTIConnextDDS::cpp2_api)
target_compile_definitions(${PROJECT_NAME}_cpp_model PRIVATE NDDS_USER_DLL_EXPORT)
target_include_directories(${PROJECT_NAME}_cpp_model PUBLIC
"$<BUILD_INTERFACE:${generated_code_dir}>"
"$<INSTALL_INTERFACE:include>")

function(define_tester tester)
  cmake_parse_arguments(_test
  "" # boolean arguments
  "" # single value arguments
  "SOURCES;LIBRARIES" # multi-value arguments
  ${ARGN} # current function arguments
  )
  add_executable(${tester}
    ${_test_SOURCES}
  )
  target_link_libraries(${tester}
    gtest_main
    ${_test_LIBRARIES}
  )
  target_include_directories(${tester} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIRECTORY}")
  gtest_discover_tests(${tester})
endfunction()

define_tester(test_c_graph
  SOURCES
    test_c_graph.cpp
  LIBRARIES
  rticonnextdds_ros2_adapter_c
    ${PROJECT_NAME}_c_model
)

define_tester(test_cpp_graph
  SOURCES
    test_cpp_graph.cpp
  LIBRARIES
  rticonnextdds_ros2_adapter_cpp
  ${PROJECT_NAME}_cpp_model
)
