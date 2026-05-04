#pragma once

#include "Uds_ClientState.hpp"
#include "Uds_Client.h"
#include "Uds_ClientMgr_ErrorCode.h"
#include "suRuntime.hpp"
#include "Uds_MsgBuilder.hpp"
#include "Uds_MsgParser.hpp"
#include "SuOS_Config.h"
#include "Uds_df.h"
#include "Uds_Client_heartbeat.hpp"
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <stdint.h>

namespace SuOS::Uds::ClientMgr {
	class ClientManager {
	public:
	    using onConnected = std::function<void()>;
		using onError = std::function<void(uint32_t error_type, std::string message)>;
		using onMsg = std::function<void(uint32_t sender_usr, uint32_t sender_part, uint32_t receiver_part, const std::vector<uint8_t>& sub_payload)>;
		ClientManager(int my_id, std::shared_ptr<SuOS::Runtime::suRuntime> runtime, onError onError, onMsg onMsg, onConnected onConnected) : _client(nullptr), _my_id(my_id), 
			_builder(runtime, _my_id), _parser(runtime), _runtime(runtime), _onError(onError), _onMsg(onMsg), _onConnected(onConnected),
			_heartbeat(_runtime, [this](uint32_t sender_part, uint32_t receiver_usr,
            uint32_t receiver_part, const std::vector<uint8_t>& sub_payload) {
			this->sendmsg(sender_part, receiver_usr, receiver_part, sub_payload);
			}, [this]() {
				// 心跳超时
				this->handleError(SuOS::Uds::ClientMgr::Errorcode::HeartbeatTimeout, "heartbeat timeout");
			}) {
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////// client 回调实现////////////////////////////////////////
			/////////////////////////////////////////////////
			_onUdsMsgCb = [this](const uint32_t cid, const std::vector<char>& data) {
				// 1. 安全检查：确保数据不为空
				if (data.empty()) {
					return;
				}

				// 2. 转换并调用 ParseMsg
				// std::vector<char>::data() 返回 char*，需要强转为 const uint8_t*
				const uint8_t* buffer = reinterpret_cast<const uint8_t*>(data.data());
				size_t size = data.size();

				auto guard = _parser.ParseMsg(buffer, size);

				if (guard.isValid()) {
					// 获取消息
					// 获取 payload
					auto payload_data = guard.payloadData();
					auto paylod_size = guard.payloadSize();
					auto payload = std::vector<uint8_t>(payload_data, payload_data + paylod_size);
					// 获取 sender_usr
					auto sender_usr = guard.getSenderUsr();
					// 获取 sender_part
					auto sender_part = guard.getSenderPart();
					// 获取 receiver_usr
					auto receiver_usr = guard.getReceiverUsr();
					// 获取 receiver_part
					auto receiver_part = guard.getReceiverPart();

					// 处理消息

					if(receiver_usr == _my_id) {
						if(sender_part == _haertbeat_part && sender_usr == _router_id && receiver_part == _haertbeat_part) {
							_heartbeat.onMsg(sender_usr, sender_part, receiver_part, payload);
						}
						else {
							// 处理其他命令
							if(_onMsg) {
								_onMsg(sender_usr, sender_part, receiver_part, payload);
							}
						}
					}
				}
			};

			// error callback
			_onUdsError = [this](const uint32_t cid, uint32_t error_type, std::string message) {
				uint32_t client_error;

				// 建立从 SuOS::Uds::Errorcode 到 SuOS::Uds::ClientMgr::Errorcode 的转换逻辑
				switch (error_type) {
				case SuOS::Uds::Errorcode::ConnectionRefused:
					client_error = SuOS::Uds::ClientMgr::Errorcode::UdsBadEroor;
					break;

				case SuOS::Uds::Errorcode::NoSuchFileOrDirectory:
					client_error = SuOS::Uds::ClientMgr::Errorcode::NoSuchFileOrDirectory;
					break;

				case SuOS::Uds::Errorcode::ConnectTimedOut:
					// 实际在handleerror中不会处理此错误，因为会多次连接
					client_error = SuOS::Uds::ClientMgr::Errorcode::UdsClientOK;
					break;

				case SuOS::Uds::Errorcode::ConnectionClosed:
				case SuOS::Uds::Errorcode::AddressInUse:
				case SuOS::Uds::Errorcode::NoDescriptors:
					client_error = SuOS::Uds::ClientMgr::Errorcode::UdsBadEroor;
					break;

				default:
					client_error = SuOS::Uds::ClientMgr::Errorcode::UdsBadEroor;
					break;
				}

				handleError(client_error, message);
			};
			// connected callback
			_onUdsConted = [this](const uint32_t cid, int pid, int uid, int gid) {
				onConnectResult(true, pid, uid, gid);
			};
		}

