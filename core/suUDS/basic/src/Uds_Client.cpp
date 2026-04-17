#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <Uds_Error_type.h>
#include <sys/socket.h> // 为 SO_PEERCRED
#include <Uds_Client.h>

using namespace boost::asio;
using namespace boost::asio::local;
using namespace SuOS::Uds::Client;
//using namespace boost::system::error_code
// 设计一次性使用，不要重复连接

namespace SuOS::Uds::Client {
    Uds_Client::Uds_Client(io_context& ioc, const uint32_t cid, const std::string path, MessageCallback cb, onError onEr,onConnected oncted)
        : socket_(ioc), cid_(cid), timer_(ioc), path_(path),
        on_msg_(cb), on_error_(onEr), on_connected_(oncted) {

        //connect_to_server(timeout_sec);
    }
    
    //// 设置连接成功回调
    //void Uds_Client::set_onConnected(onConnected oncted) {
    //    on_connected_ = oncted;
    //}

    // 异步连接服务端
    void Uds_Client::connect_to_server(int timeout_sec) {
        auto self = shared_from_this();

        // 设置超时定时器
        timer_.expires_after(std::chrono::seconds(timeout_sec));
        timer_.async_wait([this, self](boost::system::error_code ec) {
            if (!ec) {
                std::cerr << "[" << cid_ << "] uds Connection timed out。" << std::endl;
                socket_.close();
                on_error_(cid_, SuOS::UdsError::ConnectTimedOut, "Connection timed out");
            }
            });

        // 异步发起连接
        socket_.async_connect(stream_protocol::endpoint(path_),
            [this, self](boost::system::error_code ec) {
                timer_.cancel(); // 取消超时定时器

                if (!ec) {
                    std::cout << "[" << cid_ << "] uds connect to server successfully" << std::endl;
                    // 连接成功后，首先获取对方（服务端）的身份信息
                    get_peer_credentials();
                    // 开始读取循环
                    start_reading();
                }
                else {
                    std::cerr << "[" << cid_ << "] uds coonect error: " << ec.message() << std::endl;
                    handle_error(ec);
                }
            });
    }

    // 异步发送数据：同样遵循 [4字节长度][数据] 的协议
    void Uds_Client::send_async(const std::vector<char>& data) {
        auto self = shared_from_this();

        // 构造完整数据包
        uint32_t len = static_cast<uint32_t>(data.size());
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(&len, sizeof(len)));
        buffers.push_back(boost::asio::buffer(data));

        // 异步写入
        async_write(socket_, buffers, [this, self](boost::system::error_code ec, size_t) {
            if (ec) {
                handle_error(ec);
            }
            });
    }

    void Uds_Client::disconnect() {
        boost::system::error_code ec;

        // 1. 取消所有异步等待（如连接超时或读取等待）
        timer_.cancel(ec);

        // 2. 优雅地关闭 Socket
        if (socket_.is_open()) { 
            // 先停止收发，再关闭句柄
            socket_.shutdown(stream_protocol::socket::shutdown_both, ec);
            socket_.close(ec);
        }

        std::cout << "[" << cid_ << "] Client Disconnected safely" << std::endl;
    }

    Uds_Client::~Uds_Client() {
        disconnect();
    }

    void Uds_Client::start_reading() {
        start_reading_header();
    }

    void Uds_Client::start_reading_header() {
        auto self = shared_from_this();

        async_read(socket_, buffer(&next_packet_length_, sizeof(next_packet_length_)),
            [this, self](boost::system::error_code ec, size_t) {
                if (!ec) {
                    if (next_packet_length_ > 0 && next_packet_length_ < 1024 * 1024) {
                        start_reading_body(next_packet_length_);
                    }
                    else {
                        start_reading_header();
                    }
                }
                else {
                    handle_error(ec);
                }
            });
    }

    void Uds_Client::start_reading_body(uint32_t len) {
        auto self = shared_from_this();
        body_buf_.resize(len);

        async_read(socket_, buffer(body_buf_),
            [this, self](boost::system::error_code ec, size_t) {
                if (!ec) {
                    if (on_msg_) on_msg_(cid_, body_buf_);
                    start_reading_header();
                }
                else {
                    handle_error(ec);
                }
            });
    }

    // 获取服务端的 PID、UID、GID
    void Uds_Client::get_peer_credentials() {
        struct ucred cred;
        socklen_t len = sizeof(struct ucred);
        int native_sock = socket_.native_handle();

        if (getsockopt(native_sock, SOL_SOCKET, SO_PEERCRED, &cred, &len) != -1) {
            if (on_connected_) {
                on_connected_(cid_, cred.pid, cred.uid, cred.gid);
            }
        }
        else {
            if (on_connected_) on_connected_(cid_, -1, -1, -1);
        }
    }

    void Uds_Client::handle_error(const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted) return;

        uint32_t type = 0;
        // 映射逻辑与 Server 端保持一致
        if (ec == boost::asio::error::connection_refused) type = SuOS::UdsError::ConnectionRefused;
        else if (ec == boost::system::errc::no_such_file_or_directory) type = SuOS::UdsError::NoSuchFileOrDirectory;
        else if (ec == boost::asio::error::broken_pipe) type = SuOS::UdsError::ConnectionClosed;
        else if (ec == boost::asio::error::timed_out) type = SuOS::UdsError::ConnectTimedOut;
        else if (ec == boost::asio::error::operation_aborted || ec == boost::asio::error::eof) {
            return; // 正常取消，不触发回调
        }

        if (on_error_) {
            on_error_(cid_, type, ec.message());
        }
    }
}

    //uint32_t next_packet_length_ = 0;
    //std::vector<char> body_buf_;
    //stream_protocol::socket socket_;
    //steady_timer timer_;
    //uint32_t cid_;
    //std::string path_;
    //MessageCallback on_msg_;
    //onError on_error_;
    //onConnected on_connected_;