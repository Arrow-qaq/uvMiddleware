#include <uv.h>
#include <functional>

using readCallBack = std::function< void(const char*, ssize_t) >;

class Server
{
public:
    Server(int port, uv_loop_t* loop_ = uv_default_loop()) : port(port), loop(loop_)
    {
        uv_tcp_init(loop, &server);
        uv_ip4_addr("0.0.0.0", port, &addr);
        uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
        server.data = this;
    }

    void start(readCallBack lambda_callback)
    {
        readCallBack_ = lambda_callback;
        uv_listen((uv_stream_t*)&server, SOMAXCONN, on_listen);
    }

    void stop() { uv_stop(loop); }

private:
    static void on_listen(uv_stream_t* stream, int status)
    {
        auto self = static_cast< Server* >(stream->data);
        if (status == 0)
        {
            auto client   = new uv_tcp_t;
            auto callback = new std::function< void(char*, ssize_t) >(self->readCallBack_);
            client->data  = callback;

            uv_tcp_init(self->loop, client);
            if (uv_accept(stream, (uv_stream_t*)client) == 0)
            {
                uv_read_start((uv_stream_t*)client, on_alloc_cb, on_read_cb);
            } else
            {
                uv_close((uv_handle_t*)client, nullptr);
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
        auto callback = reinterpret_cast< std::function< void(char*, ssize_t) >* >(stream->data);
        (*callback)(buf->base, nread);
    }

private:
    readCallBack       readCallBack_;
    int                port;
    uv_loop_t*         loop;
    uv_tcp_t           server;
    struct sockaddr_in addr;
};
