#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <chrono>
#include <thread>

#include "Uds_Server.h"
#include "Uds_Client.h"

using namespace std::chrono_literals;
using namespace SuOS::Uds::Server;
using namespace SuOS::Uds::Client;

const std::string SOCKET_PATH = "/tmp/uds_test.sock";

// --- Server 回调函数 ---
Uds_Server::accept_result onServerConnected(int pid, int uid, int gid) {
    std::cout << "[Server] 收到连接请求: PID=" << pid << ", UID=" << uid << std::endl;
    static uint32_t id_counter = 100;
    return { true, id_counter++ }; // 允许连接并分配 ID
}

void onServerMessage(uint32_t cid, const std::vector<char> data) {
    std::string msg(data.begin(), data.end());
    std::cout << "[Server] 收到来自 Client[" << cid << "] 的数据: " << msg << std::endl;
}

void onServerError(const uint32_t cid, uint32_t error_type, std::string message) {
    std::cerr << "[Server] 错误 (ID: " << cid << "): " << message << " (Code: " << error_type << ")" << std::endl;
}

// --- Client 回调函数 ---
void onClientConnected(const uint32_t cid, int pid, int uid, int gid) {
    std::cout << "[Client " << cid << "] 已连接到 Server (Server PID=" << pid << ")" << std::endl;
}

void onClientMessage(const uint32_t cid, const std::vector<char> data) {
    std::string msg(data.begin(), data.end());
    std::cout << "[Client " << cid << "] 收到回传: " << msg << std::endl;
}

void onClientError(const uint32_t cid, uint32_t error_type, std::string message) {
    std::cerr << "[Client " << cid << "] 错误: " << message << std::endl;
} 

// --- 逻辑函数 ---
void run_server() {
    boost::asio::io_context ioc;
    // 清理之前的残余文件
    unlink(SOCKET_PATH.c_str());

    auto server = std::make_shared<Uds_Server>(
        ioc, SOCKET_PATH, 5,
        onServerMessage, onServerError, onServerConnected
    );

    server->start();
    std::cout << "[Server] 正在启动监听: " << SOCKET_PATH << std::endl;

    // 运行一段时间后自动停止（模拟测试时长）
    std::thread([&ioc]() {
        std::this_thread::sleep_for(10s);
        ioc.stop();
        }).detach();

    ioc.run();
}

void run_client(uint32_t client_id) {
    std::this_thread::sleep_for(1s); // 确保 Server 先启动
    boost::asio::io_context ioc;

    auto client = std::make_shared<Uds_Client>(
        ioc, client_id, SOCKET_PATH, 5,
        onClientMessage, onClientError, onClientConnected
    );

    client->connect_to_server(5);

    // 模拟异步发送数据
    std::thread([client, client_id, &ioc]() {
        std::this_thread::sleep_for(1s);
        std::string msg = "Hello from client " + std::to_string(client_id);
        std::vector<char> data(msg.begin(), msg.end());
        client->send_async(data);

        std::this_thread::sleep_for(2s);
        ioc.stop();
        }).detach();

    ioc.run();
}

int main() {
    pid_t pids[3];

    // 1. Fork Server 进程
    pids[0] = fork();
    if (pids[0] == 0) {
        run_server();
        return 0;
    }

    // 2. Fork Client 1
    pids[1] = fork();
    if (pids[1] == 0) {
        run_client(1);
        return 0;
    }

    // 3. Fork Client 2
    pids[2] = fork();
    if (pids[2] == 0) {
        run_client(2);
        return 0;
    }

    // 父进程等待所有子进程结束
    for (int i = 0; i < 3; i++) {
        waitpid(pids[i], nullptr, 0);
    }

    std::cout << "测试结束。" << std::endl;
    return 0;
}