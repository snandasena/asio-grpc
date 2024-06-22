//
// Created by sajith.nandasena on 18.06.2024.
//

#ifndef AWAITABLE_SERVER_RPC_H
#define AWAITABLE_SERVER_RPC_H

#include <agrpc/server_rpc.hpp>
#include <boost/asio/use_awaitable.hpp>


namespace utils
{
    template<auto RequestRPC>
    using AwaitableServerRPC = boost::asio::use_awaitable_t<>::as_default_on_t<agrpc::ServerRPC<RequestRPC> >;
}


#endif //AWAITABLE_SERVER_RPC_H
