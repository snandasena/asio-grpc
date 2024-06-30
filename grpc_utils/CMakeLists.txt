
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

set(PROTO_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/grpc/health/v1")

set(CODEGEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/codegen")
file(MAKE_DIRECTORY "${CODEGEN_DIR}")

file(GLOB PROTO_FILES "${PROTO_FILE_DIR}/*proto")

add_library(grpc_utils STATIC  ${PROTO_FILES})


protobuf_generate(
        TARGET grpc_utils
        LANGUAGE cpp
        IMPORT_DIRS "${HEALTH_PROTO_DIR}"
        PROTOC_OUT_DIR "${CODEGEN_DIR}"
)

protobuf_generate(
        TARGET grpc_utils
        LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
        IMPORT_DIRS "${HEALTH_PROTO_DIR}"
        PROTOC_OUT_DIR "${CODEGEN_DIR}"
        OUT_VAR PROTO_GRPC_FILES
)

target_include_directories(grpc_utils
        PUBLIC
        "${CODEGEN_DIR}"
)

target_link_libraries(grpc_utils
        PUBLIC
        gRPC::grpc++
)