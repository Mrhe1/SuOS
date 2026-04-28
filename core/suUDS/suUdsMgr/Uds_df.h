#ifndef UDS_DF_H
#define UDS_DF_H

#include <string>
#include <string_view>

namespace SuOS::Uds::Df {
	// 客户端尝试连接最大次数
	inline constexpr int client_connect_times = 3;
	// 客户端单次连接间隔时间，单位为ms
	inline constexpr int client_connect_dur = 2000;
	inline constexpr std::string_view uds_path = "/tmp/suUDS.sock";
	// 需根据实际情况修改
	inline constexpr std::string_view router_path = "/SuOS/router";
	// 心跳间隔时间，单位为ms
	inline constexpr int heartbeat_interval = 1600;
	// 心跳超时时间，单位为ms
	inline constexpr int heartbeat_timeout = 3000;
} 

#endif // UDS_DF_H