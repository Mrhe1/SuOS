// Uds_Client.h
#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

// 错误码定义
#include "Uds_Error_type.h"

namespace SuOS::Uds::Client { 

    class Uds_Client : public std::enable_shared_from_this<Uds_Client> {
    public:
        using MessageCallback = std::function<void(const uint32_t cid, const std::vector<char>& data)>;
        using onError = std::function<void(const uint32_t cid, uint32_t error_type, std::string message)>;
        using onConnected = std::function<void(const uint32_t cid, int pid, int uid, int gid)>;

        Uds_Client(boost::asio::io_context& ioc,
            const uint32_t cid,
            const std::string path,
            //int timeout_sec,
            MessageCallback cb,
            onError onEr,
            onConnected oncted);

        virtual ~Uds_Client();

        // 设置连接成功回调
        //void set_onConnected(onConnected oncted);
        // 异步连接
        void connect_to_server(int timeout_sec);

        // 异步发送
        void send_async(const std::vector<char>& data);

        // 断开连接
        void disconnect();

    private:
        void start_reading();
        void start_reading_header();
        void start_reading_body(uint32_t len);
        void get_peer_credentials();
        void handle_error(const boost::system::error_code& ec);

        uint32_t next_packet_length_ = 0;
        std::vector<char> body_buf_;

        boost::asio::local::stream_protocol::socket socket_;
        boost::asio::steady_timer timer_;

        uint32_t cid_;
        std::string path_;

        MessageCallback on_msg_;
        onError on_error_;
        onConnected on_connected_;
    };

} // namespace suOS::Uds::Client