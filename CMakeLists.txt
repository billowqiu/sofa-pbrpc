cmake_minimum_required(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.cmake"
#     CACHE FILEPATH
#     "Toolchain to use for building this package and dependencies")

set(HUNTER_CONFIGURATION_TYPES "Release;Debug" CACHE STRING
    "Configuration types used when building Hunter dependencies")

# include(CMakeDependentOption)
include(HunterGate)

HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.165.tar.gz"
    SHA1 "5a73f91df5f6109c0bb1104d0c0ee423f7bece79"
)
project(sofa-pbrpc VERSION 0.5.0)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(SOFA_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
    CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(cxx_flags -Wall -pedantic)
  if(SOFA_WARNINGS_AS_ERRORS)
    list(APPEND cxx_flags -Werror)
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  list(APPEND cxx_flags -Wno-unused-private-field)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -undefined dynamic_lookup")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(HUNTER_ENABLED)
  set(hunter_config "CONFIG")
else()
  set(hunter_config "")
endif()

set(package_deps)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9")
  message(FATAL_ERROR "Must use gcc >= 4.9")
endif()

hunter_add_package(Boost)
find_package(Boost ${hunter_config} REQUIRED)
list(APPEND LIBS Boost::boost)
list(APPEND package_deps Boost)

hunter_add_package(Protobuf)
find_package(Protobuf CONFIG REQUIRED)
list(APPEND LIBS protobuf::libprotobuf)

hunter_add_package(Snappy)
find_package(Snappy CONFIG REQUIRED)
list(APPEND LIBS Snappy::snappy)
list(APPEND package_deps Snappy)

hunter_add_package(ZLIB)
find_package(ZLIB CONFIG REQUIRED)
list(APPEND LIBS ZLIB::zlib)
list(APPEND package_deps ZLIB)

#jaeger-client-cpp
add_subdirectory(deps/jaeger-client-cpp)
list(APPEND LIBS jaegertracing-static)


include_directories(deps/jaeger-client-cpp/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/deps/jaeger-client-cpp/src)

option(SOFA_BUILD_EXAMPLES "Build examples" ON)

include(CTest)
if(BUILD_TESTING)
  hunter_add_package(GTest)
  find_package(GTest ${hunter_config} REQUIRED)
endif()

# check if protobuf was found
# if(PROTOBUF_FOUND)
#     message ("protobuf found")
# else()
#     message (FATAL_ERROR "Cannot find Protobuf")
# endif()

# PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS 
#   src/sofa/pbrpc/rpc_meta.proto src/sofa/pbrpc/rpc_option.proto src/sofa/pbrpc/builtin_service.proto)

set(MESSAGES_PROTOS 
  src/sofa/pbrpc/rpc_option.proto 
  src/sofa/pbrpc/rpc_meta.proto 
  src/sofa/pbrpc/builtin_service.proto
  src/sofa/pbrpc/rpc_telemetry.proto)
set(MESSAGES_SRCS)
set(MESSAGES_HDRS)

#Code Generation
foreach(proto_file ${MESSAGES_PROTOS})
  get_filename_component(proto_file_abs ${proto_file} ABSOLUTE)
  message("proto_file_abs:"  ${proto_file_abs})
  get_filename_component(basename ${proto_file} NAME_WE)
  message("basename:"  ${basename})

  set(generated_files ${CMAKE_CURRENT_SOURCE_DIR}/src/sofa/pbrpc/${basename}.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/src/sofa/pbrpc/${basename}.pb.h)

  list(APPEND MESSAGES_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/sofa/pbrpc/${basename}.pb.cc)
  list(APPEND MESSAGES_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/src/sofa/pbrpc/${basename}.pb.h)

  add_custom_command(
      OUTPUT ${generated_files}
      COMMAND protobuf::protoc --cpp_out ${CMAKE_CURRENT_SOURCE_DIR}/src/ -I ${CMAKE_CURRENT_SOURCE_DIR}/src ${proto_file_abs}
      COMMENT "Generating ${generated_files} from ${proto_file}"
      VERBATIM
  )
endforeach()

# add_library(messages ${MESSAGES_SRCS})

# target_link_libraries(messages protobuf::libprotobuf)
# target_include_directories(messages PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/..)
# target_include_directories(messages PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

foreach(genfile ${MESSAGES_HDRS})
  message(${genfile})
endforeach()
foreach(genfile ${MESSAGES_SRCS})
  message(${genfile})
endforeach()

set(SRC
    ${MESSAGES_SRCS}
    src/sofa/pbrpc/binary_rpc_request_parser.cc
    src/sofa/pbrpc/binary_rpc_request.cc
    src/sofa/pbrpc/block_wrappers.cc
    # src/sofa/pbrpc/boost_system_error_code.cc
    src/sofa/pbrpc/buffer.cc
    src/sofa/pbrpc/builtin_service_impl.cc
    src/sofa/pbrpc/common_internal.cc
    src/sofa/pbrpc/common.cc
    src/sofa/pbrpc/compressed_stream.cc
    src/sofa/pbrpc/dynamic_rpc_channel_impl.cc
    src/sofa/pbrpc/gzip_stream.cc
    src/sofa/pbrpc/http_rpc_request_parser.cc
    src/sofa/pbrpc/http_rpc_request.cc
    src/sofa/pbrpc/io_service_pool.cc
    src/sofa/pbrpc/lz4.cc
    src/sofa/pbrpc/mock_test_helper.cc
    src/sofa/pbrpc/pbjson.cc
    src/sofa/pbrpc/profiling.cc
    src/sofa/pbrpc/rpc_channel.cc
    src/sofa/pbrpc/rpc_client_impl.cc
    src/sofa/pbrpc/rpc_client.cc
    src/sofa/pbrpc/rpc_controller.cc
    src/sofa/pbrpc/rpc_endpoint.cc
    src/sofa/pbrpc/rpc_error_code.cc
    src/sofa/pbrpc/rpc_request_parser.cc
    src/sofa/pbrpc/rpc_request.cc
    src/sofa/pbrpc/rpc_server_impl.cc
    src/sofa/pbrpc/rpc_server.cc
    src/sofa/pbrpc/simple_rpc_channel_impl.cc
    src/sofa/pbrpc/string_utils.cc
    src/sofa/pbrpc/tcmalloc_extension_helper.cc
    src/sofa/pbrpc/thread_group.cc
    src/sofa/pbrpc/timeout_manager_impl.cc
    src/sofa/pbrpc/timeout_manager.cc
    src/sofa/pbrpc/web_service.cc
    src/sofa/pbrpc/tracing.cc)

function(add_lib_deps lib)
  message(STATUS "add LIBS: ${LIBS}")
  target_include_directories(${lib} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>)
  target_link_libraries(${lib} PUBLIC ${link_flags} ${LIBS})
  target_compile_options(${lib} PUBLIC ${cxx_flags})
endfunction()

add_library(sofa-pbrpc STATIC ${SRC})
set_target_properties(sofa-pbrpc PROPERTIES OUTPUT_NAME sofa-pbrpc)
add_lib_deps(sofa-pbrpc)
if(NOT SOFA-PBRPC_LIB)
  set(SOFA-PBRPC_LIB sofa-pbrpc)
endif()
list(APPEND SOFA-PBRPC_LIB sofa-pbrpc)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sample/echo/echo_service.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/sample/echo/echo_service.pb.cc
  COMMAND protobuf::protoc --cpp_out ${CMAKE_CURRENT_SOURCE_DIR}/sample/echo -I ${CMAKE_CURRENT_SOURCE_DIR}/src -I ${CMAKE_CURRENT_SOURCE_DIR}/sample/echo/ ${CMAKE_CURRENT_SOURCE_DIR}/sample/echo/echo_service.proto
  VERBATIM
)

add_executable(echoclient_async sample/echo/client_async.cc sample/echo/echo_service.pb.cc)
target_link_libraries(echoclient_async PUBLIC ${SOFA-PBRPC_LIB})

add_executable(echoserver sample/echo/server.cc sample/echo/echo_service.pb.cc)
target_link_libraries(echoserver PUBLIC ${SOFA-PBRPC_LIB})
