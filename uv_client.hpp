/*
 * @Descripttion: ${fileheader.belle}
 * @version:
 * @Author: Arrow
 * @Date: 2023-03-29 09:13:42
 * @LastEditors: Arrow
 * @LastEditTime: 2023-03-30 14:14:40
 */

#include <iostream>
#include <string>
#include <functional>
#include <uv.h>
using readCallBack  = std::function< void(std::string data) >;
using writeCallBack = std::function< void(bool) >;

class Client
{
public:
    explicit Client(uv_loop_t* loop = uv_default_loop()) : loop_(loop), tcp_client_(new uv_tcp_t)
    {
        uv_tcp_init(loop_, tcp_client_);
        tcp_client_->data = this;
    }

    void Connect(const std::string& host, int port, readCallBack read_callback)
    {
        readCB_ = read_callback;
        struct sockaddr_in addr;
        uv_ip4_addr(host.c_str(), port, &addr);

        uv_tcp_connect(&client_, tcp_client_, reinterpret_cast< const struct sockaddr* >(&addr),
                       OnConnect);
    }

    void Write(const std::string& data, writeCallBack write_cb)
    {
        if (uv_is_writable(reinterpret_cast< uv_stream_t* >(tcp_client_)))
        {
            writeCB_        = write_cb;
            buf_            = uv_buf_init(const_cast< char* >(data.data()), data.size());
            uv_write_t* req = new uv_write_t;
            req->data       = this;

            // 创建异步句柄对象，并设置回调函数和回调数据
            uv_async_t* async = new uv_async_t;
            async->data       = req;
            uv_async_init(loop_, async, onAsync);
            uv_async_send(async);

            // uv_write(req, reinterpret_cast< uv_stream_t* >(tcp_client_), &buf, 1, OnWrite);
        } else
        {
            std::cerr << "Socket is not writable" << std::endl;
        }
    }

private:
    static void OnConnect(uv_connect_t* req, int status)
    {
        if (status < 0)
        {
            std::cerr << "Connect error: " << uv_strerror(status) << std::endl;
            return;
        }

        std::cout << "Connected!" << std::endl;

        Client* client = reinterpret_cast< Client* >(req->handle->data);
        client->OnConnected();
    }

    static void OnWrite(uv_write_t* req, int status)
    {
        if (0 == status)
        {
            auto pClient = reinterpret_cast< Client* >(req->data);
            // auto callback = reinterpret_cast< writeCallBack* >(req->data);
            (pClient->writeCB_)(0 == status);

            std::cout << "Data written!" << std::endl;

        } else
        {
            std::cerr << "Write error: " << uv_strerror(status) << std::endl;
        }
        delete req;
    }

    static void OnRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
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
            std::string data(buf->base, nread);
            std::cout << "Received data: " << data << std::endl;
            auto pClient = reinterpret_cast< Client* >(stream->data);
            (pClient->readCB_)(std::string(buf->base, nread));
        }

        delete[] buf->base;
    }

    void OnConnected()
    {
        // TODO: handle connection
        // Start reading from socket

        uv_read_start(reinterpret_cast< uv_stream_t* >(tcp_client_), OnAlloc, OnRead);
    }

    static void OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
    {
        buf->base = new char[suggested_size];
        buf->len  = suggested_size;
    }

    static void onAsync(uv_async_t* handle)
    {
        // 获取写操作数据
        auto req     = (uv_write_t*)handle->data;
        auto pClient = (Client*)req->data;
        // 执行写操作
        uv_write(req, reinterpret_cast< uv_stream_t* >(pClient->tcp_client_), &pClient->buf_, 1,
                 OnWrite);
        delete handle;
        // uv_write(req, reinterpret_cast< uv_stream_t* >(tcp_client_), &buf, 1, OnWrite);
    }

private:
    writeCallBack writeCB_;
    readCallBack  readCB_;
    uv_buf_t      buf_;
    uv_connect_t  client_;
    uv_loop_t*    loop_;
    uv_tcp_t*     tcp_client_;
};
