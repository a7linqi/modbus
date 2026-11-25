# 总览图：全链路数据流与 6 周排期

> 用途：一屏看清数据怎么走、时间怎么排。GitHub 直接渲染 Mermaid。
> 约定以 `AI-HANDOFF.md`（冻结 v1）为准，本文件只做可视化，不引入新约定。

---

## 1. 全链路数据流

```mermaid
flowchart TB
    subgraph F103["F103 · Modbus RTU 从站（单板模拟 4 个逻辑从站）"]
        direction LR
        S1["Slave 1<br/>温度 / 压力"]
        S2["Slave 2<br/>电量"]
        S3["Slave 3<br/>执行器"]
        S4["Slave 4<br/>预留 / 测试"]
    end

    subgraph F407["F407 网关 · FreeRTOS（一次只允许一个 Modbus 事务）"]
        direction LR
        R1["Rs485TransportTask<br/>UART + DMA + DE/RE"]
        R2["PollingSchedulerTask<br/>周期 / 写优先 / 离线降频"]
        R3["RegisterModelTask<br/>镜像 / 有效期 / 原子更新"]
        R4["NetworkTask<br/>LwIP TCP 上报（可选）"]
        R5["HealthMonitorTask<br/>心跳 / 栈余量 / 喂狗"]
    end

    subgraph LNX["Linux 服务 · C99"]
        direction LR
        L1["epoll 事件循环"]
        L2["拆包 / 粘包处理"]
        L3["心跳 / 断线重连"]
        L4["SQLite 批量入库"]
    end

    F103 -->|"RS485 · 115200 8N1"| R1
    R2 -->|"RequestQueue"| R1
    R1 -->|"ResultQueue"| R3
    R3 -->|"SnapshotQueue"| R4
    R4 -->|"TCP · 26 字节协议头"| L1
    L1 --> L2
    L2 --> L3
    L2 --> L4
    L4 --> DB[("device_data<br/>变化触发 + 1s 下限 + 保留期")]
    R5 -.->|"喂狗前置条件"| R1
```

要点：

- 同一时刻只有 `Rs485TransportTask` 碰 UART/RS485，其他任务通过队列交互。
- 测量值走 0x04、参数/控制走 0x03，地址一律 0-based。
- 只有所有关键任务心跳都在有效期内，`HealthMonitorTask` 才喂狗。

---

## 2. 6 周排期（关键路径）

```mermaid
gantt
    title 工业边缘网关 · 6 周关键路径（2026-11-25 → 2027-01-24）
    dateFormat YYYY-MM-DD
    axisFormat %m/%d
    todayMarker stroke-width:2px,opacity:0.35

    section 准备期（每天 ≤1h）
    文档冻结 / CRC16 / 寄存器表 / 硬件采购 :prep, 2026-11-25, 19d

    section 关键路径
    W1 F103 从站 0x03/0x04/0x06/0x10 :crit, w1, 2026-12-14, 7d
    W2 F407 主站 事务状态机 + 超时重试 :crit, w2, 2026-12-21, 7d
    W3 FreeRTOS 非阻塞状态机 + 队列 :crit, w3, 2026-12-28, 7d
    W4 4 从站调度 + 寄存器镜像 + 离线恢复 :crit, w4, 2027-01-04, 7d
    W5 Linux epoll + SQLite + gateway_simulator :crit, w5, 2027-01-11, 7d
    W6 LwIP + 联调 + 24h + README :crit, w6, 2027-01-18, 7d

    section 里程碑
    可写简历的 MVP :milestone, mvp, 2027-01-24, 0d
```

节奏：12/12 CET-4 之前每天最多 1 小时（只做文档/CRC16/寄存器表）；12/13 起每天 2–3 小时。任何一周延期都会推迟 1/24 的 MVP 节点。

| 周 | 交付物 | 验收 |
|---|---|---|
| W1 | F103 从站 4 个功能码 + CRC16 + RS485 + 故障注入 | 0x03/0x04 各连续读 1000 次无错；写回显正确；非法地址返回 0x02 |
| W2 | F407 主站 0x03/0x04/0x06/0x10 + 超时/重试 | 单从站读写正常；拔线超时、重试计数正确 |
| W3 | FreeRTOS 非阻塞状态机 + 队列 | 通信期间其他任务正常运行 |
| W4 | 4 从站调度 + 寄存器镜像 + 离线/恢复 | 关闭一个从站不影响其他；形成 MCU MVP |
| W5 | Linux C epoll + SQLite + gateway_simulator | 模拟多网关，数据按策略入库 |
| W6 | LwIP + 联调 + 故障注入 + 24h + README | 20 个场景 + 演示视频 |

> 完整验收标准与寄存器表见 `AI-HANDOFF.md`；W1 提前做全 4 个功能码，是为了避免 W2 主站测试 0x04/0x10 时从站不支持而返工。
