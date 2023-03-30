/*
 * @Descripttion:
 * @version:
 * @Author: Arrow
 * @Date: 2023-03-29 09:07:45
 * @LastEditors: Arrow
 * @LastEditTime: 2023-03-30 14:17:21
 */

#include <iostream>
#include "uv.h"

#include "uvClient.hpp"
#include "uvServer.hpp"
#include <thread>

Client c;
Server server(1234);

int count = 0;

void writeback(bool status)
{
    std::cout << status << ++count;
}

void readback(std::string data)
{
    auto threadId = std::this_thread::get_id();
    std::cout << data;
    //  for (int i = 0; i < 100; i++)
    //  {
    //      std::string str = std::string("count ") + std::to_string(i);
    //      server.Broadcast(str);
    //  }
    //  for (int i = 0; i < 100; i++)
    //  {
    //      std::string str = std::string("count ") + std::to_string(i);
    //      c.Write(str, writeback);
    //      // std::this_thread::sleep_for(std::chrono::seconds(1));
    //  }
}

void threadLoop()
{
    uv_loop_t* loop = uv_default_loop();
    std::cout << "Hello, threadClient!\n";
    server.start(readback);
    // c.Connect("127.0.0.1", 1234, readback);
    uv_run(loop, UV_RUN_DEFAULT);
}

void threadClient()
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for (int i = 0; i < 100; i++)
    {
        std::string str = std::string("count ") + std::to_string(i);
        // c.Write(str, writeback);
        server.Broadcast(str);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int add(int a, int b)
{
    return a + b;
}

#include "doctest.h"

TEST_CASE("addition test")
{
    CHECK(add(1, 1) == 2);
    CHECK(add(2, 3) == 5);
    CHECK(add(5, -3) == 2);
}

TEST_CASE("addition test")
{
    CHECK(add(1, 1) == 2);
    CHECK(add(2, 3) == 5);
    CHECK(add(5, -3) == 2);
}

void main(int argc, char** argv)
{
    std::thread loop_t(threadLoop);
    std::thread client_t(threadClient);
    loop_t.join();
    client_t.join();
    //  doctest::Context(argc, argv).run();
    //  doctest::Context context;
    //  context.setOption("no_colors", true);    // 禁用颜色输出，以便于文件查看
    //  context.setOption("out", "test_result.txt");
    //  context.run();
}
