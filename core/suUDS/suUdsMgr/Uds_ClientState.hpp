#pragma once

#include <mutex>

namespace SuOS::Uds::ClientMgr { 
	enum class State {
		Idle,
		Starting,
		Working,
		Reconnecting,
		Dead,
		Stopping,
		Error
	};

	class ClientMgrManager {
	public:
		ClientMgrManager() : _state(State::Idle) {}

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
} // namespace SuOS::Uds::ClientMgr