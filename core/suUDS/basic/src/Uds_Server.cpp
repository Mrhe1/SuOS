#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <functional> 
#include <Uds_Error_type.h>
#include <unistd.h>
#include <sys/socket.h>
#include<Uds_Server.h>
#include <map>

using namespace boost::asio;
using namespace boost::asio::local;

namespace SuOS::Uds::Server { 
    // 
    class Uds_Session : public std::enable_shared_from_this<Uds_Session> {
    public:
        Uds_Session(stream_protocol::socket socket, uint32_t cid,
            Uds_Server::MessageCallback on_msg,
            // error callback to server on_session_error
            std::function<void(const uint32_t, uint32_t, std::string)> on_err)
            : socket_(std::move(socket)), cid_(cid), on_msg_(on_msg), on_error_(on_err) {}

        void start() {
            start_reading_header();
        }

        void send_async(const std::vector<char>& data) {
            auto self = shared_from_this();
            // 修复之前提到的内存安全问题：将长度封装进 shared_ptr
            auto msg_ptr = std::make_shared<std::pair<uint32_t, std::vector<char>>>(
                static_cast<uint32_t>(data.size()), data);

            std::vector<boost::asio::const_buffer> bufs;
            bufs.push_back(boost::asio::buffer(&(msg_ptr->first), 4));
            bufs.push_back(boost::asio::buffer(msg_ptr->second));

            async_write(socket_, bufs, [this, self, bufs, msg_ptr](const boost::system::error_code& e, size_t) {
                if (e) handle_error(e);
                });
        }

        void close() {
            boost::system::error_code ec;
            socket_.close(ec);
        }

    private:
        // 将原 Uds_Server 中的 start_reading_header, start_reading_body, handle_error 移至此处
        // ... (逻辑基本不变，只需注意 handle_error 里不需要关闭 acceptor)

    // 步骤 1：启动读取固定 4 字节的头部
        void start_reading_header() {
            auto self = shared_from_this();

            // async_read 保证读满 4 字节才会回调
            async_read(socket_, buffer(&next_packet_length_, sizeof(next_packet_length_)),
                [this, self](const boost::system::error_code& e, size_t /*length*/) {
                    if (!e) {
                        if (next_packet_length_ > 0 && next_packet_length_ < 1024 * 1024) { // 安全检查：限额 1MB
                            start_reading_body(next_packet_length_);
                        }
                        else {
                            start_reading_header(); // 长度非法，重置监听
                        }
                    }
                    else {
                        std::cerr << "[" << cid_ << "] uds读头失败: " << e.message() << std::endl;
                        handle_error(e);
                    }
                });
        }

        // 步骤 2：根据头部给出的长度读取 Body
        void start_reading_body(uint32_t len) {
            auto self = shared_from_this();
            // 异步读入指定长度到 dynamic_buf_
            async_read(socket_, dynamic_buf_.prepare(len),
                [this, self, len](const boost::system::error_code& e, size_t n) {
                    if (!e) {
                        dynamic_buf_.commit(n); // 将 prepare 区域转入 data 区域

                        // 将数据提取出来
                        const char* data = boost::asio::buffer_cast<const char*>(dynamic_buf_.data());
                        std::vector<char> msg(data, data + len);
                        dynamic_buf_.consume(len); // 消费掉这部分内存，以便复用

                        on_msg_(cid_, std::move(msg));
                        start_reading_header();
                    }
                    else {
                        std::cerr << "[" << cid_ << "] uds读取失败: " << e.message() << std::endl;
                        handle_error(e);
                    }
                });
        }

