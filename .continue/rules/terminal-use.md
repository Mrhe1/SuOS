---
description: A description of your rule
---

你无法通过terminal获取输出，请在需要接受输出时不考虑terminal，但允许使用terminal来执行命令。namespace SuOS.Uds.Msg.PartExample;
// ---- 子消息类型定义 ----
table CmdRequest {
seq_no: uint32;
parameters: [ubyte];
}
table CmdResponse {
seq_no: uint32;
result_code: uint32;
data: [ubyte];
}
table EventNotify {
event_id: uint32;
severity: uint32;
detail: [ubyte];
}
table ErrorResponse {
seq_no: uint32;
error_code: uint32;
error_msg: string;
}

// 示例：一个不包含实际内容的 table
table EmptySignal {
}

// ---- Union 定义 ----
union UdsPayload {
CmdRequest,
CmdResponse,
EventNotify,
ErrorResponse,
EmptySignal,
}
// ---- Union 包装表 ----
table PayloadEnvelope {
payload: UdsPayload;
}
root_type PayloadEnvelope;
