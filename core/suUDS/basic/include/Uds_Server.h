#pragma once

#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <mutex>
#include <map>

// 假设该头文件定义了错误码枚举，如 connection_refused, conncet_timed_out 等
#include <Uds_Error_type.h>
 
namespace SuOS::Uds::Server {
    class Uds_Session;

    class Uds_Server : public std::enable_shared_from_this<Uds_Server> {
    public:
        struct accept_result {
            bool accept;       // 是否允许连接
            uint32_t client_id; // 给这个新客户端分配的内部 ID
        };
        using MessageCallback = std::function<void(uint32_t cid, const std::vector<char> data)>;
        using onError = std::function<void(uint32_t error_type, std::string message)>;
        // 返回  表示接受连接，返回 false 表示拒绝并关闭该 socket
        using onConnected = std::function<accept_result(int pid, int uid, int gid)>;
        // error_type表示断开连接的原因
        using onDisConnected = std::function<void(uint32_t cid, uint32_t error_type, std::string message)>;

        Uds_Server(boost::asio::io_context& ioc,
            //const uint32_t cid,
            const std::string path,
            MessageCallback cb,
            onError onEr,
            onConnected oncted,
            onDisConnected ondiscted);

        virtual ~Uds_Server();

        // 启动监听
        void start();
        //void do_accept();

        // 异步发送数据
        void send_async(uint32_t client_id, const std::vector<char>& data);
        // 发送数据给所有连接
        void send_async_toAll(const std::vector<char>& data);
        // 获取所有连接的 ID
        std::vector<uint32_t> get_connected_client_ids() const;

        // 手动关闭服务
        void close(uint32_t cid);
        void stop();

    private:
        using onSessionError = std::function<void(const uint32_t cid, uint32_t error_type, std::string message)>;
        void on_session_error(const uint32_t cid, uint32_t error_type, std::string message);
        void start_reading();
        //void start_reading_header();
        //void start_reading_body(uint32_t len);
        //void get_peer_credentials();
        void do_accept();
        void on_server_error(const boost::system::error_code& e);
        void handle_error(uint32_t error_type, std::string message);

        uint32_t next_packet_length_ = 0;
        std::vector<char> body_buf_;

        boost::asio::local::stream_protocol::acceptor acceptor_;
        boost::asio::local::stream_protocol::socket socket_;
        boost::asio::steady_timer timer_;

        //uint32_t cid_;
        std::string path_;

        MessageCallback on_msg_;
        onConnected on_connected_;
        onError on_error_;
        onDisConnected on_disconnected_;
        mutable std::mutex mutex_;
        std::map<uint32_t, std::shared_ptr<Uds_Session>> sessions_; // 管理多个 Session
    };

} // namespace suOS::Uds::Server