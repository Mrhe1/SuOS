#include <string>
#include <string_view>

namespace SuOS::Uds::Df {
	// 客户端连接总超时时间，单位为秒
	inline constexpr int client_connect_TO = 6;
	// 客户端单次连接持续时间，单位为秒
	inline constexpr int client_connect_dur = 2;
	inline constexpr std::string_view uds_path = "/tmp/suUDS.sock";
	// 需根据实际情况修改
	inline constexpr std::string_view router_path = "/SuOS/router";
} 