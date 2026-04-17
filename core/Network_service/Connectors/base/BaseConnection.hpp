#ifndef SUOS_BASE_CONNECTION_HPP
#define SUOS_BASE_CONNECTION_HPP

#include <string>
#include <memory>
#include <cstdint>

/** 
 * @brief 定义连接监听器接口
 * 上层 AppService 继承此类并实现回调逻辑
 */
class IConnectionListener {
public:
    virtual ~IConnectionListener() = default;
    virtual void onConnected(uint32_t id) = 0;
    virtual void onDisconnected(uint32_t id) = 0;
    virtual void onError(uint32_t id, int err_code, const std::string& msg) = 0;
    virtual void onDataReceived(uint32_t id, const std::string& payload) = 0;
    // 针对具体操作的反馈，比如发送成功的确认
    virtual void onActionResponse(uint32_t id, uint32_t seq_id) = 0;
};

class BaseConnection {
public:
    /**
     * @param listener 在构造时或初始化时传入，统一管理回调
     */
    BaseConnection(uint32_t id, pid_t owner_pid, std::shared_ptr<IConnectionListener> listener)
        : conn_id_(id), owner_pid_(owner_pid), listener_(listener) {}

    virtual ~BaseConnection() = default;

    // --- 核心操作接口（不再携带回调参数） ---
    virtual void connect(const std::string& config) = 0;
    virtual void disconnect() = 0;

    /**
     * @param seq_id 操作序列号，用于在 onActionResponse 中对应是哪次发送
     */
    virtual void send(const std::string& data, uint32_t seq_id) = 0;

protected:
    uint32_t conn_id_;
    pid_t owner_pid_;
    // 使用 weak_ptr 防止循环引用，确保安全销毁
    std::weak_ptr<IConnectionListener> listener_;

    // 内部便捷调用工具
    void emitError(int code, const std::string& msg) {
        if (auto l = listener_.lock()) l->onError(conn_id_, code, msg);
    }

    void emitData(int code) {
        if (auto l = listener_.lock()) l->onDataReceived(conn_id_, code);
    }

    void emitAction(const std::string& data) {
        if (auto l = listener_.lock()) l->onDataReceived(conn_id_, data);
    }
};

#endif