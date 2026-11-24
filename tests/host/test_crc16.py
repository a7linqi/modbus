#!/usr/bin/env python3
"""CRC16 测试向量校验（不依赖硬件）。

标准 Modbus CRC16: poly=0xA001, init=0xFFFF, 数值约定 "123456789" -> 0x4B37。
注意 nanoMODBUS 的 nmbs_crc_calc 返回的是按线上顺序交换过的值（0x374B），
它按小端写入即为 37 4B，与标准值的低字节先发一致。
"""

from __future__ import annotations

import struct
import sys


def crc16_modbus(data: bytes) -> int:
    """按标准数值约定返回 CRC（0x4B37）。"""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def nmbs_crc_calc(data: bytes) -> int:
    """复刻 nanoMODBUS 的返回值：内部标准值再做高低字节交换。"""
    crc = crc16_modbus(data)
    return ((crc << 8) | (crc >> 8)) & 0xFFFF


def check(name: str, actual, expected) -> None:
    if actual != expected:
        raise AssertionError(f"{name}: 期望 {expected!r}，实际 {actual!r}")
    print(f"PASS {name}: {actual!r}")


def main() -> int:
    # 已知测试向量
    check("标准值 123456789", crc16_modbus(b"123456789"), 0x4B37)
    check("标准值空帧", crc16_modbus(b""), 0xFFFF)

    # nanoMODBUS 的返回值约定
    check("nanoMODBUS 返回值 123456789", nmbs_crc_calc(b"123456789"), 0x374B)
    # nanoMODBUS 用 put_2 高字节先写：0x374B -> 37 4B
    check("nanoMODBUS 线上字节", struct.pack(">H", nmbs_crc_calc(b"123456789")), b"\x37\x4b")

    # 标准值低字节先发，应与 nanoMODBUS 的线上字节一致
    standard = crc16_modbus(b"123456789")
    check("标准值低字节先发", bytes((standard & 0xFF, standard >> 8)), b"\x37\x4b")

    # 边界长度，验证两种实现始终一致
    for length in (0, 1, 2, 3, 7, 8, 15, 16, 255):
        data = bytes((i * 37 + 11) & 0xFF for i in range(length))
        check(
            f"长度 {length} 一致性",
            nmbs_crc_calc(data),
            ((crc16_modbus(data) << 8) | (crc16_modbus(data) >> 8)) & 0xFFFF,
        )

    print("全部 CRC16 测试通过")
    return 0


if __name__ == "__main__":
    sys.exit(main())
