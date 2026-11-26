# P3 Linux 扩展：自研边缘服务（3 周）

> 2026-11-26 决定。不新开项目：给已完成的 P3（F103 + ESP8266 + MQTT + Android）加一层**自研 Linux C 服务**，补 Linux 系统编程能力。
> 目标岗位：嵌入式 Linux / 音视频终端应用 / 智能座舱应用。
> 原工业网关计划（`gateway-architecture.md`、`task-tickets.md`、`w1-execution-plan.md`）**暂停**，保留作为架构与测试方法论参考，不再推进。

---

## 1. 为什么选这条

| 理由 | 说明 |
|---|---|
| 能力重合 | 音视频终端所需的 Linux 能力（socket/epoll、多线程、环形缓冲、文件 IO、gdb/valgrind、systemd）与通用 Linux 系统编程几乎一致 |
| 零新增硬件 | PC/Ubuntu 虚拟机即可完成；P3 设备端不动 |
| 复用现有成果 | P3 的 MQTT 上报、协议格式直接当输入，不用重新造数据源 |
| 时间可控 | 3 周，避开 CET-4（12/12）与春招准备 |
| 可增量 | 想加音视频，第 4 周加 V4L2/RTSP，1 周即可衔接（见 §8） |

## 2. 架构

```
F103 传感器 ── ESP8266 ──MQTT──┐
                              ├──► Linux 边缘服务（C，Ubuntu）
Android APP ──MQTT────────────┘        ├─ mqtt_client   订阅 / 断线重连
                                       ├─ parser         JSON 解析 + 校验
                                       ├─ store          SQLite（WAL + 批量）
                                       ├─ http_server    epoll + 极简 JSON API
                                       ├─ ring_queue     线程安全无锁/互斥队列
                                       └─ log            分级日志 + 轮转
                                              │
                                         curl / 浏览器
```

数据流：设备上报 → 服务订阅 → 解析校验 → 入队 → 独立线程落库 → HTTP 接口查询。
**两个线程 + 一个队列**，是最小但真实的并发模型。

## 3. 目录与文件

```
linux-edge-service/
├── CMakeLists.txt
├── config/edge.conf            # 键值配置（自写解析）
├── include/
│   ├── config.h  log.h  ring_queue.h
│   ├── mqtt_client.h  store.h  http_server.h
├── src/
│   ├── main.c                  # 初始化、信号处理、优雅退出
│   ├── config.c  log.c  ring_queue.c
│   ├── mqtt_client.c  store.c  http_server.c
├── systemd/edge-service.service
├── tests/
│   ├── test_ring_queue.c
│   └── test_parse.sh
└── README.md                   # 构建、部署、API 说明
```

依赖（均为系统包，无源码引入）：`libmosquitto-dev`、`libsqlite3-dev`、`libcjson-dev`、`cmake`。

## 4. 三周任务卡

### W1 · 工程骨架 + epoll HTTP 服务

- **产出**：CMake 工程可编译；配置解析；分级日志（含文件轮转）；单线程 epoll HTTP 服务，`GET /api/health` 返回 JSON。
- **必须处理**：`EINTR`/`EAGAIN`、短读短写、连接关闭、请求行过长。
- **验收**：`curl` 拿到 JSON；`strace` 能看到 `epoll_wait`；进程 `kill -TERM` 优雅退出。
- **自测问题**：epoll 水平触发与边缘触发区别？为什么 HTTP 必须处理短写？

### W2 · MQTT 订阅 + 解析 + 入库

- **产出**：MQTT 客户端（断线重连、`mosquitto_loop` 线程）；JSON 解析与字段校验；SQLite WAL + 批量提交；生产者/消费者队列。
- **必须处理**：断线重连不重复入库、`SQLITE_BUSY` 重试、队列满丢最旧并计数、未知字段兼容。
- **验收**：设备发 1000 条，库里正好 1000 条；断网 2 分钟恢复后补齐；`sqlite3` 查询与设备日志一致。
- **自测问题**：为什么用 WAL？批量提交的取舍是什么？队列为什么要有界？

### W3 · API、部署、稳定性、文档

- **产出**：`GET /api/latest`、`GET /api/history?limit=N`；systemd unit（自启、失败重启、日志入 journal）；valgrind 报告；README；简历条目。
- **必须处理**：跨线程共享数据加锁或快照；时间戳与时区；API 参数校验。
- **验收**：`systemctl restart` 后自恢复；`valgrind --leak-check=full` 无 definitely lost；连续 2 小时运行内存不涨；`kill -9` 后数据不丢（WAL）。

## 5. 验收总表（G-L）

| 门禁 | 标准 | 未通过时 |
|---|---|---|
| G-L1 | 工程编译 0 warning（`-Wall -Wextra`），health 接口可用 | 缩到单线程 HTTP + 日志，仍算 Linux 入门 |
| G-L2 | 1000 条入库 0 丢失，断网恢复补齐 | 去掉 MQTT 线程，改为读文件回放，保留入库与 API |
| G-L3 | valgrind 无泄漏、2 小时稳定、systemd 自启 | 保留前台运行版，文档注明“未守护化” |

## 6. 常见报错对照

| 现象 | 排查顺序 |
|---|---|
| `bind: Address already in use` | `ss -lntp` 找占用；是否 `SO_REUSEADDR` 未设 |
| CPU 100% 空转 | epoll 是否用了阻塞 socket；事件是否没消费完 |
| 断线后不再重连 | `mosquitto_loop_forever` 是否在独立线程；重连回调是否阻塞 |
| `database is locked` | 是否多线程共用一个连接；改成单写线程 + 队列 |
| 段错误 | `gdb ./edge-service core` → `bt full`；优先怀疑未初始化指针与队列越界 |
| 内存持续上涨 | `valgrind --leak-check=full` 看 `definitely lost`；检查 `json_object_put` 是否遗漏 |
| systemd 启动失败 | `journalctl -u edge-service -n 50`；多半是路径或权限 |
| 时区差 8 小时 | 存 UTC 还是本地时间必须在 README 写清 |

## 7. 简历条目草稿（W3 后填实测）

> **端-边-云环境监测系统 · Linux 边缘服务**　独立开发
> - 基于 Linux C（epoll + 多线程）实现 MQTT 订阅、JSON 解析与 SQLite（WAL）持久化，环形队列解耦生产与落库，断网重连与 `kill -9` 场景下数据零丢失；
> - 自研极简 HTTP JSON 接口（latest/history），systemd 托管，valgrind 验证无内存泄漏，连续运行【实测】小时 RSS 无增长；
> - 与 F103 + ESP8266 设备端、Android 端组成端-边-云链路，完成【实测】条上报数据的端到端校验。

## 8. 第 4 周可选：加音视频入口（1 天）

- `GET /api/snapshot`：服务端调用 `ffmpeg` 抓一帧 RTSP 存 JPEG 并返回（手机 IP 摄像头 App 即可当源，零成本）；
- 或 `V4L2` 采集 USB 摄像头存 MP4。
- 简历补一行：“接入 RTSP 视频源，实现抓帧与本地存储”，不影响主线结论。
- 若要深入音视频，再单开 3 周计划（V4L2 → H.264 → RTSP），不在本计划范围。

## 9. 与旧计划的关系

- 旧计划的核心资产（寄存器表生成器、CRC 测试、故障注入思路、门禁方法）可迁移：**门禁、验收、证据归档的做法照用**；
- 旧计划的工业协议内容不再推进；
- 硬件（MAX3485 模块、电位器、AHT20）可退可不退：AHT20/电位器若已买，可接到 P3 做真实传感器；MAX3485 与 RS485 暂无用途。
