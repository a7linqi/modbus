#!/usr/bin/env python3
"""本地 TCP 从站桩，用于无硬件时验证主站测试脚本。"""

from __future__ import annotations

import argparse
import asyncio

from pymodbus.datastore import (
    ModbusDeviceContext,
    ModbusSequentialDataBlock,
    ModbusServerContext,
)
from pymodbus.server import StartAsyncTcpServer


async def main_async(host: str, port: int) -> None:
    block = ModbusSequentialDataBlock(1, [0] * 256)
    device = ModbusDeviceContext(hr=block, ir=block)
    context = ModbusServerContext(devices=device, single=True)
    await StartAsyncTcpServer(context=context, address=(host, port))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=15020)
    args = parser.parse_args()
    asyncio.run(main_async(args.host, args.port))


if __name__ == "__main__":
    main()
