
find_package(GTest REQUIRED)
find_package(doctest REQUIRED)

#add_executable(test_app main.cpp)
add_executable(test_app
        test_app.cpp
)

target_link_libraries(test_app
        PRIVATE
        proto_lib
        doctest::doctest
        gtest::gtest
        protobuf::libprotobuf
        asio-grpc::asio-grpc
        GTest::gmock
)

