"""本文件由 tools/register_map/gen_registers.py 生成，不要手改。"""
"""单一事实源: docs/register-map.csv"""

REG_POINTS = [
    {'slave_id': 1, 'space': 'input', 'address': 0x0000, 'name': '设备状态', 'c_name': 'device_status', 'type': 'u16', 'access': 'R', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'measurement', 'mirror_offset': 0, 'mirror_addr': 0x0100},
    {'slave_id': 1, 'space': 'input', 'address': 0x0001, 'name': '温度', 'c_name': 'temperature', 'type': 'i16', 'access': 'R', 'scale': 0.1, 'unit': '°C', 'default': 0, 'reg_count': 1, 'mirror_region': 'measurement', 'mirror_offset': 1, 'mirror_addr': 0x0101},
    {'slave_id': 1, 'space': 'input', 'address': 0x0002, 'name': '压力', 'c_name': 'pressure', 'type': 'u16', 'access': 'R', 'scale': 0.01, 'unit': 'kPa', 'default': 0, 'reg_count': 1, 'mirror_region': 'measurement', 'mirror_offset': 2, 'mirror_addr': 0x0102},
    {'slave_id': 1, 'space': 'input', 'address': 0x0003, 'name': '报警状态', 'c_name': 'alarm_status', 'type': 'u16', 'access': 'R', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'measurement', 'mirror_offset': 3, 'mirror_addr': 0x0103},
    {'slave_id': 1, 'space': 'holding', 'address': 0x0010, 'name': '采样周期', 'c_name': 'sample_period_ms', 'type': 'u16', 'access': 'RW', 'scale': 1.0, 'unit': 'ms', 'default': 200, 'reg_count': 1, 'mirror_region': 'control', 'mirror_offset': 0, 'mirror_addr': 0x0200},
    {'slave_id': 1, 'space': 'holding', 'address': 0x00F0, 'name': '故障注入', 'c_name': 'fault_mode', 'type': 'u16', 'access': 'RW', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'control', 'mirror_offset': 1, 'mirror_addr': 0x0201},
    {'slave_id': 2, 'space': 'input', 'address': 0x0000, 'name': '电压', 'c_name': 'voltage', 'type': 'u16', 'access': 'R', 'scale': 0.1, 'unit': 'V', 'default': 0, 'reg_count': 1, 'mirror_region': 'measurement', 'mirror_offset': 0, 'mirror_addr': 0x0140},
    {'slave_id': 2, 'space': 'input', 'address': 0x0001, 'name': '电流', 'c_name': 'current', 'type': 'u16', 'access': 'R', 'scale': 0.01, 'unit': 'A', 'default': 0, 'reg_count': 1, 'mirror_region': 'measurement', 'mirror_offset': 1, 'mirror_addr': 0x0141},
    {'slave_id': 2, 'space': 'input', 'address': 0x0002, 'name': '累计电量', 'c_name': 'energy', 'type': 'u32', 'access': 'R', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 2, 'mirror_region': 'measurement', 'mirror_offset': 2, 'mirror_addr': 0x0142},
    {'slave_id': 2, 'space': 'holding', 'address': 0x0010, 'name': '清零命令', 'c_name': 'clear_command', 'type': 'u16', 'access': 'W', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'control', 'mirror_offset': 0, 'mirror_addr': 0x0240},
    {'slave_id': 2, 'space': 'holding', 'address': 0x00F0, 'name': '故障注入', 'c_name': 'fault_mode', 'type': 'u16', 'access': 'RW', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'control', 'mirror_offset': 1, 'mirror_addr': 0x0241},
    {'slave_id': 3, 'space': 'input', 'address': 0x0000, 'name': '输出状态', 'c_name': 'output_status', 'type': 'u16', 'access': 'R', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'measurement', 'mirror_offset': 0, 'mirror_addr': 0x0180},
    {'slave_id': 3, 'space': 'holding', 'address': 0x0010, 'name': '输出控制', 'c_name': 'output_control', 'type': 'u16', 'access': 'RW', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'control', 'mirror_offset': 0, 'mirror_addr': 0x0280},
    {'slave_id': 3, 'space': 'holding', 'address': 0x0011, 'name': '目标值', 'c_name': 'target_value', 'type': 'u16', 'access': 'RW', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'control', 'mirror_offset': 1, 'mirror_addr': 0x0281},
    {'slave_id': 3, 'space': 'holding', 'address': 0x00F0, 'name': '故障注入', 'c_name': 'fault_mode', 'type': 'u16', 'access': 'RW', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'control', 'mirror_offset': 2, 'mirror_addr': 0x0282},
    {'slave_id': 4, 'space': 'input', 'address': 0x0000, 'name': '运行计数', 'c_name': 'run_counter', 'type': 'u16', 'access': 'R', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'stats', 'mirror_offset': 0, 'mirror_addr': 0x03C0},
    {'slave_id': 4, 'space': 'holding', 'address': 0x0010, 'name': '测试写入值', 'c_name': 'test_value', 'type': 'u16', 'access': 'RW', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'control', 'mirror_offset': 0, 'mirror_addr': 0x02C0},
    {'slave_id': 4, 'space': 'holding', 'address': 0x00F0, 'name': '故障注入', 'c_name': 'fault_mode', 'type': 'u16', 'access': 'RW', 'scale': 1.0, 'unit': '', 'default': 0, 'reg_count': 1, 'mirror_region': 'control', 'mirror_offset': 1, 'mirror_addr': 0x02C1},
]

REG_BY_ADDR = {
    (p['slave_id'], p['space'], p['address']): p for p in REG_POINTS
}


def lookup(slave_id: int, space: str, address: int):
    """按起始地址查找点位；u32 只登记起始地址。"""
    return REG_BY_ADDR.get((slave_id, space, address))
