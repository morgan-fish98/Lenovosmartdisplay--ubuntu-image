# AGENTS.md — 给 AI 助手的操作规则

你正在协助一台 **Lenovo Smart Display 8（amber / SD-8501F，APQ8053，4GB eMMC）**
的刷机/修复。人类负责物理操作（插拔电源、按 EDL 组合键、看屏幕）；
你负责命令、判断和验证。

## 开始之前

1. **完整读一遍 `README.md` 的「给 AI 助手的说明」和「路线 A/B」**。不要跳读。
2. 确认目标设备型号是 **SD-8501F / amber**，不是 View 8GB / CD-18781Y。
   用 `edl printgpt` 的输出确认：`CPU detected: "APQ8053"`、`total_blocks:15269888`。
3. 确认你拿到的是本仓库对应版本的文件（Release 页的 `SHA256SUMS` 校验通过）。
4. **本项目只在一台设备上实测过**。同型号不同批次可能面板 / 触摸 / 存储不同；
   遇到与手册不符的现象，先停下来问用户，不要照搬。

## 硬性规则（违反会变砖）

1. **先备份尾部分区，再写任何数据**。`rootfs` 会覆盖原机的一部分区域。
2. **最后写 GPT**。顺序：数据 → 尾部分区 → vbmeta → 主 GPT → 备 GPT。
3. **不要** `edl wf`（整盘写）除非明确要恢复原厂，它会清零槽属性。
4. **不要** `fastboot flash/erase/oem unlock`，bootloader 是锁的。
5. **不要**在设备上手动 `sfdisk`/`parted`/`dd /dev/mmcblk0`。
6. **不要删** `devinfo / keystore / config / cmnlib / keymaster / lksecapp /
   persist / dsp / limits / dip / syscfg / mcfg / apdp / msadp / dpo / vbmeta`
   这 21 个分区，缺一个 lk2nd 就会秒崩。
7. 任何不确定的 LBA/分区名，**停下来问人类**，不要猜。

## 每一步都要验证

- 写完每个镜像：确认 `edl` 输出没有报错。
- 写 GPT 前：`edl printgpt` 再确认一次当前表。
- 首次启动：串口应该有输出；没有输出就先查 USB/电源，不要盲目重刷。
- 装完修复：跑 `README.md` 第 2.7 节的检查。

## 和人类协作

- 进 EDL、按键组合、看屏幕、连 WiFi 密码——让人类做，你给出明确指令。
- 串口是主要控制通道（`console=ttyGS0`）。Windows 上通常是 `COM3`/`COM4`。
- 如果串口“能打开但没输出”，先等 30 秒；还没有就检查 USB gadget / 电源。
- 人类说“屏幕黑了”时，先判断是**系统挂了**还是**只是显示没起来**：
  ping / SSH / 串口能通就是后者，看 `docs/04-display.md`。

## 失败时

按 `README.md` 第 3 节的表处理。
如果两次尝试都失败，**停止并报告**：当前 USB ID、串口最后 20 行、`edl printgpt` 输出。
不要反复重刷同一组命令。

## 输出风格

- 每步告诉人类：**现在做什么、预期看到什么、失败了怎么办**。
- 不要一次丢 20 条命令；按阶段来，每阶段验证后再继续。