		uint32_t Start() {
			if(!_runtime->isInEventLoop()) return Uds::ClientMgr::outputErrorCode::NotInEventLoop;
			if (_stateMgr.getState() != SuOS::Uds::ClientMgr::State::Idle) {
				return Uds::ClientMgr::outputErrorCode::StateError;
			}
            _stateMgr.setState(SuOS::Uds::ClientMgr::State::Starting);
			// connect等待回报
			connect();
			return Uds::ClientMgr::outputErrorCode::UdsClientOK;
        }

        uint32_t Stop() { 
			if(!_runtime->isInEventLoop()) return Uds::ClientMgr::outputErrorCode::NotInEventLoop;
			
			// 1. 状态锁定，防止重入
			if (_stateMgr.getState() == SuOS::Uds::ClientMgr::State::Idle || 
				_stateMgr.getState() == SuOS::Uds::ClientMgr::State::Stopping) {
				return Uds::ClientMgr::outputErrorCode::StateError;
			}
			_stateMgr.setState(SuOS::Uds::ClientMgr::State::Stopping);

			// 2. 停止异步触发源 (关键！)
			_heartbeat.stop();        // 停止心跳定时器
			if (_connect_task) {
				_connect_task->cancel(); // 显式取消连接重试定时器
				_connect_task = nullptr;
			}

			// 3. 执行断开连接逻辑
			disconnect(); // 内部会调用 _client->disconnect() 并置空 _client

			// 4. 重置状态
			_stateMgr.setState(SuOS::Uds::ClientMgr::State::Idle);
			return Uds::ClientMgr::outputErrorCode::UdsClientOK;
		}

		uint32_t sendmsg(uint32_t sender_part, uint32_t receiver_usr,
            uint32_t receiver_part, const std::vector<uint8_t>& sub_payload) {
				_runtime->dispatch([this, sender_part, receiver_usr, receiver_part, sub_payload] () {
					auto guard = _builder.finalizeEnvelope(sender_part, receiver_usr, receiver_part, sub_payload);
					uint8_t* data = guard.data();
					size_t size = guard.size();
					std::vector<char> data_vec(data, data + size);
					_client->send_async(data_vec);
				});
				return Uds::ClientMgr::outputErrorCode::UdsClientOK;
			}

	private:
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////// client 回调实现////////////////////////////////////////
		/////////////////////////////////////////////////
		
 		// message callback
		// 使用 std::function 定义成员变量
		std::function<void(const uint32_t, const std::vector<char>&)> _onUdsMsgCb;
		std::function<void(const uint32_t, uint32_t, std::string)> _onUdsError;
		std::function<void(const uint32_t, int, int, int)> _onUdsConted;
			
/////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////连接处理//////////////////////////////
		// ///////////////////////////////////////////////////////////////////////////
		
		// // connect_dur:单次连接持续时间秒， coonect_timeout:总超时时间秒
		void connect() {
			if (_runtime) {
				_connect_task = _runtime->scheduleAtFixedRate(100, (_client_connect_dur + 1) * 1000,
					[this]() {
						_client = std::make_shared<SuOS::Uds::basic::Uds_Client>(_runtime->getIoContext(),_cid , _uds_path, _onUdsMsgCb, _onUdsError, _onUdsConted);
						_client->connect_to_server(_client_connect_dur);
					}, false, _client_connect_times, [this]() {
						// 超时回调// 由onConnectResult处理
						onConnectResult(false, 0, 0, 0);
					}, _client_connect_dur);
			}
		}

		void disconnect() { 
			if (_client) {
				_client->disconnect();
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
				if (current_exe_path == _router_path && gid == _router_gid && uid == _router_uid) {
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
					_stateMgr.setState(SuOS::Uds::ClientMgr::State::Working);
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
					_stateMgr.setState(SuOS::Uds::ClientMgr::State::Working);
				} else {
					std::cout << "首次连接认证失败" << std::endl;
					handleError(SuOS::Uds::ClientMgr::Errorcode::CertificationError, "CertificationError");
				}
			} else {
				std::cout << "首次连接失败超时" << std::endl;
				handleError(SuOS::Uds::ClientMgr::Errorcode::ConnectionTimeOut, "ConnectErrorTimeout");
			}
		}

		// 连接结果处理//分发给重连和首次连接处理
		void onConnectResult(bool connected, int pid, int gid, int uid) {			
			bool is_certificated = false;
			if (connected) {
				// 连接成功，取消定时任务
				if(_connect_task) {
					_connect_task->cancel();
				}
				is_certificated = certificate(pid, gid, uid);
				// 心跳启动
				if(is_certificated) _heartbeat.start();
			}
			_connect_task = nullptr;
			if (_stateMgr.getState() == SuOS::Uds::ClientMgr::State::Reconnecting) {
				onReconnectingReslut(connected, is_certificated);
			} else if (_stateMgr.getState() == SuOS::Uds::ClientMgr::State::Starting) {
				onFirconnectingReslut(connected, is_certificated);
			}			
		}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////错误处理//////////////////////////////////
		//////////////////////////////////////////////////////////////

