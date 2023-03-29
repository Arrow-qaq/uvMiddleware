/*
 * @Descripttion: this is test
 * @version:
 * @Author: Arrow
 * @Date: 2023-03-29 09:13:42
 * @LastEditors: Arrow
 * @LastEditTime: 2023-03-29 16:21:16
 */
#include <iostream>
#include <string>
#include <functional>
#include <uv.h>
using readCallBack = std::function< void(std::string data) >;

class Client
{
public:
    explicit Client(uv_loop_t* loop = uv_default_loop()) : loop_(loop), socket_(new uv_tcp_t)
    {
        uv_tcp_init(loop_, socket_);
        socket_->data = this;
    }

    void Connect(const std::string& host, int port, readCallBack lambda_callback)
    {
        readCallBack_ = lambda_callback;
        struct sockaddr_in addr;
        uv_ip4_addr(host.c_str(), port, &addr);

        uv_connect_t* req = new uv_connect_t;
        uv_tcp_connect(req, socket_, reinterpret_cast< const struct sockaddr* >(&addr), OnConnect);
    }

    void Write(const std::string& data)
    {
        if (uv_is_writable(reinterpret_cast< uv_stream_t* >(socket_)))
        {
            uv_buf_t    buf = uv_buf_init(const_cast< char* >(data.data()), data.size());
            uv_write_t* req = new uv_write_t;
            uv_write(req, reinterpret_cast< uv_stream_t* >(socket_), &buf, 1, OnWrite);
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
        delete req;

        if (status < 0)
        {
            std::cerr << "Write error: " << uv_strerror(status) << std::endl;
            return;
        }

        std::cout << "Data written!" << std::endl;
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
            auto callback = reinterpret_cast< readCallBack* >(stream->data);
            (*callback)(std::string(buf->base, nread));
        }

        delete[] buf->base;
    }

    void OnConnected()
    {
        // TODO: handle connection
        // Start reading from socket

        uv_read_start(reinterpret_cast< uv_stream_t* >(socket_), OnAlloc, OnRead);
    }

    static void OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
    {
        buf->base = new char[suggested_size];
        buf->len  = suggested_size;
    }

private:
    readCallBack readCallBack_;
    uv_loop_t*   loop_;
    uv_tcp_t*    socket_;
};
