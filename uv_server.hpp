/*
 * @FilePath: uvServer.hpp
 * @version:
 * @Author: Arrow
 * @Date: 2023-03-29 09:13:42
 * @LastEditors: Arrow
 * @LastEditTime: 2023-03-30 14:53:27
 * @Copyright: 2023 xxxTech CO.,LTD. All Rights Reserved.
 * @Descripttion:
 */

#include <functional>
#include <list>
#include <uv.h>

using readCallBack = std::function< void(std::string data) >;

class Server
{
public:
    Server(int port, uv_loop_t* loop = uv_default_loop())
        : port_(port), loop_(loop), tcp_server_(new uv_tcp_t)
    {
        uv_tcp_init(loop_, tcp_server_);
        uv_ip4_addr("0.0.0.0", port_, &addr);
        uv_tcp_bind(tcp_server_, (const struct sockaddr*)&addr, 0);
        tcp_server_->data = this;
    }

    ~Server()
    {
        if (uv_is_active((uv_handle_t*)tcp_server_))
        {
            uv_close((uv_handle_t*)tcp_server_, nullptr);
        }
        for (auto client : listClient)
        {
            if (uv_is_active((uv_handle_t*)client.get()))
                uv_close((uv_handle_t*)client.get(), nullptr);
        }
    }

    void start(readCallBack lambda_callback)
    {
        readCallBack_ = lambda_callback;
        uv_listen((uv_stream_t*)tcp_server_, SOMAXCONN, on_listen);
    }

    void Broadcast(const std::string& data)
    {
        for (auto& client : listClient)
        {
            Write(client, data);
        }
    }

    void stop() { uv_stop(loop_); }

private:
    static void on_listen(uv_stream_t* server, int status)
    {
        auto self = static_cast< Server* >(server->data);
        if (status == 0)
        {
            auto client = std::make_shared< uv_tcp_t >();

            uv_tcp_init(self->loop_, client.get());
            if (uv_accept(server, (uv_stream_t*)client.get()) == 0)
            {
                auto callback = new readCallBack(self->readCallBack_);
                client->data  = callback;
                uv_read_start((uv_stream_t*)client.get(), on_alloc_cb, on_read_cb);
                self->listClient.emplace_back(client);
            } else
            {
                uv_close((uv_handle_t*)client.get(), nullptr);
            }
        }
    }

    static void on_alloc_cb(uv_handle_t* handle, size_t size, uv_buf_t* buf)
    {
        buf->base = new char[size];
        buf->len  = size;
    }

    static void on_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
    {
        if (nread < 0)
        {
            if (nread != UV_EOF)
            {
                std::cerr << "Read error: " << uv_strerror(nread) << std::endl;
            }
            uv_close(reinterpret_cast< uv_handle_t* >(stream), nullptr);
            delete[] buf->base;
            return;
        }

        if (nread > 0)
        {
            auto callback = reinterpret_cast< readCallBack* >(stream->data);
            (*callback)(std::string(buf->base, nread));
        }
        delete[] buf->base;
    }

    static void OnWrite(uv_write_t* req, int status)
    {
        if (status < 0)
        {
            std::cerr << "Write error: " << uv_strerror(status) << std::endl;
            delete req;
            return;
        }

        std::cout << "Data written!" << std::endl;
        delete req;
    }

    void Write(std::shared_ptr< uv_tcp_t > client, const std::string& data)
    {
        if (uv_is_writable(reinterpret_cast< uv_stream_t* >(client.get())))
        {
            client->data = new std::string(data);

            uv_write_t* req = new uv_write_t;
            req->data       = client.get();

            // 创建异步句柄对象，并设置回调函数和回调数据
            uv_async_t* async = new uv_async_t;
            async->data       = req;
            uv_async_init(loop_, async, onAsync);
            uv_async_send(async);
        }
    }

    static void onAsync(uv_async_t* handle)
    {
        // 获取写操作数据
        auto req     = (uv_write_t*)handle->data;
        auto pClient = (uv_stream_t*)req->data;
        auto str     = (std::string*)pClient->data;
        auto buf     = uv_buf_init(str->data(), str->size());

        // 执行写操作
        uv_write(req, pClient, &buf, 1, OnWrite);
        delete handle;
        delete str;
    }

private:
    readCallBack                             readCallBack_;
    int                                      port_;
    uv_loop_t*                               loop_;
    uv_tcp_t*                                tcp_server_;
    std::list< std::shared_ptr< uv_tcp_t > > listClient;
    struct sockaddr_in                       addr;
};
