# 正点原子 探索者 STM32F407 V3 · 板卡事实清单

> 用途：作为独立上下文交给其他对话核对。
> 约定：每条信息标注证据等级 —— 【实读】有上板日志 / 【官方资料】来自厂商原理图与 IO 表 / 【书面】购买凭证或官方教材 / 【待核】尚未实测。
> 生成日期：2026-11-25

---

## 1. 板卡身份

| 项目 | 内容 | 证据 |
|---|---|---|
| 板卡 | 正点原子 探索者 STM32F407 开发板，**V3**（ALIENTEK Explorer） | 【书面】旗舰店购买凭证 + V3 资料帖 |
| MCU | **STM32F407ZGT6**，LQFP144，1 MiB Flash | 【书面】官方教程/教材/华秋商城一致；【实读】Flash 容量 1 MiB |
| 屏幕 | 3.5 英寸 320×480 MCU 并口屏，ATK-MD0350 V2.4 | 【书面】SKU 为"探索者 + 3.5 寸屏" |
| 晶振 | HSE 8 MHz | 【官方资料】V2/V3 原理图 MCU 页 |
| 调试 | SWD（PA13/PA14 保留），ST-LINK | 【实读】SWD Hot Plug 连接成功 |

**注意**：旧版 V2 原理图标注 MCU 为 ZET6；V3 手册与 V3 IO 表均为 **ZGT6**。探索者不存在 ZET6 量产版本，卖家标题中的 "ZET" 属误标。

---

## 2. 已实测读到的数据

| 项目 | 读数 | 来源 |
|---|---|---|
| Device ID | 0x413；DBGMCU_IDCODE = 0x10076413；REV_ID = 0x1007（工具显示 Rev 2.0） | 【实读】ST-LINK/SWD 只读 |
| 内部 Flash 容量 | 寄存器 0x1FFF7A22 读出 0x0400 → 1024 KiB | 【实读】 |
| SPI NOR JEDEC ID | `EF 40 18` → W25Q128 系列（16 MiB） | 【实读】RDID 命令 |
| EEPROM | 7 位地址 0x50，读出 256 字节，CRC32 = 0x29058C73 | 【实读】 |
| 系统时钟 | HCLK / PCLK1 / PCLK2 = 168 / 42 / 84 MHz | 【实读】配置读回（非示波器测量） |
| 内部 Flash 备份 | 两份各 1048576 字节，SHA256 相同 | 【实读】 |
| 目标参考电压 | 3.25 V（ST-LINK 报告） | 【实读】 |

**限制**：Device ID / JEDEC ID 能证明"能通信"，不能证明完整型号、真伪或 Flash 后缀。

---

## 3. 引脚分配（官方 IO 表 + 原理图逐项核对）

| 功能 | 引脚 | 配置与备注 |
|---|---|---|
| 红色 LED / 绿色 LED | **PF9 / PF10** | 低电平点亮 |
| 日志串口 | **USART1：PA9 TX / PA10 RX** | 115200 8N1，走板载 USB 转串口 |
| CAN | **CAN1：PA11 RX / PA12 TX** | AF9；500 kbit/s |
| EEPROM | **I2C1：PB8 SCL / PB9 SDA** | AF4；AT24C02，7 位地址 0x50（不是 0xA0） |
| SPI NOR | **SPI1：PB3 SCK / PB4 MISO / PB5 MOSI** | AF5；**PB14 = 片选 CS，低有效** |
| 无线模块 | **PG7 = CS（保持高）/ PG6 = CE（保持低）** | 与 NOR 共用 SPI1 |
| 无线中断 | **PG8 = NRF_IRQ** | 与 RS485 方向脚共用，见第 4 节 |
| KEY0 | **PE4** | 低有效，内部上拉 |
| RS485 | **USART2：PA2 / PA3**；方向 **PG8** | 详见第 4 节 |

**同总线共享提示**：V3 的 I²C1（PB8/PB9）除 24C02 外，还接 **ST480MC 触摸控制器** 和 **ES8388 音频 Codec**，读写 EEPROM 时 0x50 不得与二者冲突。

