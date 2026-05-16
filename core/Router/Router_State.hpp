#pragma once
#include <mutex>

namespace SuOS::Uds::Router { 
    enum class State {
        Idle,
        Starting,
        Running,
        Stopping,
        Stopped,
        Error
    };

    class RouterStateMgr {
    public:
        RouterStateMgr() : _state(State::Idle) {}
        void setState(State newState) {
            std::lock_guard<std::mutex> lock(_mutex);
            _state = newState;
        }
        State getState() {
            std::lock_guard<std::mutex> lock(_mutex);
            return _state;
        }
    private:
        State _state;
        std::mutex _mutex;
    };
}