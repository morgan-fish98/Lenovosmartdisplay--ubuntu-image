# 09 · 救砖与恢复

## 先判断「死」在哪一层

| 现象 | 大概率 |
|---|---|
| 电脑上出现 `VID_18D1 PID_D00D` | 在 lk2nd fastboot，可救 |
| 出现 `VID_18D1 PID_D00B` + 串口 | 系统在跑（可能只是显示问题） |
| 只有 `VID_05C6 PID_9008` | EDL 模式，可整盘恢复 |
| 什么都没有 | 检查电源/USB 线，或长按电源 |

`ping <设备IP>` 和 SSH 能通，但屏幕黑：**先看 `docs/04-display.md`**，不要急着重刷。

## PMIC / SPMI 故障（屏幕全黑但系统正常）

### 症状

```
pmic-spmi 0-02: probe with driver pmic-spmi failed with error -5
spmi spmi-0: pmic_arb_wait_for_done: transaction failed (0x3) reg: 0x208008
spmi 0-03: deferred probe pending
```

后果：`/sys/class/drm/` 只有 `version`（没有 card0），`/sys/class/backlight/` 为空，
屏幕全黑；但串口、WiFi、网页都正常。

### 恢复：彻底断电

1. **DC 电源线和 USB 线同时拔掉**；
2. 等 **60 秒**（只拔 DC 不够，USB 会给 PMIC 供电）；
3. 只插 DC，等 **30 秒**；
4. 再插 USB。

判断：

```sh
dmesg | grep -i pmic-spmi     # 无输出 = 正常
```

## 回到 initramfs 调试 shell（临时）

如果启动失败、需要改 rootfs：

1. 在 `/boot/extlinux/extlinux.conf` 的 `append` 里加回 `pmos.debug-shell`；
2. 重启，会停在：

```
postmarketOS debug shell
Run 'pmos_continue_boot' to continue booting.
```

3. 挂载 rootfs 修东西：

```sh
mkdir -p /sysroot
mount -t ext4 /dev/mmcblk0p29 /sysroot
# ... 修改 ...
sync
umount /sysroot
pmos_continue_boot
```

修完记得把 `pmos.debug-shell` 再去掉。

## fastboot（lk2nd）

设备在 `VID_18D1 PID_D00D` 时：

```sh
fastboot devices
fastboot getvar product      # lk2nd-msm8953
fastboot continue
```

如果 `continue` 后仍然回到 fastboot，通常是引导失败或槽计数耗尽。

## 槽状态修复

槽属性在 GPT 条目里（见 `docs/02`）。如果 `current-slot` 变成 `INVALID`：

- 检查 `boot_b` 的 attribute 是否为 `0x007f000000000000`；
- 检查 `vbmeta_b` 是否是 64 KiB 原厂镜像；
- 检查 21 个小分区是否都在；
- 必要时用 `gpt_v2_primary.bin` / `gpt_v2_backup.bin` 重写 GPT。

## EDL（9008）整盘恢复

需要高通 loader（例如 `daisy_8953.mbn` 这类 msm8953 loader）和 EDL 工具。

```sh
edl --loader loader.mbn --memory emmc wf emmc_full.img   # 整盘恢复（危险）
edl --loader loader.mbn --memory emmc ws <LBA> <file>    # 写单个分区/偏移
```

常用写回位置（按本机 GPT v2，**换布局后以实际 GPT 为准**）：

```sh
edl ... ws 618656  boot_partition.img     # system_b（extlinux + 内核 + initramfs + DTB）
edl ... ws 880800  rootfs.img             # pmOS_root
edl ... ws 536736  lk2nd.img              # boot_b
edl ... ws 0       gpt_v2_primary.bin
edl ... ws 7405534 gpt_v2_backup.bin
```

> 整盘 `wf` 会清零 GPT 里的槽属性。整盘恢复后如果起不来，先检查槽属性。

## 检查清单（开不了机时按顺序）

1. 电源是否正常（DC + USB）？
2. 设备在 USB 上是什么 ID？`lsusb` / 设备管理器。
3. 串口有没有输出？最后一行是什么？
4. 能 ping 通吗？能 SSH 吗？
5. 屏幕黑但系统活：看 `docs/04`。
6. 只有 fastboot：`docs/02` 的 5 条规则。
7. 只有 9008：EDL 恢复。
