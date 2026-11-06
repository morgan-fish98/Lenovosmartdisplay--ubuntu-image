# 10 · 4GB 空间都去哪了

## 现状

```
$ df -h /
Filesystem      Size  Used Avail Use% Mounted on
/dev/mmcblk0p29 2.9G  2.5G  273M  90% /
```

根分区 2.9 GiB，已用 2.5 GiB。机器总 eMMC 约 3.53 GiB，其余是引导链和原厂小分区。

## 大头

| 路径 | 占用 | 说明 |
|---|---|---|
| `/usr` | 1.4 G | `libLLVM` 139M、`libwebkit2gtk` 98M、mesa 47M |
| `/home/ubuntu/xiaozhi-hub-server` | 725 M | 语音 Hub |
| ├ `.venv/.../claude_agent_sdk/_bundled/claude` | **231 M** | Claude Code CLI，仅 Agent 功能使用 |
| ├ ASR 模型（sherpa-onnx int8） | 162 M | 语音识别必需 |
| └ `.git` | 22 M | Hub 源码仓库 |
| `/opt` | 136 M | 客户端 + Python 环境 |
| `/var` | 87 M | 日志、apt |
| `/usr/share/doc` | 12 M | 文档 |
| `/var/log/journal` | 18 M | 系统日志 |

## 能安全释放的

```sh
# 旧客户端版本（确认新版本稳定后）
rm /opt/thinksmart/client/previous
rm -rf /opt/thinksmart/client/0.1.0

# 不用 Claude Agent 的话（最大一块）
rm -rf /home/ubuntu/xiaozhi-hub-server/main/xiaozhi-server/.venv/lib/python3.10/site-packages/claude_agent_sdk/_bundled/claude

# Hub 的 git 历史（如果不再需要拉更新）
rm -rf /home/ubuntu/xiaozhi-hub-server/.git

# 文档和日志
apt-get clean
journalctl --vacuum-size=16M
```

保守估计可回到 **500 MB+ 空闲**。

## 内存（和空间无关，但顺手）

设备只有 2 GB RAM，已经启用了 **zram**（1.9 GB swap，压缩内存）：

```sh
swapon --show
# /dev/zram0  partition  1.9G
```

这比加 swap 文件更合适（eMMC 写入寿命有限）。

## 压缩文件系统？

理论上可以，但不建议现在做：

| 方案 | 预计节省 | 代价 |
|---|---|---|
| btrfs + zstd | 400–700 MB | 需要离线转换/重装 rootfs，有风险 |
| squashfs + overlay | 约 1 GB | 只读根 + 可写层，apt/客户端更新变复杂 |
| 删文件（上面） | 300–500 MB | 无风险，推荐 |

**结论**：先把 231 MB 的 `claude` 和 16 MB 的旧客户端删掉，空间就够用了；
真要压缩，优先考虑 btrfs，而不是 squashfs（这个系统还需要 apt 和客户端自更新）。

## 备份文件（PC 侧）

设备镜像在 PC 上也可以压缩，省的是 PC 的空间：

```sh
zstd -19 rootfs.img          # 3.2 GB -> 约 1.2-1.5 GB
zstd -19 emmc_full.img       # 3.8 GB -> 约 1.5-1.8 GB
```

压缩后的镜像**不能直接** `edl wf`，要先解压。
