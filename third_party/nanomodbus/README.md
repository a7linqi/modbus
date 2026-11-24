# nanoMODBUS（第三方）

- 上游：https://github.com/debevv/nanoMODBUS
- 版本：master @ `91d6782930ee263bc760f27b0cbc5b82773c5f0d`（2026-11-24 拉取）
- 许可：MIT，见同目录 `LICENSE`
- 用途：Modbus RTU/TCP 协议编解码、CRC、功能码分发；F103 从站与 F407 主站共用。

## 约定

- 不修改 `nanomodbus.c/.h`；适配代码放 `device-f103/App/port/` 或 `gateway-f407/App/modbus/`。
- 升级时同步更新本文件的 commit 与 `LICENSE`。
- 展示仓库只保留必要文件与许可说明，不提交上游完整历史。