        void handle_error(const boost::system::error_code& e) {
            auto ec = e.value();
            std::string msg = e.message();
            uint32_t type = 0;

            // 映射 boost 错误到 Uds_Error_type.h
            if (ec == boost::asio::error::connection_refused) {
                type = SuOS::UdsError::ConnectionRefused;
            }
            else if (ec == boost::system::errc::no_such_file_or_directory) {
                type = SuOS::UdsError::NoSuchFileOrDirectory;
            }
            else if (ec == boost::asio::error::address_in_use) {
                type = SuOS::UdsError::AddressInUse;
            }
            else if (ec == boost::asio::error::broken_pipe) {
                type = SuOS::UdsError::ConnectionClosed;
            }
            else if (ec == boost::asio::error::no_descriptors) {
                type = SuOS::UdsError::NoDescriptors;
            }
            else if (ec == boost::asio::error::timed_out) {
                type = SuOS::UdsError::ConnectTimedOut;
            }
            else if (ec == boost::asio::error::operation_aborted || ec == boost::asio::error::eof) {
                return; // 正常取消，不触发回调
            }

            if (on_error_) {
                on_error_(cid_, type, msg);
            }
        }

        boost::asio::streambuf dynamic_buf_;
        stream_protocol::socket socket_;
        uint32_t cid_;
        uint32_t next_packet_length_ = 0;
        std::vector<char> body_buf_;
        Uds_Server::MessageCallback on_msg_;
        Uds_Server::onError on_error_;
    };

    Uds_Server::Uds_Server(io_context& ioc, const std::string path,
        int timeout_sec, MessageCallback cb, onError onEr, onConnected oncted)
        : acceptor_(ioc), socket_(ioc), timer_(ioc), path_(path),
        on_msg_(cb), on_connected_(oncted), on_error_(onEr) {
    }

