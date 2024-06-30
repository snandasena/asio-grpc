//
// Created by sajith.nandasena on 20.06.2024.
//

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
        [&](RPC::Ptr ptr,  const helloworld::HelloRequest &request)
        {
            helloworld::HelloReply response;
            response.set_message("Hello " + request.name());

            auto &rpc = *ptr;
            rpc.finish(response, grpc::Status::OK,
                       [&, p = std::move(ptr)](bool)
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
    const auto thred_count = std::thread::hardware_concurrency();

    helloworld::Greeter::AsyncService service;
    std::unique_ptr<grpc::Server> server;

    std::vector<std::unique_ptr<agrpc::GrpcContext> > grpc_contexts; {
        grpc::ServerBuilder builder;
        for (unsigned i = 0; i < thred_count; ++i)
        {
            grpc_contexts.emplace_back(std::make_unique<agrpc::GrpcContext>(builder.AddCompletionQueue()));
        }

        builder.AddListeningPort("0.0.0.0:3000", grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        agrpc::add_health_check_service(builder);

        server = builder.BuildAndStart();
        agrpc::start_health_check_service(*server, *grpc_contexts.front());
    }

    utils::ServerShutdown shutdown{*server, *grpc_contexts.front()};

    std::vector<std::thread> threads;
    for (unsigned i = 0; i < thred_count; ++i)
    {
        threads.emplace_back([&, i]
        {
            auto &grpc_context = *grpc_contexts[i];
            register_request_handler(grpc_context, service, shutdown);
            grpc_context.run();
        });
    }

    for (auto &thread: threads)
    {
        thread.join();
    }
}
