add_executable(AsioGrpcClient client.cpp)
target_link_libraries(AsioGrpcClient helper_util proto_lib protobuf::libprotobuf asio-grpc::asio-grpc)


add_executable(StreamingClient streaming_client.cpp)
target_link_libraries(StreamingClient helper_util proto_lib protobuf::libprotobuf asio-grpc::asio-grpc)

add_executable(HealthCheckClient health_check_client.cpp)
target_link_libraries(HealthCheckClient helper_util proto_lib grpc_utils protobuf::libprotobuf asio-grpc::asio-grpc)

add_executable(MultiThreadedClient multi_threaded_client.cpp)
target_link_libraries(MultiThreadedClient helper_util proto_lib grpc_utils protobuf::libprotobuf asio-grpc::asio-grpc)