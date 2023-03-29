/*
 * @Descripttion: this is test
 * @version:
 * @Author: Arrow
 * @Date: 2023-03-29 08:42:22
 * @LastEditors: Arrow
 * @LastEditTime: 2023-03-29 08:42:58
 */

#include <uv.h>
#include <iostream>
#include <memory>

class TcpClient {
public:
    TcpClient(uv_loop_t *loop)
    {
        uv_tcp_init(loop, &client_handle_);
        connect_req_ = std::make_unique< uv_connect_t >();
        read_req_    = std::make_unique< uv_read_t >();
    }

    ~TcpClient() { uv_close((uv_handle_t *)&client_handle_, nullptr); }

    void connect(const std::string &host, int port, std::function< void() > on_connect)
    {
        sockaddr_in addr = uv_ip4_addr(host.c_str(), port);

        uv_tcp_connect(connect_req_.get(), &client_handle_, (const struct sockaddr *)&addr,
            [on_connect](uv_connect_t *req, int status) {
                if (status == -1) {
                    std::cerr << "failed to connect" << std::endl;
                } else {
                    std::cout << "connected successfully" << std::endl;
                    on_connect();
                }
            });
    }

    void send(const std::string &message, std::function< void() > on_send)
    {
        auto write_req = std::make_unique< uv_write_t >();
        auto buf       = uv_buf_init((char *)message.c_str(), message.length());

        uv_write(write_req.get(), (uv_stream_t *)&client_handle_, &buf, 1,
            [on_send](uv_write_t *req, int status) {
                if (status == -1) {
                    std::cerr << "failed to send message" << std::endl;
                } else {
                    std::cout << "message sent successfully" << std::endl;
                    on_send();
                }
            });
    }

    void receive(std::function< void(const char *, ssize_t) > on_receive)
    {
        uv_read_start((uv_stream_t *)&client_handle_,
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

private:
    uv_tcp_t                        client_handle_;
    std::unique_ptr< uv_connect_t > connect_req_;
    std::unique_ptr< uv_read_t >    read_req_;
};
