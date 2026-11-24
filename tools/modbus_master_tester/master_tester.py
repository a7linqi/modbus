#!/usr/bin/env python3
"""W1 主站测试工具：连续读、写回显、异常码、延迟统计。

示例：
  python master_tester.py --port COM5 --slave 1 --function 4 --address 0 --count 4 --iterations 1000
  python master_tester.py --tcp 127.0.0.1:15020 --slave 1 --function 3 --address 0 --count 4 --iterations 10
"""

from __future__ import annotations

import argparse
import statistics
import sys
import time

from pymodbus.client import ModbusSerialClient, ModbusTcpClient


def build_client(args):
    if args.tcp:
        host, _, port = args.tcp.partition(":")
        return ModbusTcpClient(host=host, port=int(port or 502), timeout=args.timeout)
    return ModbusSerialClient(
        port=args.port,
        baudrate=args.baud,
        bytesize=8,
        parity="N",
        stopbits=1,
        timeout=args.timeout,
    )


def one_read(client, args):
    if args.function == 3:
        return client.read_holding_registers(args.address, count=args.count, device_id=args.slave)
    return client.read_input_registers(args.address, count=args.count, device_id=args.slave)


def one_write(client, args):
    return client.write_register(args.write_address, args.write_value, device_id=args.slave)


def fmt_ms(values: list[float]) -> str:
    if not values:
        return "n/a"
    ordered = sorted(values)
    p95 = ordered[min(len(ordered) - 1, int(len(ordered) * 0.95))]
    return (
        f"min={min(values):.2f} avg={statistics.mean(values):.2f} "
        f"p95={p95:.2f} max={max(values):.2f} ms"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Modbus 主站连续读/写测试")
    parser.add_argument("--port", default="COM5", help="串口，例如 COM5 或 /dev/ttyUSB0")
    parser.add_argument("--tcp", default=None, help="TCP 模式，host:port（用于本地回环测试）")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--slave", type=int, default=1)
    parser.add_argument("--function", type=int, choices=(3, 4), default=4)
    parser.add_argument("--address", type=int, default=0)
    parser.add_argument("--count", type=int, default=4)
    parser.add_argument("--iterations", type=int, default=100)
    parser.add_argument("--interval-ms", type=float, default=0.0)
    parser.add_argument("--timeout", type=float, default=0.05, help="秒，W1 默认 50ms")
    parser.add_argument("--write-address", type=int, default=None)
    parser.add_argument("--write-value", type=int, default=0)
    args = parser.parse_args()

    if not 1 <= args.slave <= 247:
        print("slave 必须在 1..247", file=sys.stderr)
        return 2
    if not 1 <= args.count <= 125:
        print("count 必须在 1..125", file=sys.stderr)
        return 2

    client = build_client(args)
    if not client.connect():
        print("连接失败", file=sys.stderr)
        return 2

    latencies: list[float] = []
    failures = 0
    last_values = None

    try:
        for index in range(args.iterations):
            started = time.perf_counter()
            result = one_read(client, args)
            elapsed_ms = (time.perf_counter() - started) * 1000.0
            latencies.append(elapsed_ms)

            if result.isError():
                failures += 1
                if failures <= 5:
                    print(f"[{index}] 读失败: {result}")
            else:
                last_values = result.registers

            if args.write_address is not None:
                write_result = one_write(client, args)
                if write_result.isError():
                    failures += 1
                    if failures <= 5:
                        print(f"[{index}] 写失败: {write_result}")

            if args.interval_ms:
                time.sleep(args.interval_ms / 1000.0)

            if (index + 1) % max(1, args.iterations // 10) == 0:
                print(f"进度 {index + 1}/{args.iterations}，失败 {failures}")
    finally:
        client.close()

    print("--- 结果 ---")
    print(f"成功 {args.iterations - failures}/{args.iterations}，失败 {failures}")
    print(f"延迟 {fmt_ms(latencies)}")
    if last_values is not None:
        print("最后一次寄存器:", " ".join(f"0x{v:04X}" for v in last_values))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
