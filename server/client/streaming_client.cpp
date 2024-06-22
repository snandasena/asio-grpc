//
// Created by sajith.nandasena on 18.06.2024.
//


#include <example_ext.grpc.pb.h>
#include <awaitable_client_rpc.h>
#include <example.grpc.pb.h>
#include <helper_utils.h>


#include <agrpc/alarm.hpp>
#include <agrpc/client_rpc.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include <iostream>

namespace asio = boost::asio;


using ExampleStub = example::Example::Stub;
using ExampleExtStub = example::ExampleExt::Stub;

asio::awaitable<void> make_client_streaming_request(agrpc::GrpcContext &grpc_context, ExampleStub &stub)
{
    using RPC = utils::AwaitableClientRPC<&ExampleStub::PrepareAsyncClientStreaming>;

    RPC rpc{grpc_context};
    rpc.context().set_deadline(std::chrono::system_clock::now() + std::chrono::seconds{5});

    example::Response response;
    const bool start_ok = co_await rpc.start(stub, response);

    abort_if_not(start_ok);

    const bool read_ok = co_await rpc.read_initial_metadata();

    example::Request request;
    const bool write_ok = co_await rpc.write(request);

    const grpc::Status status = co_await rpc.finish();
    abort_if_not(status.ok());


    std::cout << "ClientRPC: Client streaming completed. Response: " << response.integer() << '\n';

    silence_unused(read_ok, write_ok);
}


asio::awaitable<void> make_server_streaming_request(agrpc::GrpcContext &grpc_context, ExampleStub &stub)
{
    using RPC = utils::AwaitableClientRPC<&ExampleStub::PrepareAsyncServerStreaming>;

    RPC rpc{grpc_context};
    rpc.context().set_deadline(std::chrono::system_clock::now() + std::chrono::seconds{5});

    example::Request request;
    request.set_integer(5);

    abort_if_not(co_await rpc.start(stub, request));

    example::Response response;
    while (co_await rpc.read(response))
    {
        std::cout << "ClientRPC: Server streaming: " << response.integer() << "\n";
    }

    const grpc::Status status = co_await rpc.finish();
    abort_if_not(status.ok());
}

asio::awaitable<void> make_bidirectional_streaming_request(agrpc::GrpcContext &grpc_context, ExampleStub &stub)
{
    using RPC = utils::AwaitableClientRPC<&ExampleStub::PrepareAsyncBidirectionalStreaming>;

    RPC rpc{grpc_context};
    rpc.context().set_deadline(std::chrono::system_clock::now() + std::chrono::seconds{5});

    if (!co_await rpc.start(stub))
    {
        co_return;
    }

    example::Request request;
    request.set_integer(1);

    example::Response response;

    using namespace asio::experimental::awaitable_operators;
    auto [read_ok, write_ok] = co_await (rpc.read(response) && rpc.write(request));

    int count{};
    while (read_ok && write_ok && count < 10)
    {
        std::cout << "ClientRPC: Bidirectional streaming: " << response.integer() << '\n';
        request.set_integer(response.integer());
        ++count;
        std::tie(read_ok, write_ok) = co_await (rpc.read(response) && rpc.write(request));
    }

    const grpc::Status status = co_await rpc.finish();
    abort_if_not(status.ok());
}

asio::awaitable<void> make_and_cancel_unary_request(agrpc::GrpcContext &grpc_context, ExampleExtStub &stub)
{
    using RPC = utils::AwaitableClientRPC<&ExampleExtStub::PrepareAsyncSlowUnary>;

    grpc::ClientContext client_context;
    client_context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

    RPC::Request request;
    request.set_delay(2000);
    RPC::Response response;


    const auto not_to_exceed = std::chrono::steady_clock::now() + std::chrono::milliseconds(1900);

    const auto result = co_await asio::experimental::make_parallel_group(
        RPC::request(grpc_context, stub, client_context, request, response, asio::deferred),
        agrpc::Alarm(grpc_context).wait(std::chrono::system_clock::now() + std::chrono::milliseconds(100),
                                        asio::deferred)).async_wait(asio::experimental::wait_for_one(),
                                                                    asio::use_awaitable);

    abort_if_not(grpc::StatusCode::CANCELLED == std::get<1>(result).error_code());
    abort_if_not(std::chrono::steady_clock::now() < not_to_exceed);
}

asio::awaitable<void> make_shutdown_request(agrpc::GrpcContext &grpc_context, ExampleExtStub &stub)
{
    using RPC = utils::AwaitableClientRPC<&ExampleExtStub::PrepareAsyncShutdown>;

    grpc::ClientContext client_context;
    client_context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds{5});

    google::protobuf::Empty response;
    const grpc::Status status = co_await RPC::request(grpc_context, stub, client_context, {}, response);

    if (status.ok())
    {
        std::cout << "ClientRPC: Successfully send shutdown request to server\n";
    } else
    {
        std::cout << "ClientRPC: Failed to send shutdown request to server: " << status.error_message() << '\n';
    }
    abort_if_not(status.ok());
}


int main()
{
    const auto channel = grpc::CreateChannel("0.0.0.0:3000", grpc::InsecureChannelCredentials());
    ExampleStub stub{channel};
    ExampleExtStub stub_ext{channel};

    agrpc::GrpcContext grpc_context;

    asio::co_spawn(grpc_context, [&]()-> asio::awaitable<void>
    {
        co_await make_client_streaming_request(grpc_context, stub);
        co_await make_server_streaming_request(grpc_context, stub);
        co_await make_bidirectional_streaming_request(grpc_context, stub);
        co_await make_and_cancel_unary_request(grpc_context, stub_ext);
        co_await make_shutdown_request(grpc_context, stub_ext);
    }, utils::RethrowFirstArg{});

    grpc_context.run();
}
