//
// Created by sajith.nandasena on 18.06.2024.
//


#include <awaitable_server_rpc.h>
#include <example.grpc.pb.h>
#include <example_ext.grpc.pb.h>
#include <helper_utils.h>
#include <awaitable_server_rpc.h>
#include <server_shutdown_asio.h>


#include <agrpc/asio_grpc.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/thread_pool.hpp>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>


namespace asio = boost::asio;


using ExampleService = example::Example::AsyncService;
using ExampleExtService = example::ExampleExt::AsyncService;

using ClientStreamingRPC = utils::AwaitableServerRPC<&ExampleService::RequestClientStreaming>;
using ServerStreamingRPC = utils::AwaitableServerRPC<&ExampleService::RequestServerStreaming>;
using BidiStreamingRPC = utils::AwaitableServerRPC<&ExampleService::RequestBidirectionalStreaming>;
using Channel = asio::experimental::channel<void(boost::system::error_code, example::Request)>;
using SlowUnaryRPC = asio::use_awaitable_t<agrpc::GrpcExecutor>::as_default_on_t<agrpc::ServerRPC<&
    ExampleExtService::RequestSlowUnary> >;
using ShutdownRPC = utils::AwaitableServerRPC<&ExampleExtService::RequestShutdown>;


asio::awaitable<void> handle_client_streaming_reuqest(ClientStreamingRPC &rpc)
{
    if (!co_await rpc.send_initial_metadata())
    {
        co_return;
    }

    bool read_ok;

    do
    {
        example::Request request;
        read_ok = co_await rpc.read(request);
    } while (read_ok);

    example::Response response;
    response.set_integer(42);

    co_await rpc.finish(response, grpc::Status::OK);
}


asio::awaitable<void> handle_server_streaming_requests(ServerStreamingRPC &rpc, example::Request &request)
{
    example::Response response;
    response.set_integer(request.integer());

    while (co_await rpc.write(response) && response.integer() > 0)
    {
        response.set_integer(response.integer() - 1);
    }

    co_await rpc.finish(grpc::Status::OK);
}

asio::awaitable<void> reader(BidiStreamingRPC &rpc, Channel &channel)
{
    while (true)
    {
        example::Request request;
        if (!co_await rpc.read(request))
        {
            break;
        }

        co_await channel.async_send(boost::system::error_code{}, std::move(request),
                                    asio::as_tuple(asio::use_awaitable));
    }
    channel.close();
}


asio::awaitable<bool> writer(BidiStreamingRPC &rpc, Channel &channel, asio::thread_pool &thread_pool)
{
    bool ok{true};

    while (ok)
    {
        const auto [ec, request] = co_await channel.async_receive(asio::as_tuple(asio::use_awaitable));
        if (ec)
        {
            break;
        }

        co_await asio::post(asio::bind_executor(thread_pool, asio::use_awaitable));

        example::Response response;
        response.set_integer(request.integer() * 2);

        ok = co_await rpc.write(response);
    }

    co_return ok;
}

auto bidirectional_streaming_rpc_handler(asio::thread_pool &thread_pool)
{
    return [&](BidiStreamingRPC &rpc) -> asio::awaitable<void>
    {
        static constexpr auto MAX_BUFFER_SIZE = 2;
        Channel channel{co_await asio::this_coro::executor, MAX_BUFFER_SIZE};

        using namespace asio::experimental::awaitable_operators;
        const auto ok = co_await (reader(rpc, channel) && writer(rpc, channel, thread_pool));

        if (!ok)
        {
            co_return;
        }

        co_await rpc.finish(grpc::Status::OK);
    };
}

asio::awaitable<void, agrpc::GrpcExecutor> handle_slow_unary_request(SlowUnaryRPC &rpc, SlowUnaryRPC::Request &request)
{
    agrpc::Alarm alam{co_await asio::this_coro::executor};

    co_await alam.wait(std::chrono::system_clock::now() + std::chrono::milliseconds{request.delay()},
                       asio::use_awaitable_t<agrpc::GrpcExecutor>{});

    co_await rpc.finish({}, grpc::Status::OK);
}


int main()
{
    std::unique_ptr<grpc::Server> server;
    grpc::ServerBuilder builder;

    agrpc::GrpcContext grpc_context{builder.AddCompletionQueue()};
    builder.AddListeningPort("0.0.0.0:3000", grpc::InsecureServerCredentials());

    ExampleService service;
    builder.RegisterService(&service);

    ExampleExtService service_ext;
    builder.RegisterService(&service_ext);

    server = builder.BuildAndStart();

    abort_if_not(bool{server});

    utils::ServerShutdown server_shutdown{*server, grpc_context};
    asio::thread_pool thread_pool{1};


    agrpc::register_awaitable_rpc_handler<ClientStreamingRPC>(grpc_context, service, &handle_client_streaming_reuqest,
                                                              utils::RethrowFirstArg{});
    agrpc::register_awaitable_rpc_handler<ServerStreamingRPC>(grpc_context, service, &handle_server_streaming_requests,
                                                              utils::RethrowFirstArg{});

    agrpc::register_awaitable_rpc_handler<BidiStreamingRPC>(grpc_context, service,
                                                            bidirectional_streaming_rpc_handler(thread_pool),
                                                            utils::RethrowFirstArg{});

    agrpc::register_awaitable_rpc_handler<SlowUnaryRPC>(grpc_context, service_ext, &handle_slow_unary_request,
                                                        utils::RethrowFirstArg{});

    agrpc::register_awaitable_rpc_handler<ShutdownRPC>(grpc_context, service_ext,
                                                       [&](ShutdownRPC &rpc,
                                                           const ShutdownRPC::Request &)-> asio::awaitable<void>
                                                       {
                                                           if (co_await rpc.finish({}, grpc::Status::OK))
                                                           {
                                                               std::cout << "Received shutdown request from client\n";
                                                               server_shutdown.shutdown();
                                                           }
                                                       }, utils::RethrowFirstArg{});

    grpc_context.run();
}
