/*
 * @FilePath: uvServer.hpp
 * @version:
 * @Author: Arrow
 * @Date: 2023-03-29 09:13:42
 * @LastEditors: Arrow
 * @LastEditTime: 2023-03-30 09:51:31
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
    Server(int port, uv_loop_t* loop_ = uv_default_loop()) : port_(port), loop(loop_)
    {
        uv_tcp_init(loop, &server);
        uv_ip4_addr("0.0.0.0", port_, &addr);
        uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
        server.data = this;
    }

    ~Server()
    {
        for (auto client : listClient)
        {
            if (uv_is_active((uv_handle_t*)client.get()))
                uv_close((uv_handle_t*)client.get(), nullptr);
        }
    }

    void start(readCallBack lambda_callback)
    {
        readCallBack_ = lambda_callback;
        uv_listen((uv_stream_t*)&server, SOMAXCONN, on_listen);
    }

    void Broadcast(const std::string& data)
    {
        for (auto& client : listClient)
        {
            Write(client, data);
        }
    }

    void stop() { uv_stop(loop); }

private:
    static void on_listen(uv_stream_t* server, int status)
    {
        auto self = static_cast< Server* >(server->data);
        if (status == 0)
        {
            auto client = std::make_shared< uv_tcp_t >();

            uv_tcp_init(self->loop, client.get());
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
            return;
        }

        std::cout << "Data written!" << std::endl;
    }

    void Write(std::shared_ptr< uv_tcp_t > client, const std::string& data)
    {
        if (uv_is_writable(reinterpret_cast< uv_stream_t* >(client.get())))
        {
            uv_buf_t    buf = uv_buf_init(const_cast< char* >(data.data()), data.size());
            uv_write_t* req = new uv_write_t;
            uv_write(req, reinterpret_cast< uv_stream_t* >(client.get()), &buf, 1, OnWrite);
        } else
        {
            std::cerr << "Socket is not writable" << std::endl;
        }
    }

private:
    readCallBack                             readCallBack_;
    int                                      port_;
    uv_loop_t*                               loop;
    uv_tcp_t                                 server;
    std::list< std::shared_ptr< uv_tcp_t > > listClient;
    struct sockaddr_in                       addr;
};
