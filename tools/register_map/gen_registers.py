#!/usr/bin/env python3
"""从 docs/register-map.csv 生成 C 头文件、Python 常量与 Markdown 表。

单一事实源：docs/register-map.csv
生成物不要手改；改表后重新运行本脚本。
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from dataclasses import dataclass
from pathlib import Path

TYPE_REGS = {"u16": 1, "i16": 1, "u32": 2}
SPACE_CODE = {"input": 0, "holding": 1}
ACCESS_CODE = {"R": 0, "RW": 1, "W": 2}
MIRROR_BASE = {
    "status": 0x0000,
    "measurement": 0x0100,
    "control": 0x0200,
    "stats": 0x0300,
}
MIRROR_STRIDE = 0x40
C_NAME_RE = re.compile(r"^[a-z][a-z0-9_]*$")

REQUIRED_COLUMNS = [
    "slave_id",
    "space",
    "address",
    "name",
    "c_name",
    "type",
    "access",
    "scale",
    "unit",
    "default",
    "mirror_region",
    "mirror_offset",
    "description",
]


@dataclass(frozen=True)
class Point:
    slave_id: int
    space: str
    address: int
    name: str
    c_name: str
    type: str
    access: str
    scale: float
    unit: str
    default: int
    mirror_region: str
    mirror_offset: int
    description: str

    @property
    def reg_count(self) -> int:
        return TYPE_REGS[self.type]

    @property
    def mirror_addr(self) -> int:
        return MIRROR_BASE[self.mirror_region] + (self.slave_id - 1) * MIRROR_STRIDE + self.mirror_offset


def parse_int(text: str, field: str, line_no: int) -> int:
    try:
        return int(text, 0)
    except ValueError as exc:
        raise ValueError(f"第 {line_no} 行 {field} 不是整数: {text!r}") from exc


def load_points(csv_path: Path) -> list[Point]:
    with csv_path.open(newline="", encoding="utf-8-sig") as fh:
        reader = csv.DictReader(fh)
        if reader.fieldnames is None:
            raise ValueError("CSV 没有表头")
        missing = [c for c in REQUIRED_COLUMNS if c not in reader.fieldnames]
        if missing:
            raise ValueError(f"CSV 缺列: {', '.join(missing)}")

        points: list[Point] = []
        for line_no, row in enumerate(reader, start=2):
            if not any((v or "").strip() for v in row.values()):
                continue
            slave_id = parse_int(row["slave_id"], "slave_id", line_no)
            address = parse_int(row["address"], "address", line_no)
            default = parse_int(row["default"] or "0", "default", line_no)
            mirror_offset = parse_int(row["mirror_offset"], "mirror_offset", line_no)
            space = row["space"].strip()
            point_type = row["type"].strip()
            access = row["access"].strip()
            mirror_region = row["mirror_region"].strip()
            c_name = row["c_name"].strip()

            if not 1 <= slave_id <= 247:
                raise ValueError(f"第 {line_no} 行 slave_id 越界: {slave_id}")
            if space not in SPACE_CODE:
                raise ValueError(f"第 {line_no} 行 space 非法: {space}")
            if point_type not in TYPE_REGS:
                raise ValueError(f"第 {line_no} 行 type 非法: {point_type}")
            if access not in ACCESS_CODE:
                raise ValueError(f"第 {line_no} 行 access 非法: {access}")
            if mirror_region not in MIRROR_BASE:
                raise ValueError(f"第 {line_no} 行 mirror_region 非法: {mirror_region}")
            if not C_NAME_RE.match(c_name):
                raise ValueError(f"第 {line_no} 行 c_name 非法: {c_name}")
            if not 0 <= address <= 0xFFFF:
                raise ValueError(f"第 {line_no} 行 address 越界: {address}")
            if not 0 <= mirror_offset < MIRROR_STRIDE:
                raise ValueError(f"第 {line_no} 行 mirror_offset 越界: {mirror_offset}")
            reg_count = TYPE_REGS[point_type]
            if point_type == "u32" and address % 2:
                raise ValueError(f"第 {line_no} 行 u32 地址必须偶对齐: {address:#06x}")
            if address + reg_count > 0x10000:
                raise ValueError(f"第 {line_no} 行地址范围越界: {address:#06x}")

            points.append(
                Point(
                    slave_id=slave_id,
                    space=space,
                    address=address,
                    name=row["name"].strip(),
                    c_name=c_name,
                    type=point_type,
                    access=access,
                    scale=float(row["scale"]),
                    unit=row["unit"].strip(),
                    default=default,
                    mirror_region=mirror_region,
                    mirror_offset=mirror_offset,
                    description=row["description"].strip(),
                )
            )

    validate_no_overlap(points)
    return points


def validate_no_overlap(points: list[Point]) -> None:
    by_space: dict[tuple[int, str], list[Point]] = {}
    by_mirror: dict[tuple[int, str], list[Point]] = {}
    for point in points:
        by_space.setdefault((point.slave_id, point.space), []).append(point)
        by_mirror.setdefault((point.slave_id, point.mirror_region), []).append(point)

    for key, group in by_space.items():
        group.sort(key=lambda p: p.address)
        for prev, cur in zip(group, group[1:]):
            if cur.address < prev.address + prev.reg_count:
                raise ValueError(
                    f"从站 {key[0]} {key[1]} 地址重叠: "
                    f"{prev.c_name} {prev.address:#06x}+{prev.reg_count} 与 "
                    f"{cur.c_name} {cur.address:#06x}"
                )

    for key, group in by_mirror.items():
        group.sort(key=lambda p: p.mirror_offset)
        for prev, cur in zip(group, group[1:]):
            if cur.mirror_offset < prev.mirror_offset + prev.reg_count:
                raise ValueError(
                    f"从站 {key[0]} 镜像区 {key[1]} 重叠: "
                    f"{prev.c_name} +{prev.mirror_offset} 与 {cur.c_name} +{cur.mirror_offset}"
                )


def c_ident(point: Point) -> str:
    return f"MB_REG_S{point.slave_id}_{point.space.upper()}_{point.c_name.upper()}"


def generate_c_header(points: list[Point]) -> str:
    lines: list[str] = [
        "/* 本文件由 tools/register_map/gen_registers.py 生成，不要手改。 */",
        "/* 单一事实源: docs/register-map.csv */",
        "#ifndef REGS_GEN_H",
        "#define REGS_GEN_H",
        "",
        "#include <stdint.h>",
        "",
        "#ifdef __cplusplus",
        'extern "C" {',
        "#endif",
        "",
        "#define MB_REG_POINT_COUNT %dU" % len(points),
        "",
        "typedef enum {",
        "    MB_REG_SPACE_INPUT = 0,",
        "    MB_REG_SPACE_HOLDING = 1",
        "} MbRegSpace;",
        "",
        "typedef enum {",
        "    MB_REG_ACCESS_R = 0,",
        "    MB_REG_ACCESS_RW = 1,",
        "    MB_REG_ACCESS_W = 2",
        "} MbRegAccess;",
        "",
        "typedef struct {",
        "    uint8_t  slave_id;",
        "    uint8_t  space;       /* MbRegSpace */",
        "    uint16_t address;     /* 0-based 协议地址 */",
        "    uint8_t  reg_count;   /* u16/i16=1, u32=2 */",
        "    uint8_t  access;      /* MbRegAccess */",
        "    uint16_t mirror_addr; /* F407 镜像地址 */",
        "    uint16_t default_value;",
        "    const char *name;",
        "} MbRegPoint;",
        "",
        "extern const MbRegPoint g_mb_reg_points[MB_REG_POINT_COUNT];",
        "",
    ]

    for point in points:
        base = c_ident(point)
        lines.append(f"#define {base}_ADDR 0x{point.address:04X}U")
        lines.append(f"#define {base}_REGS {point.reg_count}U")
        lines.append(f"#define {base}_MIRROR 0x{point.mirror_addr:04X}U")
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append("}")
    lines.append("#endif")
    lines.append("")
    lines.append("#endif /* REGS_GEN_H */")
    lines.append("")
    return "\n".join(lines)


def generate_c_source(points: list[Point]) -> str:
    lines: list[str] = [
        "/* 本文件由 tools/register_map/gen_registers.py 生成，不要手改。 */",
        "/* 单一事实源: docs/register-map.csv */",
        '#include "regs_gen.h"',
        "",
        "const MbRegPoint g_mb_reg_points[MB_REG_POINT_COUNT] = {",
    ]
    for point in points:
        name = point.name.replace('"', '\\"')
        lines.append(
            "    { %dU, %dU, 0x%04XU, %dU, %dU, 0x%04XU, %dU, \"%s\" },"
            % (
                point.slave_id,
                SPACE_CODE[point.space],
                point.address,
                point.reg_count,
                ACCESS_CODE[point.access],
                point.mirror_addr,
                point.default & 0xFFFF,
                name,
            )
        )
    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def generate_python(points: list[Point]) -> str:
    lines: list[str] = [
        '"""本文件由 tools/register_map/gen_registers.py 生成，不要手改。"""',
        '"""单一事实源: docs/register-map.csv"""',
        "",
        "REG_POINTS = [",
    ]
    for point in points:
        lines.append(
            "    {"
            f"'slave_id': {point.slave_id}, 'space': '{point.space}', 'address': 0x{point.address:04X}, "
            f"'name': '{point.name}', 'c_name': '{point.c_name}', 'type': '{point.type}', "
            f"'access': '{point.access}', 'scale': {point.scale}, 'unit': '{point.unit}', "
            f"'default': {point.default}, 'reg_count': {point.reg_count}, "
            f"'mirror_region': '{point.mirror_region}', 'mirror_offset': {point.mirror_offset}, "
            f"'mirror_addr': 0x{point.mirror_addr:04X}"
            "},"
        )
    lines.append("]")
    lines.append("")
    lines.append("REG_BY_ADDR = {")
    lines.append("    (p['slave_id'], p['space'], p['address']): p for p in REG_POINTS")
    lines.append("}")
    lines.append("")
    lines.append("")
    lines.append("def lookup(slave_id: int, space: str, address: int):")
    lines.append('    """按起始地址查找点位；u32 只登记起始地址。"""')
    lines.append("    return REG_BY_ADDR.get((slave_id, space, address))")
    lines.append("")
    return "\n".join(lines)


def generate_markdown(points: list[Point]) -> str:
    lines: list[str] = [
        "# 寄存器表（生成物）",
        "",
        "> 本文件由 `tools/register_map/gen_registers.py` 从 `docs/register-map.csv` 生成，不要手改。",
        "",
    ]
    for slave_id in sorted({p.slave_id for p in points}):
        lines.append(f"## Slave {slave_id}")
        lines.append("")
        lines.append("| 空间 | 地址 | 名称 | 类型 | 权限 | 缩放 | 单位 | 默认 | 镜像地址 |")
        lines.append("|---|---|---|---|---|---|---|---|---|")
        for point in [p for p in points if p.slave_id == slave_id]:
            space = "0x04 输入" if point.space == "input" else "0x03 保持"
            lines.append(
                f"| {space} | 0x{point.address:04X} | {point.name} | {point.type} | "
                f"{point.access} | {point.scale:g} | {point.unit or '-'} | {point.default} | "
                f"0x{point.mirror_addr:04X} |"
            )
        lines.append("")
    return "\n".join(lines)


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description="从 register-map.csv 生成 C/Python/Markdown")
    parser.add_argument("--csv", type=Path, default=root / "docs/register-map.csv")
    parser.add_argument("--out-c", type=Path, default=root / "shared/registers/regs_gen.h")
    parser.add_argument("--out-c-src", type=Path, default=root / "shared/registers/regs_gen.c")
    parser.add_argument("--out-py", type=Path, default=root / "tools/modbus_master_tester/regs_gen.py")
    parser.add_argument("--out-md", type=Path, default=root / "docs/register-map.generated.md")
    args = parser.parse_args()

    try:
        points = load_points(args.csv)
    except (OSError, ValueError) as exc:
        print(f"错误: {exc}", file=sys.stderr)
        return 1

    for path, content in (
        (args.out_c, generate_c_header(points)),
        (args.out_c_src, generate_c_source(points)),
        (args.out_py, generate_python(points)),
        (args.out_md, generate_markdown(points)),
    ):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")
        print(f"生成 {path.relative_to(root)}")

    print(f"共 {len(points)} 个点位，校验通过")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
