//
// Created by sajith.nandasena on 17.06.2024.
//

#include <helloworld.grpc.pb.h>
#include <helper_utils.h>
#include <awaitable_client_rpc.h>

#include <agrpc/asio_grpc.hpp>
#include <boost/asio/co_spawn.hpp>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include <agrpc/client_rpc.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <cstdio>
#include <cstdlib>

namespace asio = boost::asio;


int main()
{
    helloworld::Greeter::Stub stub{grpc::CreateChannel("0.0.0.0:3000", grpc::InsecureChannelCredentials())};
    agrpc::GrpcContext grpc_context;

    grpc::Status status;

    asio::co_spawn(grpc_context, [&]()-> asio::awaitable<void>
    {
        using RPC = utils::AwaitableClientRPC<&helloworld::Greeter::Stub::PrepareAsyncSayHello>;
        grpc::ClientContext client_context;
        helloworld::HelloRequest request;
        request.set_name("Client ");
        helloworld::HelloReply response;

        status = co_await RPC::request(grpc_context, stub, client_context, request, response);
        std::cout << "Status: " << status.ok()
                << " \nResponse: " << response.message() << std::endl;
    }, utils::RethrowFirstArg{});

    grpc_context.run();

    abort_if_not(status.ok());

    return 0;
}
