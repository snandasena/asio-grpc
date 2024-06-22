//
// Created by sajith.nandasena on 20.06.2024.
//

#include <health.grpc.pb.h>
#include <helloworld.grpc.pb.h>
#include <server_shutdown_asio.h>
#include <helper_utils.h>

#include <agrpc/asio_grpc.hpp>
#include <agrpc/health_check_service.hpp>
#include <boost/asio/detached.hpp>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <memory>
#include <thread>
#include <vector>

namespace asio = boost::asio;


void register_request_handler(agrpc::GrpcContext &grpc_context, helloworld::Greeter::AsyncService &service,
                              utils::ServerShutdown &shutdown)
{
    using RPC = agrpc::ServerRPC<&helloworld::Greeter::AsyncService::RequestSayHello>;

    agrpc::register_callback_rpc_handler<RPC>(
        grpc_context, service,
        [&](RPC::Ptr &ptr, const helloworld::HelloRequest &request)
        {
            helloworld::HelloReply response;
            response.set_message("Hello " + request.name());

            auto &rpc = *ptr;
            rpc.finish(response, grpc::Status::OK, [&, p = std::move(ptr)](bool)
            {
                static std::atomic_int couter{};
                if (couter.fetch_add(1) == 19)
                {
                    shutdown.shutdown();
                }
            });
        }, utils::RethrowFirstArg{});
}


int main()
{
    helloworld::Greeter::AsyncService service;
    std::unique_ptr<grpc::Server> server;

    std::vector<agrpc::GrpcContext> grpc_contexts;

}