#include "Uds_ClientState.hpp"
#include "Uds_Client.h"
#include "Uds_ClientMgr_ErrorCode.h"
#include "suRuntime.hpp"
#include "Uds_MsgBuilder.hpp"
#include "Uds_MsgParser.hpp"
#include "SuOS_Config.h"
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

namespace SuOS::UDS::ClientMgr {
	class ClientManager {
	public:
		using onError = std::function<void(const uint32_t cid, uint32_t error_type, std::string message)>;

		ClientManager(int my_id) : _client(std::make_unique<SuOS::Uds::Client>(), _my_id(my_id), 
		_builder(new SuOS::Uds::MsgBuilder::MessageBuilder(_my_id))) {}

		uint32_t Start() {
			if (_stateMgr.getState() =! SuOS::Uds::ClientState::State::Idle) {
				return SuOS::Uds::ClientMgr::Errorcode::StateError;
			}
            _stateMgr.setState(SuOS::Uds::ClientState::State::Starting);
			*_client = initClient();
			if(_client == nullptr) return SuOS::Uds::ClientMgr::Errorcode::UdsBadEroor;
			// connect等待回报
			connect(_client);
        }

        uint32_t Stop() { 
            if (_stateMgr.getState() != SuOS::Uds::ClientState::State::Working) {
                return SuOS::Uds::ClientMgr::Errorcode::StateError;
            }
            _stateMgr.setState(SuOS::Uds::ClientState::State::Stoping);
            disconnect();
			_connect_task = nullptr;
			_client = nullptr;
            _stateMgr.setState(SuOS::Uds::ClientState::State::Idle);
            return SuOS::Uds::ClientMgr::Errorcode::Success;
        }

		uint32_t sendmsg(uint32_t sender_part, uint32_t receiver_usr,
            uint32_t receiver_part, uint32_t cmd_id, const std::vector<uint8_t>& sub_payload) {

			}

	private:
		std::unique_ptr<SuOS::Uds::Client> initClient() {
			return std::make_unique<SuOS::Uds::Client>(_router_id, _uds_path, _onUdsMsgCb, _onUdsErrorr, _onUdsConted);
		}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////连接处理//////////////////////////////
		// ///////////////////////////////////////////////////////////////////////////
		
		// // connect_dur:单次连接持续时间秒， coonect_timeout:总超时时间秒
		void connect(std::shared_ptr<SuOS::Uds::Client::Uds_Client> client) {
			if (_runtime) {

				_connect_task = *_runtime->scheduleAtFixedRate(100, (_client_connect_dur + 1) * 1000, false,
					[client, this, client_connect_dur]() {
						client->connect_to_server(_client_connect_dur);
					});

				*_runtime->schedule(_client_connect_TO, false, [this]() {
					// timeout
					onConnectResult(false, 0, 0, 0);
					});
			}
		}

		void disconnect() { 
			if (_client) {
				*_client->disconnect();
				_client = nullptr;
			}
		}

		// 对router认证 
		bool certificate(int pid, int gid, int uid) {
			char proc_path[64];
			char exe_path[PATH_MAX];

			// 1. 构造 proc 路径
			snprintf(proc_path, sizeof(proc_path), "/proc/%d/exe", pid);

			// 2. 读取软链接
			ssize_t len = readlink(proc_path, exe_path, sizeof(exe_path) - 1);

			if (len != -1) {
				exe_path[len] = '\0';

				// --- 转换与比对部分 ---

				// 将 char 数组转为 std::string
				std::string current_exe_path(exe_path);

				if (current_exe_path.ends_with(" (deleted)")) {
					// 发现路径已被标记为删除，拒绝认证
					return false;
				}
				if (current_exe_path == _router_path) {
					// 路径匹配，认证通过
					return true;
				}
				else {
					// 路径不匹配，可能是非法伪造进程
					return false;
				}
			}
			return false; // 读取失败默认不通过
		}

		// 重连结果处理
		void onReconnectingReslut(bool connected, bool is_certificated) {
			if (connected) {
				if (is_certificated) {
					std::cout << "重新连接成功" << std::endl;
					_state_manager->setState(ClientStateManager::State::Working);
				}
				else {
					std::cout << "重新连接认证失败" << std::endl;
					handleError(SuOS::Uds::ClientMgr::Errorcode::CertificationError, "CertificationError");
				}
			}
			else {
				std::cout << "重新连接失败超时" << std::endl;
				handleError(SuOS::Uds::ClientMgr::Errorcode::ConnectionTimeOut, "ConnectErrorTimeout");
			}
		}

		// 首次连接结果处理
		void onFirconnectingReslut(bool connected, bool is_certificated) {
			if (connected) {
				if (is_certificated) {
					std::cout << "首次连接成功" << std::endl;
					_state_manager->setState(ClientStateManager::State::Working);
				} else {
					std::cout << "首次连接认证失败" << std::endl;
					handleError(SuOS::Uds::ClientMgr::Errorcode::CertificationError, "CertificationError");
				}
			} else {
				std::cout << "首次连接失败超时" << std::endl;
				handleError(SuOS::Uds::ClientMgr::Errorcode::ConnectionTimeOut, "ConnectErrorTimeout");
			}
		}

