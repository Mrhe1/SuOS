#include "main.h"


void SuOS::main::Main::run() {
    // 加载数据库
    _db->loadAll(std::string(SuOS::Config::configPath::config));
    // 确定启动列表(用于检查系统服务是否都启动了)
    std::list<int> start_list;
    
}

void SuOS::main::Main::stop() {

}




// 全局实例指针
static std::unique_ptr<SuOS::main::Main> g_mainInstance = nullptr;

// 进程入口函数
int main(int argc, char** argv) {
    std::cout << "Application starting..." << std::endl;
    
    // 创建主类实例
    g_mainInstance = std::make_unique<SuOS::main::Main>();
    
    try {
        // 运行主循环
        g_mainInstance->run();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return -1;
    }
    
    std::cout << "Application exited normally" << std::endl;
    return 0;
}