		void handleError(uint32_t error_code, std::string msg) {
			auto cur_state = _stateMgr.getState();
			if (cur_state == SuOS::Uds::ClientMgr::State::Dead || cur_state == SuOS::Uds::ClientMgr::State::Error
				|| error_code == SuOS::Uds::ClientMgr::Errorcode::UdsClientOK) {
               return;
			}

			_stateMgr.setState(SuOS::Uds::ClientMgr::State::Error);

			// start阶段忽略UdsBadEroor，以多次连接最终结果为准
			if (cur_state == SuOS::Uds::ClientMgr::State::Starting || cur_state == SuOS::Uds::ClientMgr::State::Reconnecting)
			{
				if (error_code == SuOS::Uds::ClientMgr::Errorcode::ConnectionTimeOut || 
					error_code == SuOS::Uds::ClientMgr::Errorcode::CertificationError ||
					error_code == SuOS::Uds::ClientMgr::Errorcode::NoSuchFileOrDirectory) {
					//致命错误//报告错误
					_stateMgr.setState(SuOS::Uds::ClientMgr::State::Dead);
					uint32_t out_error_code = [&]() -> uint32_t {
						switch(error_code) {
						case SuOS::Uds::ClientMgr::Errorcode::ConnectionTimeOut :
						    if (cur_state == SuOS::Uds::ClientMgr::State::Starting) {
							return SuOS::Uds::ClientMgr::outputErrorCode::ConnectionTimeOut;
							} else if (cur_state == SuOS::Uds::ClientMgr::State::Reconnecting) {
								return SuOS::Uds::ClientMgr::outputErrorCode::ReconnectFail;
							}
						case SuOS::Uds::ClientMgr::Errorcode::CertificationError:
							return SuOS::Uds::ClientMgr::outputErrorCode::CertificationError;
						case SuOS::Uds::ClientMgr::Errorcode::NoSuchFileOrDirectory:
							return SuOS::Uds::ClientMgr::outputErrorCode::NoSuchFileOrDirectory;
							default:
							return SuOS::Uds::ClientMgr::outputErrorCode::UdsClientOK;
						}
					}();
					// 保护性停止
					_heartbeat.stop();
					_onError(out_error_code, msg);
				}
			}

			if (cur_state == SuOS::Uds::ClientMgr::State::Working) {
				if (error_code == SuOS::Uds::ClientMgr::Errorcode::UdsBadEroor || error_code == SuOS::Uds::ClientMgr::Errorcode::HeartbeatTimeout) {
					// 重连
					_heartbeat.stop();
					_stateMgr.setState(SuOS::Uds::ClientMgr::State::Reconnecting);
					disconnect();
					connect();
				}
				if (error_code == SuOS::Uds::ClientMgr::Errorcode::NoSuchFileOrDirectory) {
					//致命错误//报告错误
					_heartbeat.stop();
					_stateMgr.setState(SuOS::Uds::ClientMgr::State::Dead);
					_onError(SuOS::Uds::ClientMgr::outputErrorCode::NoSuchFileOrDirectory, msg);
				}
			}

			// 如果错误未得到处理->忽略错误
			if(_stateMgr.getState() == SuOS::Uds::ClientMgr::State::Error) {
				_stateMgr.setState(cur_state);
			}
		}
		
		onError _onError;
		onMsg _onMsg;
		onConnected _onConnected;
		SuOS::Uds::ClientMgr::Uds_Client_heartbeat _heartbeat;
		SuOS::Uds::ClientMgr::ClientMgrManager _stateMgr;
		std::shared_ptr<SuOS::Uds::basic::Uds_Client> _client = nullptr;
		SuOS::Uds::Msg::MessageBuilder _builder;
        SuOS::Uds::Msg::MessageParser _parser;
		std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
		std::shared_ptr<SuOS::Runtime::suRuntime::ScheduledTask> _connect_task;
		// 自动配置cid（client——id）
		const int _cid = 1001;
		const int _client_connect_times = SuOS::Uds::ClientMgr::Df::client_connect_times;
		const int _client_connect_dur = SuOS::Uds::ClientMgr::Df::client_connect_dur;
		const uint32_t _router_id = SuOS::Config::Usr::ROUTER;
		const uint32_t _router_part = SuOS::Config::Part::ROUTER;
		const uint32_t _haertbeat_part = SuOS::Config::Part::Heartbeat;
		const uint32_t _router_uid = SuOS::Config::uid::ROUTER;
		const uint32_t _router_gid = SuOS::Config::gid::ROUTER;
		//const uint32_t _heartbeat_id = SuOS::Config::CommandId::heartbeat_id;
		// 自身usr_id
		const uint32_t _my_id;
		const std::string _uds_path = std::string(SuOS::Uds::ClientMgr::Df::uds_path);
		const std::string _router_path = std::string(SuOS::Uds::ClientMgr::Df::router_path);
	};
}