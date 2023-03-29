#include <uv.h>
#include <iostream>
#include <memory>

class TcpServer {
public:
    TcpServer(uv_loop_t *loop)
    {
        uv_tcp_init(loop, &server_handle_);
        read_req_ = std::make_unique< uv_read_t >();
    }

    ~TcpServer() { uv_close((uv_handle_t *)&server_handle_, nullptr); }

    void listen(int port, std::function< void() > on_listen)
    {
        sockaddr_in addr = uv_ip4_addr("0.0.0.0", port);

        uv_tcp_bind(&server_handle_, (const struct sockaddr *)&addr, 0);
        uv_listen(
            (uv_stream_t *)&server_handle_, 128, [on_listen](uv_stream_t *server, int status) {
                if (status == -1) {
                    std::cerr << "failed to listen" << std::endl;
                } else {
                    std::cout << "listening on port " << server->port << std::endl;
                    on_listen();
                }
            });
    }

    void receive(uv_stream_t *client, std::function< void(const char *, ssize_t) > on_receive)
    {
        uv_read_start(
            client,
            [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
                buf->base = new char[suggested_size];
                buf->len  = suggested_size;
            },
            [on_receive](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
                if (nread == -1) {
                    std::cerr << "failed to read data" << std::endl;
                } else if (nread > 0) {
                    std::cout << "received " << nread << " bytes of data" << std::endl;
                    on_receive(buf->base, nread);
                }
                delete[] buf->base;
            });
    }

    void send(uv_stream_t *client, const std::string &message, std::function< void() > on_send)
    {
        auto write_req = std::make_unique< uv_write_t >();
        auto buf       = uv_buf_init((char *)message.c_str(), message.length());

        uv_write(write_req.get(), client, &buf, 1, [on_send](uv_write_t *req, int status) {
            if (status == -1) {
                std::cerr << "failed to send message" << std::endl;
            } else {
                std::cout << "message sent successfully" << std::endl;
                on_send();
            }
        });
    }

private:
    uv_tcp_t                     server_handle_;
    std::unique_ptr< uv_read_t > read_req_;
};
