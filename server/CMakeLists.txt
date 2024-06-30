add_executable(AsioGrpcServer server.cpp)
target_link_libraries(AsioGrpcServer helper_util proto_lib protobuf::libprotobuf asio-grpc::asio-grpc)


add_executable(StreamingServer streaming_server.cpp)
target_link_libraries(StreamingServer helper_util proto_lib protobuf::libprotobuf asio-grpc::asio-grpc)

add_executable(MultiThreadedServer multi_threaded_server.cpp)
target_link_libraries(MultiThreadedServer
        helper_util
        proto_lib
        grpc_utils
        protobuf::libprotobuf
        asio-grpc::asio-grpc)



