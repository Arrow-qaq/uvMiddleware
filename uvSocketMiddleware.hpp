#include <iostream>
#include <memory>
#include <uv.h>

class Socket {
public:
    Socket(uv_loop_t *loop) : loop_(loop)
    {
        uv_tcp_init(loop_, &socket_);
        socket_.data = this;
    }

    void bind(const char *ip, int port)
    {
        sockaddr_in addr;
        uv_ip4_addr(ip, port, &addr);
        uv_tcp_bind(&socket_, (const struct sockaddr *)&addr, 0);
    }

    void listen(int backlog, const std::function< void(Socket *) > &callback)
    {
        listen_callback_ = callback;
        uv_listen((uv_stream_t *)&socket_, backlog, [](uv_stream_t *server, int status) {
            auto socket = static_cast< Socket * >(server->data);
            if (status == 0) {
                // 新连接已建立，回调listen_callback_
                socket->listen_callback_(socket);
            } else {
                // 连接失败，关闭套接字
                socket->close();
            }
        });
    }

    void accept(Socket &client)
    {
        uv_accept((uv_stream_t *)&socket_, (uv_stream_t *)&client.socket_);
    }

    void connect(const char *ip, int port, const std::function< void() > &callback)
    {
        connect_callback_ = callback;
        uv_connect_t *req = new uv_connect_t{};
        sockaddr_in   addr;
        uv_ip4_addr(ip, port, &addr);
        uv_tcp_connect(
            req, &socket_, (const struct sockaddr *)&addr, [](uv_connect_t *req, int status) {
                auto socket = static_cast< Socket * >(req->handle->data);
                if (status == 0) {
                    // 连接成功，回调connect_callback_
                    socket->connect_callback_();
                } else {
                    // 连接失败，关闭套接字
                    socket->close();
                }
                delete req;
            });
    }

    void read_start(const std::function< void(const char *, ssize_t) > &callback)
    {
        read_callback_ = callback;
        uv_read_start((uv_stream_t *)&socket_,
            [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
                buf->base = new char[suggested_size];
                buf->len  = suggested_size;
            },
            [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
                auto socket = static_cast< Socket * >(stream->data);
                if (nread < 0) {
                    // 读取错误或读到了EOF，关闭套接字
                    socket->close();
                    delete[] buf->base;
                    return;
                }
                if (nread > 0) {
                    // 读取成功，回调read_callback_
                    socket->read_callback_(buf->base, nread);
                }
                delete[] buf->base;
            });
    }

    void write(const char *data, size_t len)
    {
        uv_buf_t buf = uv_buf_init(const_cast< char * >(data), len);
        auto     req = std::make_unique< uv_write_t >();
        uv_write(req.get(), (uv_stream_t *)&socket_, &buf, 1,
            [](uv_write_t *req, int status) { delete req; });
        req.release();
    }

    void close()
    {
        uv_close((uv_handle_t *)&socket_,
            [](uv_handle_t *handle) { delete static_cast< Socket * >(handle->data); });
    }

private:
    uv_loop_t                                   *loop_;
    uv_tcp_t                                     socket_;
    std::function< void(Socket *) >              listen_callback_;
    std::function< void() >                      connect_callback_;
    std::function< void(const char *, ssize_t) > read_callback_;
};

class Server : public Socket {
public:
    Server(uv_loop_t *loop) : Socket(loop) { }

    void start(const char *ip, int port, const std::function< void(Socket *) > &callback)
    {
        bind(ip, port);
        listen(128, [this, callback](Socket *client) {
            // 接受新连接请求
            auto new_client = std::make_unique< Socket >(client->loop_);
            accept(*new_client);

            // 回调用户定义的回调函数，传入新客户端的Socket对象
            callback(new_client.get());

            // 将新客户端加入clients_列表
            clients_.push_back(std::move(new_client));
        });
    }

    void broadcast(const char *data, size_t len)
    {
        for (auto &client : clients_) {
            client->write(data, len);
        }
    }

private:
    std::vector< std::unique_ptr< Socket > > clients_;
};

// int main()
// {
//     // 初始化libuv事件循环
//     uv_loop_t *loop = uv_default_loop();

//     // 创建Server对象，并启动服务器
//     auto server = std::make_unique< Server >(loop);
//     server->start("0.0.0.0", 12345, [](Socket *client) {
//         // 向新客户端发送欢迎消息
//         const char message[] = "Welcome to the server!";
//         client->write(message, sizeof(message));
//         client->read_start([](const char *data, ssize_t len) {
//             // 输出客户端发送的消息
//             std::cout << "Received: " << std::string(data, len) << std::endl;
//         });
//     });

//     // 定时向所有客户端发送消息
//     uv_timer_t timer;
//     uv_timer_init(loop, &timer);
//     uv_timer_start(
//         &timer,
//         [](uv_timer_t *handle) {
//             const char message[] = "Hello from the server!";
//             server->broadcast(message, sizeof(message));
//         },
//         0, 1000);

//     // 开始事件循环
//     uv_run(loop, UV_RUN_DEFAULT);

//     return 0;
// }

class Client : public Socket {
public:
    Client(uv_loop_t *loop) : Socket(loop) { }

    void connect(const char *ip, int port, const std::function< void() > &callback)
    {
        connect_callback_ = callback;
        uv_connect_t *req = new uv_connect_t{};
        sockaddr_in   addr;
        uv_ip4_addr(ip, port, &addr);
        uv_tcp_connect(
            req, &socket_, (const struct sockaddr *)&addr, [](uv_connect_t *req, int status) {
                auto client = static_cast< Client * >(req->handle->data);
                if (status == 0) {
                    // 连接成功，回调connect_callback_
                    client->connect_callback_();
                    client->read_start([](const char *data, ssize_t len) {
                        // 输出服务器发送的消息
                        std::cout << "Received: " << std::string(data, len) << std::endl;
                    });
                } else {
                    // 连接失败，关闭套接字
                    client->close();
                }
                delete req;
            });
    }

private:
    std::function< void() > connect_callback_;
};

// int main()
// {
//     // 初始化libuv事件循环
//     uv_loop_t *loop = uv_default_loop();

//     // 创建Client对象，并连接服务器
//     auto client = std::make_unique< Client >(loop);
//     client->connect("127.0.0.1", 12345, []() {
//         // 连接成功后向服务器发送消息
//         const char message[] = "Hello from the client!";
//         client->write(message, sizeof(message));
//     });

//     // 开始事件循环
//     uv_run(loop, UV_RUN_DEFAULT);

//     return 0;
// }
