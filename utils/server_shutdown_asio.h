

#ifndef AGRPC_HELPER_SERVER_SHUTDOWN_ASIO_HPP
#define AGRPC_HELPER_SERVER_SHUTDOWN_ASIO_HPP

#include <agrpc/grpc_executor.hpp>
#include <boost/asio/signal_set.hpp>
#include <grpcpp/server.h>

#include <atomic>
#include <thread>

namespace utils
{
// ---------------------------------------------------
// A helper to properly shut down a gRPC server without deadlocking.
// ---------------------------------------------------
struct ServerShutdown
{
    grpc::Server& server;
    boost::asio::basic_signal_set<agrpc::GrpcExecutor> signals;
    std::atomic_bool is_shutdown{};
    std::thread shutdown_thread;

    ServerShutdown(grpc::Server& server, agrpc::GrpcContext& grpc_context)
        : server(server), signals(grpc_context, SIGINT, SIGTERM)
    {
        signals.async_wait(
            [&](auto&& ec, auto&&)
            {
                if (boost::asio::error::operation_aborted != ec)
                {
                    shutdown();
                }
            });
    }

    void shutdown()
    {
        if (!is_shutdown.exchange(true))
        {
            // This will cause all coroutines to run to completion normally
            // while returning `false` from RPC related steps. Also cancels the signals
            // so that the GrpcContext will eventually run out of work and return
            // from `run()`.
            // We cannot call server.Shutdown() on the same thread that runs a GrpcContext because that can lead to a
            // deadlock, therefore create a new thread.
            shutdown_thread = std::thread(
                [&]
                {
                    signals.cancel();
                    // For Shutdown to ever complete some other thread must be calling grpc_context.run().
                    server.Shutdown();
                });
        }
    }

    ~ServerShutdown()
    {
        if (shutdown_thread.joinable())
        {
            shutdown_thread.join();
        }
        else
        {
            server.Shutdown();
        }
    }
};
}

#endif  // AGRPC_HELPER_SERVER_SHUTDOWN_ASIO_HPP