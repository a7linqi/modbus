/* 本文件由 tools/register_map/gen_registers.py 生成，不要手改。 */
/* 单一事实源: docs/register-map.csv */
#include "regs_gen.h"

const MbRegPoint g_mb_reg_points[MB_REG_POINT_COUNT] = {
    { 1U, 0U, 0x0000U, 1U, 0U, 0x0100U, 0U, "设备状态" },
    { 1U, 0U, 0x0001U, 1U, 0U, 0x0101U, 0U, "温度" },
    { 1U, 0U, 0x0002U, 1U, 0U, 0x0102U, 0U, "压力" },
    { 1U, 0U, 0x0003U, 1U, 0U, 0x0103U, 0U, "报警状态" },
    { 1U, 1U, 0x0010U, 1U, 1U, 0x0200U, 200U, "采样周期" },
    { 1U, 1U, 0x00F0U, 1U, 1U, 0x0201U, 0U, "故障注入" },
    { 2U, 0U, 0x0000U, 1U, 0U, 0x0140U, 0U, "电压" },
    { 2U, 0U, 0x0001U, 1U, 0U, 0x0141U, 0U, "电流" },
    { 2U, 0U, 0x0002U, 2U, 0U, 0x0142U, 0U, "累计电量" },
    { 2U, 1U, 0x0010U, 1U, 2U, 0x0240U, 0U, "清零命令" },
    { 2U, 1U, 0x00F0U, 1U, 1U, 0x0241U, 0U, "故障注入" },
    { 3U, 0U, 0x0000U, 1U, 0U, 0x0180U, 0U, "输出状态" },
    { 3U, 1U, 0x0010U, 1U, 1U, 0x0280U, 0U, "输出控制" },
    { 3U, 1U, 0x0011U, 1U, 1U, 0x0281U, 0U, "目标值" },
    { 3U, 1U, 0x00F0U, 1U, 1U, 0x0282U, 0U, "故障注入" },
    { 4U, 0U, 0x0000U, 1U, 0U, 0x03C0U, 0U, "运行计数" },
    { 4U, 1U, 0x0010U, 1U, 1U, 0x02C0U, 0U, "测试写入值" },
    { 4U, 1U, 0x00F0U, 1U, 1U, 0x02C1U, 0U, "故障注入" },
};
