#include <mutex>

namespace SuOS::Uds::ClientState { 
	enum class State {
		Idle,
		Starting,
		Working,
		Reconnecting,
		Dead,
		Stoiping,
		Error
	};

	class ClientStateManager {
	public:
		ClientStateManager() : _state(State::Idle) {}

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
} // namespace SuOS::Uds::ClientState