

set(PROTOFILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

set(CODEGEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/codegen")
file(MAKE_DIRECTORY "${CODEGEN_DIR}")

file(GLOB PROTO_FILES "${PROTOFILE_DIR}/*.proto")

add_library(proto_lib STATIC  ${PROTO_FILES})

protobuf_generate(
        TARGET proto_lib
        LANGUAGE cpp
        IMPORT_DIRS "${PROTOFILE_DIR}"
        PROTOC_OUT_DIR "${CODEGEN_DIR}"
)

protobuf_generate(
        TARGET proto_lib
        LANGUAGE grpc
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
        IMPORT_DIRS "${PROTOFILE_DIR}"
        PROTOC_OUT_DIR "${CODEGEN_DIR}"
        OUT_VAR PROTO_GRPC_FILES
)

target_include_directories(proto_lib
        PUBLIC
        "${CODEGEN_DIR}"
)

target_link_libraries(proto_lib
        PUBLIC
        gRPC::grpc++
)