		// 连接结果处理
		void onConnectResult(bool connected, int pid, int gid, int uid) {
			if (_stateMgr) {
				bool is_certificated = false;
				if (connected) {
					is_certificated = certificate(pid, gid, uid);
				}
				if (_stateMgr->getState() == SuOS::Uds::ClientState::Reconnecting) {
					onReconnectingReslut(connected, is_certificated);
				} else if (_stateMgr->getState() == SuOS::Uds::ClientState::Starting) {
					onFirconnectingReslut(connected, is_certificated);
				}
			}
		}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////错误处理//////////////////////////////////
		//////////////////////////////////////////////////////////////

		void handleError(uint32_t error_code, std::string msg) {
			int cur_state = _stateMgr->getState();
			if (cur_state == SuOS::Uds::ClientState::Dead || cur_state == SuOS::Uds::ClientState::Error) {
               return;
			}

			*_stateMgr->setState(SuOS::Uds::ClientState::Error);

			// Starting与Reconnecting阶段只处理ConnectionTimeOut
			// 忽略client的onerror
			if (cur_state == SuOS::Uds::ClientState::Starting || cur_state == SuOS::Uds::ClientState::Reconnecting)
			{
				if (error_code == SuOS::Uds::ClientMgr::Errorcode::ConnectionTimeOut || 
					error_code == SuOS::Uds::ClientMgr::Errorcode::CertificationError) {
					//报告错误
					*_stateMgr->setState(SuOS::Uds::ClientState::Dead);
				}
				else {
					// 忽略client的onerror
                    *_stateMgr->setState(cur_state);
				}
			}

			if (current_state == SuOS::Uds::ClientState::Working) {
				if (error_code == SuOS::Uds::ClientMgr::Errorcode::UdsBadEroor) {
					// 重连
					*_stateMgr->setState(SuOS::Uds::ClientState::Reconnecting);
					disconnect();
					connect(_client);
				}
			}

			// 如果错误未得到处理->忽略错误
			if(*_stateMgr->getState() == SuOS::Uds::ClientState::Error) {
				*_stateMgr->setState(cur_state);
			}
			
		}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////// client 回调实现////////////////////////////////////////
		/////////////////////////////////////////////////
		
 		// message callback
		auto _onUdsMsgCb = [this](const uint32_t cid, const std::vector<char>& data) {
			// 1. 安全检查：确保数据不为空
			if (data.empty()) {
				return;
			}

			// 2. 转换并调用 ParseMsg
			// std::vector<char>::data() 返回 char*，需要强转为 const uint8_t*
			const uint8_t* buffer = reinterpret_cast<const uint8_t*>(data.data());
			size_t size = data.size();

			auto guard = ParseMsg(buffer, size);

			// 后续业务逻辑...
		};

		// error callback
		auto _onUdsError = [this](const uint32_t cid, uint32_t error_type, std::string message) {
			uint32_t client_error;

			// 建立从 SuOS::UdsError 到 SuOS::Uds::ClientMgr::Errorcode 的转换逻辑
			switch (error_type) {
			case SuOS::UdsError::ConnectionRefused:
				client_error = SuOS::Uds::ClientMgr::Errorcode::UdsBadEroor;
				break;

			case SuOS::UdsError::NoSuchFileOrDirectory:
				client_error = SuOS::Uds::ClientMgr::Errorcode::NoSuchFileOrDirectory;
				break;

			case SuOS::UdsError::ConnectTimedOut:
				//timeout不处理由本class的connect多次连接最终确定
				//client_error = SuOS::Uds::ClientMgr::Errorcode::ConnectionTimeOut;
				break;

			case SuOS::UdsError::ConnectionClosed:
			case SuOS::UdsError::AddressInUse:
			case SuOS::UdsError::NoDescriptors:
				client_error = SuOS::Uds::ClientMgr::Errorcode::UdsBadEroor;
				break;

			default:
				client_error = SuOS::Uds::ClientMgr::Errorcode::UdsBadEroor;
				break;
			}

			handleError(client_error, message);
		};
		// connected callback
		auto _onUdsConted = [this](const uint32_t cid, int pid, int uid, int gid)>{
			onConnectResult(true, pid, uid, gid);
		};

			
		
		SuOS::Uds::ClientState::ClientStateManager _stateMgr;
		std::unique_ptr<SuOS::Uds::Client> _client = nullptr;
		Uds_MsgBuilder _builder;
        Uds_MsgParser _parser;
		std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
		std::unique_ptr<ScheduledTask> _connect_task;
		const int _client_connect_TO = SuOS::Uds::Df::client_connect_TO;
		const int _client_connect_dur = SuOS::Uds::Df::client_connect_dur;
		const int _router_id = SuOS::Config::Usr::ROUTER;
		const int _router_part = SuOS::Config::Part::ROUTER;
		// 自身usr_id
		const int _my_id;
		const std::string _uds_path = SuOS::Uds::Df::uds_path;
		const std::string _router_path = SuOS::Uds::Df::router_path;
	}
}
