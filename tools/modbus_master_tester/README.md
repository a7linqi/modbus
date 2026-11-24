# W1 主站测试工具

基于 pymodbus，支持串口 RTU 与 TCP 回环。W1 验收用。

## 安装

```bash
python -m pip install -r requirements.txt
```

## 串口连续读

```bash
python master_tester.py --port COM5 --slave 1 --function 4 --address 0 --count 4 --iterations 1000
```

- `--function 3`：读保持寄存器；`--function 4`：读输入寄存器。
- `--timeout 0.05`：W1 默认 50ms，可按帧时间调整。
- 退出码 0 表示全部成功，1 表示有失败。

## 写回显

```bash
python master_tester.py --port COM5 --slave 1 --function 3 --address 0x0010 --count 1 \
  --write-address 0x0010 --write-value 250 --iterations 100
```

## 本地 TCP 回环（无硬件时自测）

先用 pymodbus 起一个 TCP 从站，再：

```bash
python master_tester.py --tcp 127.0.0.1:15020 --slave 1 --function 3 --address 0 --count 4 --iterations 10
```

## 输出

- 成功/失败次数；
- 延迟 min/avg/p95/max；
- 最后一次读到的寄存器；
- 前 5 个错误详情。

## 与寄存器表的关系

`regs_gen.py` 由 `docs/register-map.csv` 生成，可用于查地址和点位定义；本工具本身不硬编码寄存器表。
