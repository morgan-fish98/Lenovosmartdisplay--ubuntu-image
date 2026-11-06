# 02 · 分区布局（GPT v2）

## 结论

保留原厂 GPT 的**前 26 项**（`modem_a … boot_b`），后面重新排：

| 分区 | 起始 LBA | 大小 | 作用 |
|---|---|---|---|
| `modem_a … boot_b`（p1..p26） | 原厂不变 | — | 引导链/基带，**不要动** |
| `system_a` | 602272 | 8 MiB | 占位（全零），给 ABL 一个合法的 A 槽 |
| `system_b` | 618656 | 128 MiB | **引导分区**：ext2，放 `/extlinux`、`/vmlinuz`、`/initramfs`、DTB |
| `pmOS_root` | 880800 | 3056 MiB | **Ubuntu rootfs**（ext4） |
| `devinfo` | 7139488 | 1 MiB | 解锁状态，**必须在** |
| `keystore … dpo`（19 个） | 7141536 起 | 共约 107 MiB | **必须在** |
| `vbmeta_a` | 7361271 | 64 KiB | 原厂 vbmeta |
| `vbmeta_b` | 7361399 | 64 KiB | 原厂 vbmeta（**不能缩成 256 字节**） |

### 尾部小分区的新位置（路线 A 必须迁移）

原厂分区表里这些分区在别的偏移；改成 GPT v2 后要把它们的内容写到下面这些位置。
**先备份（`edl r`），再写（`edl ws`）**，顺序不能反。

| 分区 | 新 LBA | 大小 |
| --- | ---: | ---: |
| `devinfo` | 7139488 | 1 MiB |
| `keystore` | 7141536 | 512 KiB |
| `config` | 7142560 | 512 KiB |
| `cmnlib_a` | 7143584 | 1 MiB |
| `cmnlib_b` | 7145632 | 1 MiB |
| `persist` | 7147680 | 32 MiB |
| `dsp_a` | 7213216 | 16 MiB |
| `dsp_b` | 7245984 | 16 MiB |
| `limits` | 7278752 | 32 KiB |
| `dip` | 7278816 | 1 MiB |
| `syscfg` | 7280864 | 512 KiB |
| `mcfg` | 7281888 | 4 MiB |
| `lksecapp_a` | 7290080 | 128 KiB |
| `lksecapp_b` | 7290336 | 128 KiB |
| `keymaster_a` | 7290592 | 1 MiB |
| `keymaster_b` | 7292640 | 1 MiB |
| `apdp` | 7294688 | 256 KiB |
| `msadp` | 7295200 | 256 KiB |
| `dpo` | 7295712 | 32 MiB |

> 少了其中任何一个，ABL/lk2nd 会在 1–2 秒内崩溃。

GPT 共 50 项，中间不留空洞。改动前后都要备份 `gpt_v2_primary.bin` 和
`gpt_v2_backup.bin`（主/备 GPT）。

## 槽属性（关键）

ABL 的槽状态**不在 `misc` 里**，而在 GPT 条目自身的 attribute 位：

```
bit48-49  优先级
bit50     激活
bit51-53  retry 计数
bit54     成功
bit55     不可启动
```

本机：

```
boot_a = 0x0080000000000000   # 不可启动
boot_b = 0x007f000000000000   # 优先级 3 + 激活 + retry 7 + 成功
```

任何整盘覆盖写（`edl wf`）都会把这些位清零。

## 五条硬性规则（都是实测踩出来的）

1. **lk2nd 的 boot 镜像必须是「合规尺寸」**
   boot header 里的 `kernel_size + ramdisk_size` 算出来的总大小必须是
   **25,180,160** 字节。否则 ABL 会连续重试 7 次然后判定槽不可启动
   （`current-slot=INVALID`）。
2. **`vbmeta_b` 必须是 64 KiB 的原厂镜像**。写一个 256 字节的空 vbmeta
   会让整个槽校验失败。
3. **引导分区必须是一个真实的 GPT 分区，内容从分区起始 LBA 开始。**
   lk2nd 只挂载 GPT 分区本身，读根目录的 `/extlinux/extlinux.conf`；
   它不会去找分区内部的 MBR，也不会猜偏移。
4. **不能删那 21 个小分区**
   （`devinfo / keystore / config / cmnlib_a,b / keymaster_a,b / lksecapp_a,b /
   persist / dsp_a,b / limits / dip / syscfg / mcfg / apdp / msadp / dpo /
   vbmeta_a,b`）。删掉后 ABL/lk2nd 会秒崩；补回来立刻恢复。
5. **槽属性在 GPT 里**，不在 `misc`；整盘写入会清零。

## 备份

刷之前至少保存：

- 原厂整盘（本机约 3.79 GB，按扇区 `dd` / `edl rf`）。
- 当前 GPT 主/备。
- `boot_b`（lk2nd）与 `system_b`（引导分区）的原始内容。
- 原厂 `vbmeta_a` / `vbmeta_b`。

恢复整盘用 EDL：

```sh
edl wf emmc_full.img
```