**引脚复用提示**：PB3/PB4 复位后兼有 JTAG/SWO 功能，配置为 SPI1 后 SWO 不可用，SWD（PA13/PA14）仍保留。

---

## 4. RS485 电路（重点核对项）

### 4.1 收发器型号

板上实际器件为 **TPT8485**（3PEAK），**不是 SP3485**。
两者 pin-to-pin 兼容，8 脚封装，引脚定义一致：

```
1 = RO    2 = /RE    3 = DE    4 = DI
5 = GND   6 = A      7 = B     8 = VCC
```

### 4.2 连接关系（EXPLORER_V3.4.pdf 第 4 页 RS485 图块）

```
U6  TPT8485
  pin 1  RO   ←──── 网络 RS485_TX   （接 MCU 的 USART2_RX）
  pin 2  /RE  ←──── 网络 RS485_RE   （标注 PG8）
  pin 3  DE   ─────┐
                   └─ 与 pin 2 短接在同一节点
  pin 4  DI   ←──── 网络 RS485_RX   （接 MCU 的 USART2_TX）
  pin 5  GND
  pin 6  A    ───── 120R 终端电阻 + R30
  pin 7  B    ───── 360R 上下拉（R24 / R26）
  pin 8  VCC  ←──── VCC3.3（C43 = 104 去耦）
```

### 4.3 方向控制结论

**DE 与 /RE 短接，由单根 PG8 控制：**

```
PG8 = 1  →  DE=1（发送使能）、/RE=1（接收禁止）  →  发送模式
PG8 = 0  →  DE=0（发送禁止）、/RE=0（接收使能）  →  接收模式
```

**注意事项（三条）：**

1. **P4 跳线帽必须选 RS485 侧**。PA2/PA3 同时接 RS232（COM2，经 TPT3232）与 RS485，靠 P4 二选一；跳线错时串口完全无响应且代码看不出问题。
2. **PG8 与无线模块 IRQ 共用**。插入 NRF24L01 模块时，模块会往 PG8 驱动信号，与方向控制冲突。两者不可同时使用。
3. **发送结束必须等 TC（Transmission Complete）再切回接收**，不能只等 TXE；否则最后一个字节的停止位被截断。

```c
/* 参考实现 */
static void rs485_tx_mode(void) { HAL_GPIO_WritePin(GPIOG, GPIO_PIN_8, GPIO_PIN_SET); }
static void rs485_rx_mode(void) { HAL_GPIO_WritePin(GPIOG, GPIO_PIN_8, GPIO_PIN_RESET); }

void rs485_send(const uint8_t *buf, uint16_t len)
{
    rs485_tx_mode();
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 100);
    while ((USART2->SR & USART_SR_TC) == 0u) { }   /* 关键：等 TC */
    rs485_rx_mode();
}
```

### 4.4 网络命名容易看错

| 原理图网络名 | 接到 | 对应的 MCU 信号 |
|---|---|---|
| `RS485_RX` | DI（pin 4） | PA2 = **USART2_TX** |
| `RS485_TX` | RO（pin 1） | PA3 = **USART2_RX** |

网络名是从收发器视角命名的，**与 MCU 引脚功能名称相反**，不要按名字直接连。

---

## 5. 跳线与总线终端

| 项目 | 说明 |
|---|---|
| RS232 / RS485 选择 | **P4**，二选一 |
| CAN / USB 选择 | V3 手册为 **P5**；旧版 V2 原理图标注为 **P11**。按对应图号将 **1–3、2–4** 分别短接到 CAN 侧 |
| CAN 终端电阻 | 板载 **120 Ω**；与外部 USB-CAN 组成两端总线时，两端各一个 120 Ω（断电测 CANH-CANL 约 60 Ω 可辅助核对） |
| RS485 终端 | A/B 之间 120R（R30），另有 360R 上下拉 |
| USB 与 CAN 冲突 | 选择 CAN 侧时，PA11/PA12 不能同时作 USB 数据线 |

---

## 6. 时钟与外设配置（仓库参考 BSP 使用值）

