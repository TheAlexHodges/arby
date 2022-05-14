//
// Created by ahodges on 14/05/22.
//

#include <binance/connector.hpp>
#include <fmt/format.h>


namespace asio = arby::asio;

asio::awaitable< void >
run()
{
    fmt::print("hello world");
    co_return;
}

int
main()
{
    auto ioc = arby::asio::io_context(1);
    asio::co_spawn(ioc.get_executor(),run(),asio::detached);
    try
    {
        ioc.run();
    }
    catch (std::exception &ec)
    {
        fmt::print("error {}\n",ec.what());
    }

}