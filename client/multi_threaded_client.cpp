//
// Created by sajith.nandasena on 24.06.2024.
//

#include <helloworld.grpc.pb.h>
#include <awaitable_client_rpc.h>
#include <helper_utils.h>

#include <agrpc/asio_grpc.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include <memory>
#include <thread>
#include <vector>


namespace asio = boost::asio;

template<class Itertor>
class RoundRobin
{
public:
    RoundRobin(Itertor begin, std::size_t size): begin_(begin), size_(size)
    {
    }

    decltype(auto) next()
    {
        const auto curr = current_.fetch_add(1, std::memory_order_relaxed);
        const auto pos = curr % size_;
        return *std::next(begin_, pos);
    }

private:
    Itertor begin_;
    std::size_t size_;
    std::atomic_size_t current_{};
};


struct GuardedGrpcContext
{
    agrpc::GrpcContext context;
    asio::executor_work_guard<agrpc::GrpcContext::executor_type> guard{context.get_executor()};
};


asio::awaitable<void> make_request(agrpc::GrpcContext &grpc_context, helloworld::Greeter::Stub &stub)
{
    using RPC = utils::AwaitableClientRPC<&helloworld::Greeter::Stub::PrepareAsyncSayHello>;

    grpc::ClientContext client_context;
    client_context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds{5});

    RPC::Request request;
    request.set_name("Client ");

    RPC::Response response;
    const auto status = co_await RPC::request(grpc_context, stub, client_context, request, response);
    abort_if_not(status.ok());

    std::cout << "Status: " << status.ok()
            << " \nResponse: " << response.message() << std::endl;
}


int main()
{
    helloworld::Greeter::Stub stub{grpc::CreateChannel("0.0.0.0:3000", grpc::InsecureChannelCredentials())};

    std::vector<std::unique_ptr<GuardedGrpcContext> > grpc_contexts;

    const auto thread_count = std::thread::hardware_concurrency();
    for (unsigned i = 0; i < thread_count; ++i)
    {
        grpc_contexts.emplace_back(std::make_unique<GuardedGrpcContext>());
    }

    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    for (unsigned i = 0; i < thread_count; ++i)
    {
        threads.emplace_back([&, i]
        {
            grpc_contexts[i]->context.run();
        });
    }

    RoundRobin round_robin_grpc_contexts{grpc_contexts.begin(), thread_count};
    for (unsigned i = 0; i < 20; ++i)
    {
        auto &grpc_context = round_robin_grpc_contexts.next()->context;
        asio::co_spawn(grpc_context, make_request(grpc_context, stub), utils::RethrowFirstArg{});
    }

    for (auto &grpc_context: grpc_contexts)
    {
        grpc_context->guard.reset();
    }

    for (auto &thread: threads)
    {
        thread.join();
    }
}