```
PLL:     M = 8, N = 336, P = 2, Q = 7
时钟:    HCLK = 168 MHz, PCLK1 = 42 MHz, PCLK2 = 84 MHz
CAN:     分频 6, BS1 = 11 TQ, BS2 = 2 TQ, SJW = 1 TQ
         → 42 MHz / (6 × (1+11+2)) = 500 kbit/s，采样点 ≈ 85.7 %
SPI:     mode 0, 8 bit, MSB first, 84 MHz / 16 = 5.25 MHz
I²C:     100 kHz
```

**这些是按 8 MHz 晶振推导的配置值，尚未用示波器或总线分析仪实测。**

---

## 7. 与 V2 的差异（选型/查资料时容易混淆）

| 项目 | V2 | V3 |
|---|---|---|
| MCU 标注 | STM32F407**ZET6** | STM32F407**ZGT6**（1 MiB） |
| CAN 跳线 | P11 | **P5** |
| 以太网 PHY | LAN8720 | **YT8512C（RMII）** |
| 音频 Codec | WM8978 | **ES8388（I2S）** |
| 触摸 | — | **ST480MC**（挂在 I²C1） |
| 可替换器件 | — | NOR 可换 NM25Q128 / BY25Q128；CAN 收发器可换 SIT1050；RS485 收发器为 TPT8485 |

**结论：V2 的引脚图不能直接套用到 V3，尤其是以太网、音频、CAN 跳线编号。**

---

## 8. 尚未核实的项

- PCB 修订号（丝印未读出，屏幕遮挡）
- MCU 完整丝印
- NOR 完整型号后缀与页/扇区参数
- EEPROM 完整型号与页大小
- CAN 收发器完整型号
- 供电入口与实测电压
- 晶振丝印与实测频率
- 物理 CAN / RS485 总线实测（回环与波形均未做）

---

## 9. 资料来源

| 文档 | 说明 | 地址 |
|---|---|---|
| `EXPLORER_V3.4.pdf` | V3 主原理图，内部修订号 V3.4；RS485 在第 4 页 | https://github.com/Limitlloss/STM32F407_Explore_Files/blob/main/3%EF%BC%8C%E5%8E%9F%E7%90%86%E5%9B%BE/EXPLORER_V3.4.pdf |
| `探索者V3 IO引脚分配表.xlsx` | 全部 IO 分配；PG8 = RS485_RE 在第 93 行 | https://github.com/Limitlloss/STM32F407_Explore_Files/blob/main/3%EF%BC%8C%E5%8E%9F%E7%90%86%E5%9B%BE/%E6%8E%A2%E7%B4%A2%E8%80%85V3%20IO%E5%BC%95%E8%84%9A%E5%88%86%E9%85%8D%E8%A1%A8.xlsx |
| `探索者V3 硬件参考手册_V1.0.pdf` | IO 表、硬件变更表、各外设章节 | https://github.com/Limitlloss/STM32F407_Explore_Files/blob/main/%E6%8E%A2%E7%B4%A2%E8%80%85V3%20%E7%A1%AC%E4%BB%B6%E5%8F%82%E8%80%83%E6%89%8B%E5%86%8C_V1.0.pdf |
| `Explorer STM32F4_V2.2_SCH.pdf` | V2 原理图（对照用） | https://github.com/Meta-Team/Datasheets/blob/master/STM32F4/STM32F4%20%E6%8E%A2%E7%B4%A2%E8%80%85/Explorer%20STM32F4_V2.2_SCH.pdf |
| `ATK-MD0350 V2.4.pdf` | 3.5 寸屏原理图 | https://github.com/Limitlloss/STM32F407_Explore_Files/blob/main/3%EF%BC%8C%E5%8E%9F%E7%90%86%E5%9B%BE/ATK-MD0350%20V2.4.pdf |

**说明**：以上为第三方 GitHub 镜像，非厂商官方下载服务器。文件名与体积和官方资料目录一致，但不能据此推定逐字节相同。本清单中的引脚结论同时用「IO 分配表」与「原理图」两个独立来源交叉核对。
