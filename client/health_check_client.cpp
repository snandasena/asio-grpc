//
// Created by sajith.nandasena on 24.06.2024.
//

//
// Created by sajith.nandasena on 17.06.2024.
//

#include <grpc/health/v1/health.grpc.pb.h>
#include <helper_utils.h>
#include <awaitable_client_rpc.h>

#include <agrpc/asio_grpc.hpp>
#include <boost/asio/co_spawn.hpp>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include <agrpc/client_rpc.hpp>


namespace asio = boost::asio;


int main()
{
    grpc::health::v1::Health::Stub stub{grpc::CreateChannel("0.0.0.0:3000", grpc::InsecureChannelCredentials())};
    agrpc::GrpcContext grpc_context;

    grpc::Status status;

    asio::co_spawn(grpc_context, [&]()-> asio::awaitable<void>
    {
        using RPC = utils::AwaitableClientRPC<&grpc::health::v1::Health::Stub::PrepareAsyncCheck>;
        grpc::ClientContext client_context;
        grpc::health::v1::HealthCheckResponse response;

        status = co_await RPC::request(grpc_context, stub, client_context,
                                       grpc::health::v1::HealthCheckRequest::default_instance(), response);
        std::cout << "Status: " << status.ok()
                << " \nResponse: " << response.DebugString() << std::endl;
    }, utils::RethrowFirstArg{});

    grpc_context.run();

    abort_if_not(status.ok());

    return 0;
}