    void Uds_Server::start() {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            unlink(path_.c_str());
            acceptor_.open(stream_protocol::endpoint(path_).protocol());
            acceptor_.bind(stream_protocol::endpoint(path_));
            acceptor_.listen();
            // 立即开始等待连接，带超时
            //wait_for_client(timeout_sec);
            do_accept();
        }
        catch (const boost::system::error_code& e) {
            on_server_error(e);
        }
    }

    // 开始异步等待连接
    void Uds_Server::do_accept() {

        // 每次 accept 使用一个新的临时 socket
        auto temp_socket = std::make_shared<stream_protocol::socket>(acceptor_.get_executor());

        acceptor_.async_accept(*temp_socket, [this, temp_socket](boost::system::error_code ec) {
            if (!ec) {
                // 1. 获取客户端凭据
                struct ucred cred;
                socklen_t len = sizeof(struct ucred);
                bool accepted = false;
                uint32_t client_id = 0;

                if (getsockopt(temp_socket->native_handle(), SOL_SOCKET, SO_PEERCRED, &cred, &len) != -1) {
                    // 2. 调用回调询问是否接受
                    if (on_connected_) {
                        accept_result res = on_connected_(cred.pid, cred.uid, cred.gid);
                        accepted = res.accept;
                        client_id = res.client_id;
                    }
                }

                if (accepted) {
                    // 3. 接受连接：创建 Session 并启动
                    auto session = std::make_shared<Uds_Session>(
                        std::move(*temp_socket),
                        client_id,
                        on_msg_,
                        [this](const uint32_t cid, uint32_t type, std::string msg) {
                            this->on_session_error(cid, type, msg);
                        }
                    );

                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        // 将 session 存入 map 管理（自定义的 id）
                        sessions_[client_id] = session;
                    }
                    session->start();
                }
                else {
                    // 4. 拒绝连接：直接关闭 socket
                    temp_socket->close();
                }
            }

            // 5. 核心：无论成功失败，继续监听下一个客户端
            do_accept();
            });
    }

    // 异步发送数据：同样遵循 [4字节长度][数据] 的协议
    void Uds_Server::send_async(uint32_t cid, const std::vector<char>& data) {
        std::shared_ptr<Uds_Session> target_session;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sessions_.find(cid);
            if (it != sessions_.end()) {
                target_session = it->second; // 拿到安全的引用
            }
        }

        if (target_session) {
            target_session->send_async(data);
        }
    }

    void Uds_Server::send_async_toAll(const std::vector<char>& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& session : sessions_) {
            session.second->send_async(data);
        }
    }

    void Uds_Server::close(uint32_t cid) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.find(cid)->second->close();
    }

    void Uds_Server::stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        // 1. 停止监听新连接
        boost::system::error_code ec;
        acceptor_.close(ec);

        // 2. 停止所有已连接的客户端
        for (auto& session : sessions_) {
            session.second->close();
        }

        // 3. 停止定时器
        timer_.cancel();
    }

    // 析构函数中确保调用
    Uds_Server::~Uds_Server() {
        stop();
    }

    std::vector<uint32_t> Uds_Server::get_connected_client_ids() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<uint32_t> ids;
        ids.reserve(sessions_.size());
        for (const auto& pair : sessions_) {
            ids.push_back(pair.first);
        }
        return ids;
    }

    void Uds_Server::on_session_error(const uint32_t cid, uint32_t error_type, std::string message) {
        std::shared_ptr<Uds_Session> session_to_stop;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sessions_.find(cid);
            if (it == sessions_.end()) {
                return; // 忽略错误，因为已经不存在了
            }
            
            if (error_type == SuOS::UdsError::ConnectionClosed ||
                error_type == SuOS::UdsError::ConnectTimedOut) {
                session_to_stop = it->second; // 拷贝指针，增加引用计数
                sessions_.erase(it);          // 从 map 移除，此时还不会析构
            }
        }

        // 在锁外执行资源释放，避免 lock 状态下调用 handle_error 产生潜在死锁
        if (session_to_stop && (error_type == SuOS::UdsError::ConnectionClosed ||
            error_type == SuOS::UdsError::ConnectTimedOut)) {
            session_to_stop->close();
        }

        handle_error(cid, error_type, message);
    }

    void Uds_Server::on_server_error(const boost::system::error_code& e) {
        auto ec = e.value();

        std::string message = e.message();
        uint32_t type = 0;

        // 映射 boost 错误到 Uds_Error_type.h
        if (ec == boost::asio::error::connection_refused) {
            type = SuOS::UdsError::ConnectionRefused;
        }
        else if (ec == boost::system::errc::no_such_file_or_directory) {
            type = SuOS::UdsError::NoSuchFileOrDirectory;
        }
        else if (ec == boost::asio::error::address_in_use) {
            type = SuOS::UdsError::AddressInUse;
        }
        else if (ec == boost::asio::error::broken_pipe) {
            type = SuOS::UdsError::ConnectionClosed;
        }
        else if (ec == boost::asio::error::no_descriptors) {
            type = SuOS::UdsError::NoDescriptors;
        }
        else if (ec == boost::asio::error::timed_out) {
            type = SuOS::UdsError::ConnectTimedOut;
        }
        else if (ec == boost::asio::error::operation_aborted || ec == boost::asio::error::eof) {
            return; // 正常取消，不触发回调
        }

        // -1表示server错误
        handle_error(-1, type, message);
    }

    void Uds_Server::handle_error(const uint32_t cid, uint32_t error_type, std::string message) {
        if (on_error_) on_error_(cid, error_type, message);

        if (error_type == SuOS::UdsError::NoSuchFileOrDirectory ||
            error_type == SuOS::UdsError::AddressInUse ||
            error_type == SuOS::UdsError::NoDescriptors) {
            // 错误类型为文件不存在、地址被占用、描述符不足时，关闭server
            stop();
        }
    }
}

    //uint32_t next_packet_length_ = 0; // 存储解析出的长度
    //std::vector<char> body_buf_;      // 存储实际消息内容
    //stream_protocol::acceptor acceptor_;
    //stream_protocol::socket socket_;
    //steady_timer timer_;
    //uint32_t cid_;
    //std::string path_;
    //std::string read_buf_;
    //MessageCallback on_msg_;
    //onConnected on_connected_;
    //onError on_error_;
