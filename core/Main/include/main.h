#pragma once
#include "suRuntime.hpp"
#include "suDbRegistryMgr.hpp"
#include "SuOS_Config.h"
#include <list>
#include <iostream>
#include <memory>

namespace SuOS::main {
    class Main {
    public:
        Main();
        void run();

    private:
        std::shared_ptr<SuOS::Database::RegistryManager> _db = std::make_shared<SuOS::Database::RegistryManager>(SuOS::Database::RegistryManager::instance());
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime = std::make_shared<SuOS::Runtime::suRuntime>();
    };
}
