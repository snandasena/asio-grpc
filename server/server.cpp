// src/main.cpp


#include <helloworld.grpc.pb.h>
#include <helper_utils.h>
#include <awaitable_server_rpc.h>

#include <agrpc/asio_grpc.hpp>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <agrpc/server_rpc.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <memory>

namespace asio = boost::asio;


int main()
{
    helloworld::Greeter::AsyncService service;
    std::unique_ptr<grpc::Server> server;

    grpc::ServerBuilder builder;
    agrpc::GrpcContext grpc_context{builder.AddCompletionQueue()};

    builder.AddListeningPort("0.0.0.0:3000", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    server = builder.BuildAndStart();

    using RPC = utils::AwaitableServerRPC<&helloworld::Greeter::AsyncService::RequestSayHello>;

    agrpc::register_awaitable_rpc_handler<RPC>(grpc_context, service,
                                               [&](RPC &rpc, RPC::Request &request) -> asio::awaitable<void>
                                               {
                                                   helloworld::HelloReply respone;
                                                   respone.set_message("Hello, " + request.name());
                                                   co_await rpc.finish(respone, grpc::Status::OK);
                                                   server->Shutdown();
                                               }, utils::RethrowFirstArg{});


    grpc_context.run();
    return 0;
}
